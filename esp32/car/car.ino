#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "pagina.h"   // gerado de html/ por: python3 esp32/gerar_pagina.py

// ---------- Configuração (distâncias em décimos de cm) ----------
const char*    VAGA_ID     = "A01";        // nome no mapa, até 3 caracteres; a 1ª letra é o setor
const char*    AP_PASS     = "estaciona";  // rede "vagai-<VAGA_ID>", mínimo de 8 caracteres
const uint8_t  PIN_TRIG = 5, PIN_ECHO = 18, PIN_LED_G = 26, PIN_LED_R = 27;
const uint8_t  CHANNEL     = 1;      // mesmo canal da central
const uint16_t OCC_ON      = 1500;   // < 150,0 cm  -> ocupada
const uint16_t OCC_OFF     = 1700;   // > 170,0 cm  -> livre (histerese)
const uint32_t DEBOUNCE_MS = 3000;
const uint32_t PING_MS     = 70;     // HC-SR04: >= 60 ms entre disparos
const uint32_t SEND_MS     = 1000;
const uint32_t CENTRAL_MS  = 5000;   // sem tabela da central por esse tempo = mostra só esta vaga
const uint16_t NO_ECHO     = 0xFFFF;
const uint8_t  PROTO       = 2;

enum : uint8_t { MSG_HELLO = 1, MSG_DATA = 2, MSG_STATE = 4 };
enum : uint8_t { LIVRE = 0, OCUPADA = 1, SEM_SINAL = 2 };

struct __attribute__((packed)) Msg {   // car -> central
  uint8_t  type, version;
  uint16_t seq, dist10;
  uint8_t  occupied;
  char     id[4];           // VAGA_ID
  uint8_t  mac[6];          // origem
};
const uint8_t MAX_SPOTS = 64;
const uint8_t STATE_MAX = 40;
struct __attribute__((packed)) StateEntry { char id[4]; uint8_t estado; };
struct __attribute__((packed)) StateMsg {   // central -> cars
  uint8_t type, version, total, offset, count;
  StateEntry e[STATE_MAX];
};

const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t  myMac[6];
char     myId[4];
uint16_t win[5] = {NO_ECHO, NO_ECHO, NO_ECHO, NO_ECHO, NO_ECHO};
uint8_t  wi = 0;
bool occupied = false, pending = false, changed = false;
uint32_t pendingSince = 0, lastPing = 0, lastSend = 0, lastHello = 0;
uint16_t seq = 0, dist = NO_ECHO;

// Última tabela recebida da central (escrita no callback do ESP-NOW).
StateEntry table[MAX_SPOTS];
uint8_t  tableTotal = 0;
uint32_t lastState = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

WebServer server(80);
DNSServer dns;

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

void updateOccupancy(uint16_t d) {
  bool next = occupied ? !(d > OCC_OFF) : (d < OCC_ON);
  if (next == occupied) { pending = false; return; }
  if (!pending) { pending = true; pendingSince = millis(); }
  else if (millis() - pendingSince >= DEBOUNCE_MS) {
    occupied = next; pending = false; changed = true;
  }
}

// ---------- ESP-NOW ----------
void sendMsg(uint8_t type) {
  Msg m = { type, PROTO, seq++, dist, occupied, {0}, {0} };
  memcpy(m.id, myId, 4);
  memcpy(m.mac, myMac, 6);
  esp_now_send(BCAST, (uint8_t*)&m, sizeof(m));
}

// Arduino-ESP32 3.x. Na 2.x, a assinatura é (const uint8_t* mac, const uint8_t* data, int len).
void onRecv(const esp_now_recv_info_t*, const uint8_t* data, int len) {
  if (len < (int)offsetof(StateMsg, e)) return;
  StateMsg m = {}; memcpy(&m, data, min<int>(len, sizeof(m)));
  if (m.type != MSG_STATE || m.version != PROTO || m.count > STATE_MAX) return;
  if (len != (int)(offsetof(StateMsg, e) + m.count * sizeof(StateEntry))) return;
  if (m.total > MAX_SPOTS || m.offset + m.count > m.total) return;
  portENTER_CRITICAL(&mux);
  memcpy(table + m.offset, m.e, m.count * sizeof(StateEntry));
  tableTotal = m.total;
  lastState = millis();
  portEXIT_CRITICAL(&mux);
}

