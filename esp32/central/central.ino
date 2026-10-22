#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "pagina.h"   // gerado de html/ por: uv run esp32/gerar_pagina.py

// Recebe por ESP-NOW o estado de cada car, cria a rede Wi-Fi do estacionamento
// e serve a página e a API para o celular do motorista.

const char*    AP_SSID = "vagai";
const char*    AP_PASS = "estaciona";   // mínimo de 8 caracteres
const uint8_t  CHANNEL = 1;             // igual ao dos cars
const uint8_t  MAX_SPOTS = 64;          // capacidade da tabela
const uint32_t OFFLINE_MS = 5000;
const uint16_t NO_ECHO = 0xFFFF;
const uint8_t  PROTO = 2;

enum : uint8_t { MSG_HELLO = 1, MSG_DATA = 2 };
struct __attribute__((packed)) Msg {   // car -> central (idêntica à do car)
  uint8_t type, version; uint16_t seq, dist10; uint8_t occupied; char id[4]; uint8_t mac[6];
};

struct Spot {
  bool used; uint8_t mac[6]; char id[4];
  uint16_t dist10; bool occupied;
  uint32_t lastSeen;
};
Spot spots[MAX_SPOTS];
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

WebServer server(80);
DNSServer dns;

// ---------- Tabela de vagas ----------
// Chamar dentro da seção crítica.
int findOrCreate(const uint8_t* mac) {
  int freeIdx = -1;
  for (int i = 0; i < MAX_SPOTS; i++) {
    if (spots[i].used && memcmp(spots[i].mac, mac, 6) == 0) return i;
    if (!spots[i].used && freeIdx < 0) freeIdx = i;
  }
  if (freeIdx < 0) return -1;                     // tabela cheia
  Spot& s = spots[freeIdx];
  memset(&s, 0, sizeof(s));
  s.used = true; memcpy(s.mac, mac, 6); s.dist10 = NO_ECHO;
  return freeIdx;
}

// Arduino-ESP32 3.x. Na 2.x, a assinatura é (const uint8_t* mac, const uint8_t* data, int len).
void onRecv(const esp_now_recv_info_t*, const uint8_t* data, int len) {
  if (len != sizeof(Msg)) return;
  Msg m; memcpy(&m, data, sizeof(m));
  if (m.version != PROTO || (m.type != MSG_DATA && m.type != MSG_HELLO)) return;
  portENTER_CRITICAL(&mux);
  int i = findOrCreate(m.mac);
  if (i >= 0) {
    spots[i].lastSeen = millis();
    memcpy(spots[i].id, m.id, 4); spots[i].id[3] = '\0';
    if (m.type == MSG_DATA) { spots[i].dist10 = m.dist10; spots[i].occupied = m.occupied; }
  }
  portEXIT_CRITICAL(&mux);
}

// ---------- Página ----------
const char* ESTADOS[] = { "livre", "ocupada", "sem_sinal" };

// Formato descrito no topo de html/js/conexao.js. ?vaga=A01 escolhe a vaga da manobra.
void sendEstado() {
  Spot list[MAX_SPOTS]; uint8_t n = 0;
  portENTER_CRITICAL(&mux);
  uint32_t now = millis();
  for (auto& s : spots)
    if (s.used) list[n++] = s;
  portEXIT_CRITICAL(&mux);

  for (int i = 1; i < n; i++) {                      // ordena por id para o mapa ficar A01, A02, ...
    Spot k = list[i]; int j = i - 1;
    while (j >= 0 && strncmp(list[j].id, k.id, 4) > 0) { list[j + 1] = list[j]; j--; }
    list[j + 1] = k;
  }

  String vaga = server.arg("vaga");
  const Spot* manobra = nullptr;
  String j = "{\"vagas\":[";
  for (uint8_t i = 0; i < n; i++) {
    bool online = now - list[i].lastSeen < OFFLINE_MS;
    if (i) j += ',';
    j += "{\"id\":\""; j += list[i].id; j += "\",\"estado\":\"";
    j += ESTADOS[!online ? 2 : list[i].occupied ? 1 : 0]; j += "\"}";
    if (!manobra && vaga.length() && vaga == list[i].id) manobra = &list[i];
  }
  j += "],\"manobra\":";
  if (!manobra) j += "null";
  else {
    bool online = now - manobra->lastSeen < OFFLINE_MS;
    j += "{\"vaga\":\""; j += manobra->id; j += "\",\"frente\":";
    j += (!online || manobra->dist10 == NO_ECHO) ? String("null") : String(manobra->dist10 / 10.0, 1);
    j += "}";
  }
  j += "}";
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
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, CHANNEL);          // o AP fixa o rádio no canal do ESP-NOW
  dns.start(53, "*", WiFi.softAPIP());             // qualquer endereço leva à página

  esp_now_init();
  esp_now_register_recv_cb(onRecv);                // só recebe: não precisa cadastrar peers

  server.on("/", [] { sendFile("text/html; charset=utf-8", INDEX_HTML_GZ, INDEX_HTML_GZ_LEN, true); });
  server.on("/car-hero.webp", [] { sendFile("image/webp", CAR_HERO_WEBP, CAR_HERO_WEBP_LEN, false); });
  server.on("/car-top.webp", [] { sendFile("image/webp", CAR_TOP_WEBP, CAR_TOP_WEBP_LEN, false); });
  server.on("/api/estado", sendEstado);
  server.onNotFound([] {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  });
  server.begin();

  Serial.printf("Central pronta. Rede \"%s\" criada. Abra http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void loop() {
  dns.processNextRequest();
  server.handleClient();
}
