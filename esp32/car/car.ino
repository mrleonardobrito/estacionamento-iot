#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Lê o sensor da vaga e envia o estado à central por ESP-NOW.
// Quem cria a rede e serve a página é a central (esp32/central/central.ino).

// ---------- Configuração (distâncias em décimos de cm) ----------
const char*    VAGA_ID     = "A01";        // nome no mapa, até 3 caracteres; a 1ª letra é o setor
const uint8_t  PIN_TRIG = 25, PIN_ECHO = 26, PIN_LED_G = 12, PIN_LED_R = 27, PIN_BUZZER = 14;
const bool     BUZZER_ATIVO = false; // true: buzzer ativo (liga/desliga); false: passivo (precisa de tom)
const uint16_t BUZZER_HZ   = 3000;  // frequência do tom, só para buzzer passivo (2000–4000 Hz soa melhor)
const uint8_t  CHANNEL     = 1;      // mesmo canal da central
// STOP_AT ("Pare": tom contínuo) e OCC_ON (faixa "chegando": ocupada na hora) vêm de limites.h,
// gerado a partir de html/js/config.js (pareCm e chegandoMaxCm). Para mudar, edite o config.js e
// rode `uv run esp32/gerar_pagina.py`.
#include "limites.h"
const uint32_t STOP_TONE_MS = 3000;  // duração do tom contínuo no "Pare"
const uint32_t PARKED_MS   = 2000;   // parado esse tempo na faixa "chegando" -> estacionado (buzzer silencia)
const uint16_t STILL_TOL   = 5;      // variação até 0,5 cm ainda conta como parado (ruído do sensor)
const uint16_t OCC_OFF     = OCC_ON + 20;  // saiu da faixa "chegando" com 2,0 cm de folga -> livre
const uint32_t BEEP_ON_MS  = 60;     // duração de cada bipe
const uint32_t BEEP_SLOW_MS = 600;   // intervalo entre bipes no início da faixa "chegando"
const uint32_t BEEP_FAST_MS = 150;   // intervalo entre bipes perto do "Pare"
const uint32_t DEBOUNCE_MS = 500;    // tempo seguido acima de OCC_OFF para liberar
const uint32_t PING_MS     = 70;     // HC-SR04: >= 60 ms entre disparos
const uint32_t SEND_MS     = 1000;   // envio periódico, mesmo sem mudança
const uint32_t FAST_MS     = 200;    // envio quando a distância muda (para a manobra)
const uint16_t DIST_STEP   = 10;     // mudança mínima que antecipa o envio: 1,0 cm
const uint16_t NO_ECHO     = 0xFFFF;
const uint8_t  PROTO       = 2;

enum : uint8_t { MSG_HELLO = 1, MSG_DATA = 2 };

struct __attribute__((packed)) Msg {   // car -> central
  uint8_t  type, version;
  uint16_t seq, dist10;
  uint8_t  occupied;
  char     id[4];           // VAGA_ID
  uint8_t  mac[6];          // origem
};

const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t  myMac[6];
char     myId[4];
uint16_t win[5] = {NO_ECHO, NO_ECHO, NO_ECHO, NO_ECHO, NO_ECHO};
uint8_t  wi = 0;
bool occupied = false, pending = false, changed = false;
uint32_t pendingSince = 0, lastPing = 0, lastSend = 0, lastHello = 0;
uint16_t seq = 0, dist = NO_ECHO, sentDist = NO_ECHO;
enum : uint8_t { STOP_ARMED, STOP_TONE, STOP_DONE } stopState = STOP_ARMED;  // tom do "Pare"
bool     beepOn = false;
uint32_t beepAt = 0, stopAt = 0, stillSince = 0;
uint16_t stillRef = NO_ECHO;   // distância de referência para saber se o carro está parado

// ---------- Sensor ----------
uint16_t readOnce() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  uint32_t us = pulseIn(PIN_ECHO, HIGH, 25000);            // timeout ~ 4,3 m
  return us ? (uint16_t)(us * 1715UL / 10000UL) : NO_ECHO; // d = us*0,01715 cm
}

uint16_t median5() {
  uint16_t t[5]; memcpy(t, win, sizeof(t));
  for (int i = 1; i < 5; i++) {
    uint16_t k = t[i]; int j = i - 1;
    while (j >= 0 && t[j] > k) { t[j + 1] = t[j]; j--; }
    t[j + 1] = k;
  }
  return t[2];
}

// Ocupa assim que o carro entra na faixa "chegando"; só libera quando ele sai dela (com folga e debounce).
void updateOccupancy(uint16_t d) {
  if (!occupied) {
    if (d <= OCC_ON) { occupied = true; changed = true; }
    return;
  }
  if (d <= OCC_OFF) { pending = false; return; }   // NO_ECHO (sem eco) também conta como fora
  if (!pending) { pending = true; pendingSince = millis(); }
  else if (millis() - pendingSince >= DEBOUNCE_MS) {
    occupied = false; pending = false; changed = true;
  }
}