// ---------- Página ----------
const char* ESTADOS[] = { "livre", "ocupada", "sem_sinal" };

// Formato descrito no topo de html/js/conexao.js.
void sendEstado() {
  StateEntry list[MAX_SPOTS + 1]; uint8_t n = 0;
  portENTER_CRITICAL(&mux);
  if (millis() - lastState < CENTRAL_MS)
    for (uint8_t i = 0; i < tableTotal; i++)
      if (strncmp(table[i].id, myId, 4) != 0) list[n++] = table[i];
  portEXIT_CRITICAL(&mux);
  memcpy(list[n].id, myId, 4);                       // a própria vaga vem do sensor local, sempre atual
  list[n++].estado = occupied ? OCUPADA : LIVRE;

  for (int i = 1; i < n; i++) {                      // ordena por id para o mapa ficar A01, A02, ...
    StateEntry k = list[i]; int j = i - 1;
    while (j >= 0 && strncmp(list[j].id, k.id, 4) > 0) { list[j + 1] = list[j]; j--; }
    list[j + 1] = k;
  }

  String j = "{\"vagas\":[";
  for (uint8_t i = 0; i < n; i++) {
    char id[4]; memcpy(id, list[i].id, 4); id[3] = '\0';
    if (i) j += ',';
    j += "{\"id\":\""; j += id; j += "\",\"estado\":\"";
    j += ESTADOS[min<int>(list[i].estado, SEM_SINAL)]; j += "\"}";
  }
  j += "],\"manobra\":{\"vaga\":\""; j += myId; j += "\",\"frente\":";
  j += (dist == NO_ECHO) ? String("null") : String(dist / 10.0, 1);
  j += "}}";
  server.sendHeader("Access-Control-Allow-Origin", "*");  // permite testar a página aberta no PC (?esp=IP)
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

void sendFile(const char* type, const uint8_t* data, size_t len, bool gzip) {
  if (gzip) server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", gzip ? "no-cache" : "max-age=86400");
  server.send_P(200, type, (const char*)data, len);
}

// ---------- Setup e loop ----------
void setup() {
  Serial.begin(115200);
  strncpy(myId, VAGA_ID, 3); myId[3] = '\0';
  pinMode(PIN_TRIG, OUTPUT); pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED_G, OUTPUT); pinMode(PIN_LED_R, OUTPUT);

  WiFi.mode(WIFI_AP_STA);
  String ssid = String("vagai-") + myId;
  WiFi.softAP(ssid.c_str(), AP_PASS, CHANNEL);     // o AP fixa o rádio no canal do ESP-NOW
  WiFi.macAddress(myMac);
  dns.start(53, "*", WiFi.softAPIP());             // qualquer endereço leva à página

  esp_now_init();
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, BCAST, 6); p.channel = CHANNEL; p.ifidx = WIFI_IF_AP; p.encrypt = false;
  esp_now_add_peer(&p);

  server.on("/", [] { sendFile("text/html; charset=utf-8", INDEX_HTML_GZ, INDEX_HTML_GZ_LEN, true); });
  server.on("/car-hero.webp", [] { sendFile("image/webp", CAR_HERO_WEBP, CAR_HERO_WEBP_LEN, false); });
  server.on("/car-top.webp", [] { sendFile("image/webp", CAR_TOP_WEBP, CAR_TOP_WEBP_LEN, false); });
  server.on("/api/estado", sendEstado);
  server.onNotFound([] {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  });
  server.begin();

  Serial.printf("Rede \"%s\" criada. Abra http://%s\n", ssid.c_str(), WiFi.softAPIP().toString().c_str());
  sendMsg(MSG_HELLO);
}

void loop() {
  dns.processNextRequest();
  server.handleClient();

  uint32_t now = millis();
  if (now - lastPing >= PING_MS) {
    lastPing = now;
    win[wi] = readOnce(); wi = (wi + 1) % 5;
    dist = median5();
    updateOccupancy(dist);
    digitalWrite(PIN_LED_R, occupied);
    digitalWrite(PIN_LED_G, !occupied);
  }
  if (changed || now - lastSend >= SEND_MS) {
    sendMsg(MSG_DATA); lastSend = now; changed = false;
  }
  if (now - lastHello >= 30000) { sendMsg(MSG_HELLO); lastHello = now; }
}
