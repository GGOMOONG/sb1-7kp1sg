// ESP32 스마트글래스 BLE 탐지기
//
// 주변 BLE 광고 패킷을 계속 스캔하면서, 스마트글래스 제조사 Company ID 또는
// 기기 이름 키워드와 일치하는 기기만 골라 시리얼/LED/부저로 알려줍니다.
//
// 라이브러리: NimBLE-Arduino 2.x (기본 Arduino BLE보다 메모리를 훨씬 적게 씀)

#include <Arduino.h>
#include <NimBLEDevice.h>

#include <cctype>
#include <cstring>
#include <string>

#include "signatures.h"

// ---------------- 설정 ----------------
static const int LED_PIN = 2;             // 대부분의 ESP32 DevKit 내장 LED. 없으면 -1
static const int BUZZER_PIN = -1;         // 액티브 부저 연결 시 GPIO 번호 (예: 25)
static const int RSSI_THRESHOLD = -90;    // 이보다 약한 신호는 무시 (dBm)
static const Confidence MIN_ALERT_CONFIDENCE = CONF_MEDIUM;  // 알림을 울릴 최소 신뢰도
static const uint32_t REALERT_MS = 30000; // 같은 기기를 다시 알릴 최소 간격
static const uint32_t FORGET_MS = 60000;  // 이 시간 동안 안 보이면 목록에서 제거
static const uint32_t SUMMARY_MS = 5000;  // 주변 탐지 요약 출력 주기
static const bool RAW_DUMP = false;       // true면 모든 광고 패킷을 출력 (시그니처 수집용)
// --------------------------------------

struct Detection {
  char address[18];
  char name[32];
  char vendor[40];
  uint16_t companyId;
  int8_t rssi;
  Confidence confidence;
};

struct TrackedDevice {
  bool used;
  char address[18];
  char vendor[40];
  int8_t rssi;
  Confidence confidence;
  uint32_t lastSeen;
  uint32_t lastAlert;
};

static const size_t MAX_TRACKED = 32;
static TrackedDevice tracked[MAX_TRACKED];
static QueueHandle_t detectionQueue;

static bool containsIgnoreCase(const std::string& haystack, const char* needle) {
  size_t n = strlen(needle);
  if (n == 0 || haystack.size() < n) return false;
  for (size_t i = 0; i + n <= haystack.size(); i++) {
    size_t j = 0;
    while (j < n && std::tolower((unsigned char)haystack[i + j]) == std::tolower((unsigned char)needle[j])) j++;
    if (j == n) return true;
  }
  return false;
}

static const char* proximityLabel(int rssi) {
  if (rssi >= -55) return "매우 가까움(~1m)";
  if (rssi >= -70) return "가까움(~3m)";
  if (rssi >= -80) return "근처(~10m)";
  return "먼 거리";
}

static const char* confidenceLabel(Confidence c) {
  switch (c) {
    case CONF_HIGH: return "HIGH";
    case CONF_MEDIUM: return "MEDIUM";
    case CONF_LOW: return "LOW";
    default: return "-";
  }
}

// 광고 패킷 하나를 시그니처와 비교. BLE 호스트 태스크에서 호출되므로 가볍게 유지.
static bool classify(const NimBLEAdvertisedDevice* dev, Detection& out) {
  Confidence conf = CONF_NONE;
  const char* vendor = nullptr;
  uint16_t companyId = 0xFFFF;

  if (dev->haveManufacturerData()) {
    std::string mfr = dev->getManufacturerData();
    if (mfr.size() >= 2) {
      companyId = (uint8_t)mfr[0] | ((uint8_t)mfr[1] << 8);  // little-endian
      for (size_t i = 0; i < COMPANY_SIGNATURE_COUNT; i++) {
        if (COMPANY_SIGNATURES[i].companyId == companyId) {
          conf = COMPANY_SIGNATURES[i].confidence;
          vendor = COMPANY_SIGNATURES[i].vendor;
          break;
        }
      }
    }
  }

  std::string name = dev->haveName() ? dev->getName() : std::string();
  if (!name.empty()) {
    for (size_t i = 0; i < NAME_SIGNATURE_COUNT; i++) {
      if (containsIgnoreCase(name, NAME_SIGNATURES[i].keyword)) {
        if (conf == CONF_NONE) {
          conf = CONF_LOW;
          vendor = NAME_SIGNATURES[i].vendor;
        } else {
          conf = CONF_HIGH;  // 제조사 ID + 이름이 모두 일치
        }
        break;
      }
    }
  }

  if (conf == CONF_NONE) return false;

  memset(&out, 0, sizeof(out));
  strncpy(out.address, dev->getAddress().toString().c_str(), sizeof(out.address) - 1);
  strncpy(out.name, name.c_str(), sizeof(out.name) - 1);
  strncpy(out.vendor, vendor ? vendor : "?", sizeof(out.vendor) - 1);
  out.companyId = companyId;
  out.rssi = (int8_t)dev->getRSSI();
  out.confidence = conf;
  return true;
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    if (RAW_DUMP) {
      Serial.printf("[RAW] %s\n", dev->toString().c_str());
    }
    if (dev->getRSSI() < RSSI_THRESHOLD) return;
    Detection d;
    if (classify(dev, d)) {
      xQueueSend(detectionQueue, &d, 0);  // 큐가 가득 차면 그냥 버림
    }
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    NimBLEDevice::getScan()->start(0, false, true);  // 혹시 멈추면 재시작
  }
};

