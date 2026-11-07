#include "detector.h"

#include <NimBLEDevice.h>

#include <cctype>
#include <cstring>
#include <string>

#include "config.h"

namespace {

// BLE 콜백(호스트 태스크)에서 메인 루프로 넘기는 관측값
struct Observation {
  char address[18];
  char name[24];
  const char* vendor;
  uint16_t companyId;
  uint32_t fingerprint;
  int8_t rssi;
  Confidence confidence;
  Category category;
  bool connectable;
  bool legacy;
  uint32_t ms;
};

const size_t MAX_TARGETS = 24;
Target targets[MAX_TARGETS];
uint16_t nextId = 1;
QueueHandle_t queue;

volatile uint32_t totalPackets = 0;
uint32_t ppsLastCount = 0;
uint32_t ppsLastMs = 0;
uint32_t ppsValue = 0;

bool containsIgnoreCase(const std::string& haystack, const char* needle) {
  size_t n = strlen(needle);
  if (n == 0 || haystack.size() < n) return false;
  for (size_t i = 0; i + n <= haystack.size(); i++) {
    size_t j = 0;
    while (j < n && std::tolower((unsigned char)haystack[i + j]) == std::tolower((unsigned char)needle[j])) j++;
    if (j == n) return true;
  }
  return false;
}

uint32_t fnv1a(uint32_t h, const void* data, size_t len) {
  const uint8_t* p = (const uint8_t*)data;
  for (size_t i = 0; i < len; i++) {
    h ^= p[i];
    h *= 16777619u;
  }
  return h;
}

bool classify(const NimBLEAdvertisedDevice* dev, Observation& out) {
  Confidence conf = CONF_NONE;
  const char* vendor = nullptr;
  Category cat = CAT_NONE;
  uint16_t companyId = 0xFFFF;
  size_t mfrLen = 0;
  bool haveMfr = false;
  uint16_t firstCid = 0xFFFF;

  // 한 패킷에 Manufacturer Data가 여러 개 들어 있을 수 있어서 모두 확인합니다
  uint8_t mfrCount = dev->getManufacturerDataCount();
  for (uint8_t m = 0; m < mfrCount && cat != CAT_GLASSES; m++) {
    std::string mfr = dev->getManufacturerData(m);
    if (mfr.size() < 2) continue;
    uint16_t cid = (uint8_t)mfr[0] | ((uint8_t)mfr[1] << 8);
    if (!haveMfr) { haveMfr = true; firstCid = cid; mfrLen = mfr.size(); }
    for (size_t i = 0; i < COMPANY_SIGNATURE_COUNT; i++) {  // ① 안경 화이트리스트
      if (COMPANY_SIGNATURES[i].companyId == cid) {
        conf = COMPANY_SIGNATURES[i].confidence;
        vendor = COMPANY_SIGNATURES[i].vendor;
        cat = CAT_GLASSES;
        companyId = cid;
        mfrLen = mfr.size();
        break;
      }
    }
  }

  // ② 이름 키워드 (안경 이름이면 새로 잡거나 신뢰도 상향)
  std::string name = dev->haveName() ? dev->getName() : std::string();
  if (!name.empty()) {
    for (size_t i = 0; i < NAME_SIGNATURE_COUNT; i++) {
      if (containsIgnoreCase(name, NAME_SIGNATURES[i].keyword)) {
        if (cat != CAT_GLASSES) {
          conf = NAME_SIGNATURES[i].confidence;
          vendor = NAME_SIGNATURES[i].vendor;
          cat = CAT_GLASSES;
        } else {
          conf = CONF_HIGH;
        }
        break;
      }
    }
  }

  // ③ 안경이 아니면 블랙리스트(무시) / 미확인 칩 벤더(의심) 판정
  if (cat != CAT_GLASSES && haveMfr) {
    for (size_t i = 0; i < DENY_COMPANY_COUNT; i++)
      if (DENY_COMPANIES[i] == firstCid) return false;  // 이어폰/워치/폰 등 → 무시
    for (size_t i = 0; i < SUSPECT_COMPANY_COUNT; i++) {
      if (SUSPECT_COMPANIES[i].companyId == firstCid) {
        // 연결 가능한 것만 (무작위 비콘 노이즈 제거). 싸구려 카메라 제어채널은 보통 연결형.
        if (!dev->isConnectable()) return false;
        cat = CAT_UNKNOWN;
        conf = SUSPECT_COMPANIES[i].confidence;
        vendor = SUSPECT_COMPANIES[i].vendor;
        companyId = firstCid;
        break;
      }
    }
  }

  if (cat == CAT_NONE) return false;
#if !SHOW_UNKNOWN
  if (cat == CAT_UNKNOWN) return false;
#endif

  memset(&out, 0, sizeof(out));
  strncpy(out.address, dev->getAddress().toString().c_str(), sizeof(out.address) - 1);
  strncpy(out.name, name.c_str(), sizeof(out.name) - 1);
  out.vendor = vendor;
  out.companyId = companyId;
  out.rssi = (int8_t)dev->getRSSI();
  out.confidence = conf;
  out.category = cat;
  out.connectable = dev->isConnectable();
  out.legacy = dev->isLegacyAdvertisement();
  out.ms = millis();

  // 주소가 랜덤으로 바뀌어도 같은 기기를 이어서 추적하기 위한 지문.
  // 패킷마다 변하는 값(이름, RSSI 등)은 넣지 않습니다.
  uint32_t h = 2166136261u;
  h = fnv1a(h, &companyId, sizeof(companyId));
  h = fnv1a(h, &mfrLen, sizeof(mfrLen));
  h = fnv1a(h, vendor, strlen(vendor));
  out.fingerprint = h;
  return true;
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    totalPackets++;
    if (RAW_DUMP) Serial.printf("[RAW] %s\n", dev->toString().c_str());
    if (dev->getRSSI() < RSSI_THRESHOLD) return;
    Observation o;
    if (classify(dev, o)) xQueueSend(queue, &o, 0);
  }

