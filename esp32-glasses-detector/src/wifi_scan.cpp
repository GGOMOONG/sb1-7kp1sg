#include "wifi_scan.h"

#include "config.h"

#if ENABLE_WIFI_SCAN

#include <WiFi.h>
#include <cctype>
#include <cstring>

namespace {

// ── 카메라형 SSID 키워드 (소문자, 부분일치) ──
// 싸구려/스파이 카메라와 앱이 만드는 핫스팟 이름에서 자주 보이는 패턴입니다.
// 완벽한 목록은 없습니다. 실제로 돌려보며 여기에 추가하세요(오탐도 있을 수 있음).
const char* SSID_KEYWORDS[] = {
    "cam", "ipc", "ipcam", "wificam", "hdcam", "minicam", "spycam", "camera",
    "dvr", "nvr", "dv", "hd ", "p2p", "v380", "yoosee", "icsee", "eye4",
    "escam", "anran", "sricam", "wanscam", "a9", "sq", "mini", "spy",
    "gopro", "insta360", "dji", "glass",  // 웨어러블 카메라 브랜드 포함
};
const size_t SSID_KEYWORD_COUNT = sizeof(SSID_KEYWORDS) / sizeof(SSID_KEYWORDS[0]);

// ── 카메라 모듈이 자주 쓰는 OUI(BSSID 앞 3바이트) ──
// 비워 두는 게 안전합니다(오탐 큼). 특정 기기 OUI를 알아내면 추가하세요.
// 예: {0x00,0x12,0x34}. ESP/일반 공유기 OUI는 절대 넣지 마세요(모든 걸 오탐).
const uint8_t SUSPECT_OUI[][3] = {
    // {0xAA,0xBB,0xCC},
};
const size_t SUSPECT_OUI_COUNT = sizeof(SUSPECT_OUI) / sizeof(SUSPECT_OUI[0]);

const size_t MAX_HITS = 24;
WifiHit hits[MAX_HITS];
int scanState = 0;  // 0=idle, 1=scanning
uint32_t lastScanMs = 0;
const uint32_t FORGET_MS = 90000;

void toLowerCopy(char* dst, const char* src, size_t n) {
  size_t i = 0;
  for (; src[i] && i < n - 1; i++) dst[i] = (char)std::tolower((unsigned char)src[i]);
  dst[i] = 0;
}

bool matchKeyword(const char* ssidLower, const char*& why) {
  for (size_t k = 0; k < SSID_KEYWORD_COUNT; k++)
    if (strstr(ssidLower, SSID_KEYWORDS[k])) {
      why = SSID_KEYWORDS[k];
      return true;
    }
  return false;
}

bool matchOui(const uint8_t* bssid) {
  for (size_t i = 0; i < SUSPECT_OUI_COUNT; i++)
    if (memcmp(bssid, SUSPECT_OUI[i], 3) == 0) return true;
  return false;
}

WifiHit* findByBssid(const uint8_t* b) {
  for (auto& h : hits)
    if (h.lastSeen && memcmp(h.bssid, b, 6) == 0) return &h;
  return nullptr;
}
WifiHit* allocHit() {
  WifiHit* oldest = &hits[0];
  for (auto& h : hits) {
    if (!h.lastSeen) return &h;
    if (h.lastSeen < oldest->lastSeen) oldest = &h;
  }
  return oldest;
}

void collect(int count) {
  uint32_t now = millis();
  for (int i = 0; i < count; i++) {
    String ssid = WiFi.SSID(i);
    uint8_t* bssid = WiFi.BSSID(i);
    if (!bssid) continue;
    char low[40];
    toLowerCopy(low, ssid.c_str(), sizeof(low));
    const char* why = nullptr;
    bool suspect = matchKeyword(low, why);
    if (!suspect && matchOui(bssid)) { suspect = true; why = "OUI"; }

    WifiHit* h = findByBssid(bssid);
    if (!h) h = allocHit();
    memset(h, 0, sizeof(*h));
    strncpy(h->ssid, ssid.length() ? ssid.c_str() : "(hidden)", sizeof(h->ssid) - 1);
    memcpy(h->bssid, bssid, 6);
    h->rssi = (int8_t)WiFi.RSSI(i);
    h->channel = (uint8_t)WiFi.channel(i);
    h->suspect = suspect;
    h->why = why ? why : "";
    h->lastSeen = now;

    if (suspect)
      Serial.printf("WIFI,%lu,%02X:%02X:%02X:%02X:%02X:%02X,%d,%d,%s,%s\n", (unsigned long)now, bssid[0], bssid[1],
                    bssid[2], bssid[3], bssid[4], bssid[5], h->rssi, h->channel, h->why, h->ssid);
  }
}

void expire(uint32_t now) {
  for (auto& h : hits)
    if (h.lastSeen && now - h.lastSeen > FORGET_MS) h.lastSeen = 0;
}

}  // namespace

void wifiBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  lastScanMs = millis() - WIFI_SCAN_INTERVAL_MS + 2000;  // 시작 2초 후 첫 스캔
}

void wifiProcess() {
  uint32_t now = millis();
  expire(now);
  if (scanState == 0) {
    if (now - lastScanMs >= WIFI_SCAN_INTERVAL_MS) {
      WiFi.scanNetworks(true /*async*/, true /*hidden 포함*/);
      scanState = 1;
    }
  } else {
    int c = WiFi.scanComplete();
    if (c >= 0) {
      collect(c);
      WiFi.scanDelete();
      scanState = 0;
      lastScanMs = now;
    } else if (c == WIFI_SCAN_FAILED) {
      scanState = 0;
      lastScanMs = now;
    }
  }
}

size_t wifiSuspectCount() {
  size_t n = 0;
  for (auto& h : hits)
    if (h.lastSeen && h.suspect) n++;
  return n;
}

size_t wifiSnapshot(WifiHit* out, size_t max) {
  size_t n = 0;
  for (auto& h : hits)
    if (h.lastSeen && h.suspect && n < max) out[n++] = h;
  for (size_t i = 1; i < n; i++) {  // RSSI 강한 순
    WifiHit key = out[i];
    size_t j = i;
    while (j > 0 && out[j - 1].rssi < key.rssi) { out[j] = out[j - 1]; j--; }
    out[j] = key;
  }
  return n;
}

const char* wifiTopSuspect() {
  WifiHit* best = nullptr;
  for (auto& h : hits)
    if (h.lastSeen && h.suspect && (!best || h.rssi > best->rssi)) best = &h;
  return best ? best->ssid : nullptr;
}

#else  // ENABLE_WIFI_SCAN == 0
void wifiBegin() {}
void wifiProcess() {}
size_t wifiSuspectCount() { return 0; }
size_t wifiSnapshot(WifiHit*, size_t) { return 0; }
const char* wifiTopSuspect() { return nullptr; }
#endif
