// 128x64 OLED 화면
#pragma once
#include "detector.h"

struct UiStatus {
  int batteryPct;  // -1이면 표시 안 함
  bool muted;
  uint32_t pps;    // 초당 수신 BLE 패킷 수 (스캐너가 살아 있다는 표시)
  int wifiSuspects;      // WiFi 카메라 의심 AP 수 (-1이면 WiFi 스캔 꺼짐)
  const char* wifiTop;   // 가장 강한 의심 AP SSID (없으면 nullptr)
};

void displayBegin();
void displayList(const Target* list, size_t n, size_t selected, const UiStatus& st);
void displayTrack(const Target* t, const UiStatus& st);  // t == nullptr이면 대상을 잃어버린 상태
void displayAlert(const Target& t);
