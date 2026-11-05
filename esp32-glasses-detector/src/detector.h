// BLE 스캔 + 스마트글래스 추적 엔진
#pragma once
#include <Arduino.h>

#include "signatures.h"

// 광고 패턴으로 추정한 사용 상태 (추정값이며 확정이 아님)
enum UsageState : uint8_t {
  USAGE_UNKNOWN = 0,
  USAGE_PAIRING,    // 빠르고 연결 가능한 광고: 폰과 연결되지 않았거나 페어링 대기 중
  USAGE_CONNECTED,  // 느리거나 연결 불가능한 광고: 이미 폰과 연결되어 사용 중일 가능성이 높음
};

struct Target {
  bool used;
  uint16_t id;          // 주소가 바뀌어도 유지되는 고유 번호
  char address[18];
  char name[24];
  const char* vendor;   // signatures.h의 정적 문자열
  uint16_t companyId;
  uint32_t fingerprint;
  Confidence confidence;

  int8_t lastRssi;
  float emaRssi;        // 평활화한 RSSI
  float hist[8];        // 0.5초 간격 RSSI 기록 (접근/멀어짐 판단용)
  uint8_t histIdx;
  uint8_t histCount;
  uint32_t lastHistMs;

  float ivlMs;          // 광고 간격 추정치
  bool lastConnectable;
  bool lastLegacy;
  uint8_t addrChanges;  // 주소 랜덤화를 따라간 횟수

  uint32_t firstSeen;
  uint32_t lastSeen;
  uint32_t lastAlert;
  uint32_t packets;
};

void detectorBegin();
// 큐에 쌓인 관측값을 처리합니다. 새로 알려야 할 대상이 생기면 onAlert를 호출합니다.
void detectorProcess(void (*onAlert)(const Target&));
// 현재 기억 중인 대상들을 RSSI가 강한 순서로 복사합니다.
size_t detectorSnapshot(Target* out, size_t max);
bool detectorGet(uint16_t id, Target& out);
uint32_t detectorPacketsPerSec();

UsageState usageState(const Target& t);
float trendDb(const Target& t);           // 양수면 가까워지는 중
const char* proximityLabel(float rssi);   // 대략적인 거리
const char* usageLabel(UsageState s);
const char* confidenceLabel(Confidence c);