static void alert(Confidence conf) {
  int beeps = (conf == CONF_HIGH) ? 3 : 1;
  for (int i = 0; i < beeps; i++) {
    if (LED_PIN >= 0) digitalWrite(LED_PIN, HIGH);
    if (BUZZER_PIN >= 0) digitalWrite(BUZZER_PIN, HIGH);
    delay(120);
    if (LED_PIN >= 0) digitalWrite(LED_PIN, LOW);
    if (BUZZER_PIN >= 0) digitalWrite(BUZZER_PIN, LOW);
    delay(120);
  }
}

static TrackedDevice* findOrCreate(const char* address, uint32_t now) {
  TrackedDevice* freeSlot = nullptr;
  TrackedDevice* oldest = &tracked[0];
  for (size_t i = 0; i < MAX_TRACKED; i++) {
    if (tracked[i].used && strcmp(tracked[i].address, address) == 0) return &tracked[i];
    if (!tracked[i].used && !freeSlot) freeSlot = &tracked[i];
    if (tracked[i].lastSeen < oldest->lastSeen) oldest = &tracked[i];
  }
  TrackedDevice* slot = freeSlot ? freeSlot : oldest;
  memset(slot, 0, sizeof(*slot));
  slot->used = true;
  strncpy(slot->address, address, sizeof(slot->address) - 1);
  return slot;
}

static void handleDetection(const Detection& d) {
  uint32_t now = millis();
  TrackedDevice* t = findOrCreate(d.address, now);
  bool isNew = (t->lastSeen == 0);
  t->lastSeen = now;
  t->rssi = d.rssi;
  if (d.confidence > t->confidence) t->confidence = d.confidence;
  strncpy(t->vendor, d.vendor, sizeof(t->vendor) - 1);

  if (t->confidence < MIN_ALERT_CONFIDENCE) return;
  if (!isNew && t->lastAlert != 0 && now - t->lastAlert < REALERT_MS) return;
  t->lastAlert = now;

  Serial.println();
  Serial.println("========== 스마트글래스 감지 ==========");
  Serial.printf(" 제조사/모델 : %s\n", d.vendor);
  Serial.printf(" 신뢰도      : %s\n", confidenceLabel(t->confidence));
  Serial.printf(" 신호 세기   : %d dBm (%s)\n", d.rssi, proximityLabel(d.rssi));
  Serial.printf(" BLE 주소    : %s\n", d.address);
  if (d.name[0]) Serial.printf(" 기기 이름   : %s\n", d.name);
  if (d.companyId != 0xFFFF) Serial.printf(" Company ID  : 0x%04X\n", d.companyId);
  Serial.println("========================================");
  alert(t->confidence);
}

static void printSummary() {
  uint32_t now = millis();
  int count = 0;
  for (size_t i = 0; i < MAX_TRACKED; i++) {
    TrackedDevice& t = tracked[i];
    if (!t.used) continue;
    if (now - t.lastSeen > FORGET_MS) {
      t.used = false;
      continue;
    }
    if (t.confidence < MIN_ALERT_CONFIDENCE) continue;
    if (count == 0) Serial.println("[주변] 현재 감지 중인 스마트글래스:");
    count++;
    Serial.printf("  - %-32s %4d dBm  %-18s %s (%lus 전)\n", t.vendor, t.rssi, proximityLabel(t.rssi),
                  confidenceLabel(t.confidence), (unsigned long)((now - t.lastSeen) / 1000));
  }
  if (count == 0) Serial.println("[주변] 감지된 스마트글래스 없음");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  if (LED_PIN >= 0) pinMode(LED_PIN, OUTPUT);
  if (BUZZER_PIN >= 0) pinMode(BUZZER_PIN, OUTPUT);

  detectionQueue = xQueueCreate(32, sizeof(Detection));

  NimBLEDevice::init("");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new ScanCallbacks(), true);  // true: 같은 기기 반복 수신 허용 (RSSI 갱신용)
  scan->setActiveScan(true);  // Scan Response까지 받아야 기기 이름이 보이는 경우가 많음
  scan->setInterval(100);
  scan->setWindow(99);
  scan->setMaxResults(0);  // 결과를 메모리에 쌓지 않음 (콜백만 사용)
  scan->start(0, false, true);  // 0 = 무한 스캔

  Serial.println("스마트글래스 BLE 탐지기 시작 - 스캔 중...");
}

void loop() {
  static uint32_t lastSummary = 0;
  Detection d;
  while (xQueueReceive(detectionQueue, &d, pdMS_TO_TICKS(100)) == pdTRUE) {
    handleDetection(d);
  }
  if (millis() - lastSummary >= SUMMARY_MS) {
    lastSummary = millis();
    printSummary();
  }
}
