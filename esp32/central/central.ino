#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Recebe por ESP-NOW o estado de cada car e repassa a tabela inteira para todos eles.
// Quem serve a página é o car (esp32/car/car.ino).

const uint8_t  CHANNEL = 1;           // igual ao dos cars
const uint8_t  MAX_SPOTS = 64;        // capacidade da tabela
const uint32_t OFFLINE_MS = 5000;
const uint32_t STATE_MS = 1000;       // a cada quanto a tabela é repassada
const uint16_t NO_ECHO = 0xFFFF;
const uint8_t  PROTO = 2;

enum : uint8_t { MSG_HELLO = 1, MSG_DATA = 2, MSG_STATE = 4 };
enum : uint8_t { LIVRE = 0, OCUPADA = 1, SEM_SINAL = 2 };
struct __attribute__((packed)) Msg {   // car -> central (idêntica à do car)
  uint8_t type, version; uint16_t seq, dist10; uint8_t occupied; char id[4]; uint8_t mac[6];
};
const uint8_t STATE_MAX = 40;          // entradas por pacote (ESP-NOW: até 250 bytes)
struct __attribute__((packed)) StateEntry { char id[4]; uint8_t estado; };
struct __attribute__((packed)) StateMsg {   // central -> cars (idêntica à do car)
  uint8_t type, version, total, offset, count;
  StateEntry e[STATE_MAX];
};

struct Spot {
  bool used; uint8_t mac[6]; char id[4];
  uint16_t dist10; bool occupied;
  uint32_t lastSeen;
};
Spot spots[MAX_SPOTS];
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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

// ---------- Repasse aos cars ----------
void broadcastState() {
  StateEntry all[MAX_SPOTS]; uint8_t n = 0;
  portENTER_CRITICAL(&mux);
  uint32_t now = millis();
  for (auto& s : spots)
    if (s.used) {
      memcpy(all[n].id, s.id, 4);
      all[n++].estado = now - s.lastSeen >= OFFLINE_MS ? SEM_SINAL : s.occupied ? OCUPADA : LIVRE;
    }
  portEXIT_CRITICAL(&mux);

  // Mesmo sem vagas envia um pacote vazio: assim os cars sabem que a central está viva.
  uint8_t off = 0;
  do {
    StateMsg m = { MSG_STATE, PROTO, n, off, (uint8_t)min<int>(n - off, STATE_MAX) };
    memcpy(m.e, all + off, m.count * sizeof(StateEntry));
    esp_now_send(BCAST, (uint8_t*)&m, offsetof(StateMsg, e) + m.count * sizeof(StateEntry));
    off += STATE_MAX;
    delay(5);
  } while (off < n);
}

// ---------- Setup e loop ----------
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, BCAST, 6); p.channel = CHANNEL; p.encrypt = false;
  esp_now_add_peer(&p);
  Serial.print("Central pronta. MAC "); Serial.println(WiFi.macAddress());
}

void loop() {
  static uint32_t lastState = 0;
  if (millis() - lastState >= STATE_MS) {
    lastState = millis();
    broadcastState();
  }
  delay(10);
}