  void onScanEnd(const NimBLEScanResults&, int) override {
    NimBLEDevice::getScan()->start(0, false, true);
  }
};

Target* findByAddress(const char* addr) {
  for (auto& t : targets)
    if (t.used && strcmp(t.address, addr) == 0) return &t;
  return nullptr;
}

// 주소가 바뀐 같은 기기 찾기: 같은 지문이고, 직전에 사라졌고, 너무 오래되지 않은 대상
Target* findRotated(const Observation& o) {
  Target* best = nullptr;
  for (auto& t : targets) {
    if (!t.used || t.fingerprint != o.fingerprint) continue;
    uint32_t gap = o.ms - t.lastSeen;
    if (gap < 3000 || gap > 90000) continue;
    if (!best || t.lastSeen > best->lastSeen) best = &t;
  }
  return best;
}

Target* allocate(uint32_t now) {
  Target* oldest = nullptr;
  for (auto& t : targets) {
    if (!t.used) return &t;
    if (!oldest || t.lastSeen < oldest->lastSeen) oldest = &t;
  }
  return oldest;  // 꽉 차면 가장 오래 안 보인 대상을 덮어씁니다
}

void ingest(const Observation& o, void (*onAlert)(const Target&)) {
  Target* t = findByAddress(o.address);
  bool rotated = false;
  if (!t) {
    t = findRotated(o);
    if (t) {
      rotated = true;
      strncpy(t->address, o.address, sizeof(t->address) - 1);
      t->addrChanges++;
    }
  }
  bool isNew = false;
  if (!t) {
    t = allocate(o.ms);
    memset(t, 0, sizeof(*t));
    t->used = true;
    t->id = nextId++;
    strncpy(t->address, o.address, sizeof(t->address) - 1);
    t->fingerprint = o.fingerprint;
    t->firstSeen = o.ms;
    t->emaRssi = o.rssi;
    isNew = true;
  }

  uint32_t prevSeen = t->lastSeen;
  // 광고 간격 추정. 20ms 미만은 같은 광고의 Scan Response라서 제외합니다.
  if (!isNew) {
    uint32_t delta = o.ms - prevSeen;
    if (!rotated && delta >= 15 && delta < 30000) {
      t->ivlMs = (t->ivlMs <= 0) ? delta : t->ivlMs * 0.85f + delta * 0.15f;
    }
    t->emaRssi = t->emaRssi * 0.7f + o.rssi * 0.3f;
  }

  t->lastSeen = o.ms;
  t->lastRssi = o.rssi;
  t->packets++;
  t->vendor = o.vendor;
  t->companyId = (o.companyId != 0xFFFF) ? o.companyId : t->companyId;
  t->lastConnectable = o.connectable;
  t->lastLegacy = o.legacy;
  if (o.name[0]) strncpy(t->name, o.name, sizeof(t->name) - 1);
  if (o.confidence > t->confidence) t->confidence = o.confidence;
  if (o.category > t->category) t->category = o.category;  // UNKNOWN→GLASSES 승격 허용

  if (LOG_OBSERVATIONS) {
    // CSV: OBS,시간ms,id,주소,companyId,rssi,connectable,legacy,이름
    Serial.printf("OBS,%lu,%u,%s,0x%04X,%d,%d,%d,%s\n", (unsigned long)o.ms, t->id, o.address, o.companyId, o.rssi,
                  o.connectable, o.legacy, o.name);
  }

  // 처음 발견했을 때, 또는 한동안 안 보이다가 다시 나타났을 때만 알립니다
  // (계속 근처에 있는 대상 때문에 계속 울리지 않도록)
  bool returned = !isNew && (o.ms - prevSeen >= REALERT_MS) && (o.ms - t->lastAlert >= REALERT_MS);
  if (t->confidence >= CONF_MEDIUM && (t->lastAlert == 0 || returned)) {
    t->lastAlert = o.ms;
    onAlert(*t);
  }
}

