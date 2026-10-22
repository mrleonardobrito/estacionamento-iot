#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Lê o sensor da vaga e envia o estado à central por ESP-NOW.
// Quem cria a rede e serve a página é a central (esp32/central/central.ino).

// ---------- Configuração (distâncias em décimos de cm) ----------
const char*    VAGA_ID     = "A01";        // nome no mapa, até 3 caracteres; a 1ª letra é o setor
const uint8_t  PIN_TRIG = 5, PIN_ECHO = 18, PIN_LED_G = 26, PIN_LED_R = 27;
const uint8_t  CHANNEL     = 1;      // mesmo canal da central
// Mesmos limites da manobra na página (html/js/config.js: idealMaxCm e chegandoMaxCm).
const uint16_t OCC_ON      = 500;    // <= 50,0 cm (posição ideal) -> ocupada na hora
const uint16_t OCC_OFF     = 1000;   // > 100,0 cm (saiu da faixa "chegando") -> livre
const uint32_t DEBOUNCE_MS = 3000;   // tempo seguido acima de OCC_OFF para liberar
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

// Ocupa assim que o carro chega na posição ideal; só libera quando ele sai da faixa "chegando".
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
  if (changed || now - lastSend >= SEND_MS || (now - lastSend >= FAST_MS && distMoved())) {
    sendMsg(MSG_DATA); lastSend = now; sentDist = dist; changed = false;
  }
  if (now - lastHello >= 30000) { sendMsg(MSG_HELLO); lastHello = now; }
}
