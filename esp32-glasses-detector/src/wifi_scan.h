// WiFi 카메라 AP 스캐너 (논블로킹)
// 주변 WiFi AP를 주기적으로 async 스캔해서, 카메라로 의심되는 것을 골라냅니다.
#pragma once
#include <Arduino.h>

struct WifiHit {
  char ssid[33];
  uint8_t bssid[6];
  int8_t rssi;
  uint8_t channel;
  bool suspect;
  const char* why;  // 의심 이유 (정적 문자열)
  uint32_t lastSeen;
};

void wifiBegin();
void wifiProcess();                            // loop에서 자주 호출 (블로킹 안 함)
size_t wifiSuspectCount();                     // 현재 의심 AP 수
size_t wifiSnapshot(WifiHit* out, size_t max); // 의심 AP를 신호 강한 순으로 복사
const char* wifiTopSuspect();                  // 가장 강한 의심 AP의 SSID (없으면 nullptr)