void sampleHistory(uint32_t now) {
  for (auto& t : targets) {
    if (!t.used) continue;
    if (now - t.lastSeen > FORGET_MS) {
      t.used = false;
      continue;
    }
    if (now - t.lastHistMs >= 500) {
      t.lastHistMs = now;
      t.hist[t.histIdx] = t.emaRssi;
      t.histIdx = (t.histIdx + 1) % 8;
      if (t.histCount < 8) t.histCount++;
    }
  }
}

}  // namespace

void detectorBegin() {
  queue = xQueueCreate(48, sizeof(Observation));
  NimBLEDevice::init("");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new ScanCallbacks(), true);  // 중복 허용: 모든 패킷을 받아야 RSSI/간격 추적 가능
  scan->setActiveScan(true);
  // 듀티 100%: 연결된 안경은 광고가 몇 초에 한 번뿐이라 한 패킷도 놓치지 않는 게 중요합니다
  scan->setInterval(100);
  scan->setWindow(100);
  scan->setMaxResults(0);
#if CONFIG_BT_NIMBLE_EXT_ADV
  // BLE 5 확장 광고 + Coded PHY(장거리)까지 수신. ESP32-S3/C3에서만 가능
  scan->setPhy(NimBLEScan::Phy::SCAN_ALL);
#endif
  scan->start(0, false, true);
}

void detectorProcess(void (*onAlert)(const Target&)) {
  Observation o;
  while (xQueueReceive(queue, &o, 0) == pdTRUE) ingest(o, onAlert);
  uint32_t now = millis();
  sampleHistory(now);
  if (now - ppsLastMs >= 1000) {
    uint32_t c = totalPackets;
    ppsValue = (c - ppsLastCount) * 1000 / (now - ppsLastMs);
    ppsLastCount = c;
    ppsLastMs = now;
  }
}

size_t detectorSnapshot(Target* out, size_t max) {
  size_t n = 0;
  for (auto& t : targets) {
    if (!t.used) continue;
    // 안경(확신 MEDIUM 이상) + 미확인(의심) 기기를 표시
    bool show = (t.category == CAT_GLASSES && t.confidence >= CONF_MEDIUM) || t.category == CAT_UNKNOWN;
    if (!show) continue;
    if (n < max) out[n++] = t;
  }
  // 정렬: 안경 먼저 → 최근에 보인 것 → 신호 강한 순
  uint32_t now = millis();
  for (size_t i = 1; i < n; i++) {
    Target key = out[i];
    bool keyActive = now - key.lastSeen < ACTIVE_MS;
    size_t j = i;
    while (j > 0) {
      Target& p = out[j - 1];
      bool prevActive = now - p.lastSeen < ACTIVE_MS;
      bool keyFirst;
      if (key.category != p.category) {
        keyFirst = (key.category == CAT_GLASSES);  // 안경을 미확인보다 위로
      } else if (keyActive != prevActive) {
        keyFirst = keyActive;
      } else {
        keyFirst = key.emaRssi > p.emaRssi;
      }
      if (!keyFirst) break;
      out[j] = out[j - 1];
      j--;
    }
    out[j] = key;
  }
  return n;
}

bool detectorGet(uint16_t id, Target& out) {
  for (auto& t : targets) {
    if (t.used && t.id == id) {
      out = t;
      return true;
    }
  }
  return false;
}

uint32_t detectorPacketsPerSec() { return ppsValue; }

UsageState usageState(const Target& t) {
  if (t.packets < 3) return t.lastConnectable ? USAGE_UNKNOWN : USAGE_CONNECTED;
  // 연결 안 된 안경은 폰이 빨리 찾도록 수십~수백 ms 간격으로 "연결 가능" 광고를 보냅니다.
  // 폰과 연결되면 광고를 멈추거나, 1초 이상 간격으로 줄이거나, 연결 불가능 광고로 바꾸는 경우가 많습니다.
  if (!t.lastConnectable) return USAGE_CONNECTED;
  if (t.ivlMs >= 900) return USAGE_CONNECTED;
  if (t.ivlMs > 0 && t.ivlMs < 400) return USAGE_PAIRING;
  return USAGE_UNKNOWN;
}

float trendDb(const Target& t) {
  if (t.histCount < 4) return 0;
  uint8_t oldest = (t.histIdx + 8 - t.histCount) % 8;
  return t.emaRssi - t.hist[oldest];
}

const char* proximityLabel(float rssi) {
  if (rssi >= -55) return "~1m";
  if (rssi >= -67) return "~3m";
  if (rssi >= -78) return "~10m";
  if (rssi >= -88) return "~20m";
  return ">20m";
}

const char* usageLabel(UsageState s) {
  switch (s) {
    case USAGE_PAIRING: return "IDLE";
    case USAGE_CONNECTED: return "IN USE?";
    default: return "?";
  }
}

const char* confidenceLabel(Confidence c) {
  switch (c) {
    case CONF_HIGH: return "HIGH";
    case CONF_MEDIUM: return "MED";
    case CONF_LOW: return "LOW";
    default: return "-";
  }
}