// ---------- Buzzer (sensor de proximidade) ----------
void buzzer(bool on) {
  if (on == beepOn) return;
  beepOn = on;
  if (BUZZER_ATIVO) digitalWrite(PIN_BUZZER, on);
  else if (on) tone(PIN_BUZZER, BUZZER_HZ);
  else noTone(PIN_BUZZER);
}

// Entrando: apita repetidamente na faixa "chegando", mais rápido quanto mais perto; no "Pare", tom contínuo
// por STOP_TONE_MS. O carro conta como estacionado quando o tom termina ou quando fica PARKED_MS parado na
// faixa. Estacionado, qualquer movimento é saída: silêncio até a vaga ser liberada (próxima entrada).
void updateBuzzer(uint32_t now) {
  if (dist != NO_ECHO && (dist > OCC_ON || abs((int)dist - (int)stillRef) > STILL_TOL)) { stillRef = dist; stillSince = now; }

  if (stopState == STOP_DONE) {            // estacionado ou saindo
    if (!occupied) stopState = STOP_ARMED; // vaga liberada: pronto para a próxima entrada
    else { buzzer(false); return; }
  }
  if (stopState == STOP_ARMED && dist <= OCC_ON && now - stillSince >= PARKED_MS) { stopState = STOP_DONE; buzzer(false); return; }
  if (dist <= STOP_AT && stopState == STOP_ARMED) { stopState = STOP_TONE; stopAt = now; buzzer(true); }
  // Saiu do "Pare" durante o tom. Sem eco não conta como saída: colado no sensor o HC-SR04 às vezes não devolve eco.
  else if (stopState == STOP_TONE && dist != NO_ECHO && dist > STOP_AT) stopState = STOP_ARMED;
  if (stopState == STOP_TONE && now - stopAt >= STOP_TONE_MS) { stopState = STOP_DONE; buzzer(false); }
  if (stopState != STOP_ARMED) return;     // tom contínuo ou estacionado: nada de bipes
  if (dist > OCC_ON) { buzzer(false); return; }   // inclui NO_ECHO (0xFFFF)
  uint32_t period = map(dist, STOP_AT, OCC_ON, BEEP_FAST_MS, BEEP_SLOW_MS);
  if (beepOn) { if (now - beepAt >= BEEP_ON_MS) buzzer(false); }
  else if (now - beepAt >= period) { beepAt = now; buzzer(true); }
}

// ---------- ESP-NOW ----------
void sendMsg(uint8_t type) {
  Msg m = { type, PROTO, seq++, dist, occupied, {0}, {0} };
  memcpy(m.id, myId, 4);
  memcpy(m.mac, myMac, 6);
  esp_now_send(BCAST, (uint8_t*)&m, sizeof(m));
}

bool distMoved() {
  if ((dist == NO_ECHO) != (sentDist == NO_ECHO)) return true;
  return dist != NO_ECHO && abs((int)dist - (int)sentDist) >= DIST_STEP;
}

// ---------- Setup e loop ----------
void setup() {
  Serial.begin(115200);
  strncpy(myId, VAGA_ID, 3); myId[3] = '\0';
  pinMode(PIN_TRIG, OUTPUT); pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED_G, OUTPUT); pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_G, HIGH); digitalWrite(PIN_LED_R, LOW);   // começa livre

  WiFi.mode(WIFI_STA);                             // sem rede própria: só ESP-NOW
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  WiFi.macAddress(myMac);

  esp_now_init();
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, BCAST, 6); p.channel = CHANNEL; p.encrypt = false;
  esp_now_add_peer(&p);

  Serial.printf("Vaga %s pronta. MAC %s\n", myId, WiFi.macAddress().c_str());
  sendMsg(MSG_HELLO);
}

void loop() {
  uint32_t now = millis();
  if (now - lastPing >= PING_MS) {
    lastPing = now;
    win[wi] = readOnce(); wi = (wi + 1) % 5;
    dist = median5();
    updateOccupancy(dist);
    digitalWrite(PIN_LED_R, occupied);
    digitalWrite(PIN_LED_G, !occupied);
  }
  updateBuzzer(now);
  if (changed) {  // broadcast do ESP-NOW não tem confirmação: repete a mudança para não depender do envio de 1 s
    for (int k = 0; k < 3; k++) { sendMsg(MSG_DATA); delay(3); }
    lastSend = now; sentDist = dist; changed = false;
  } else if (now - lastSend >= SEND_MS || (now - lastSend >= FAST_MS && distMoved())) {
    sendMsg(MSG_DATA); lastSend = now; sentDist = dist;
  }
  if (now - lastHello >= 30000) { sendMsg(MSG_HELLO); lastHello = now; }
}
