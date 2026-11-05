// 스마트글래스 BLE 시그니처 테이블
//
// 판별 근거는 두 가지입니다.
//  1) Manufacturer Specific Data 앞 2바이트의 Bluetooth SIG Company ID
//     - MAC 주소와 달리 무작위화되지 않으므로 가장 신뢰도가 높습니다.
//  2) 광고 패킷의 기기 이름(Complete/Shortened Local Name) 키워드
//     - 페어링 모드일 때 주로 보이며, 이름을 안 내보내는 기기도 많습니다.
//
// 아래 값은 공개된 오픈소스 탐지 프로젝트(Nearby Glasses, NoPeek 등)에서 보고된 값입니다.
// 실제 기기로 검증한 뒤 필요하면 추가/수정하세요. (RAW_DUMP 모드로 직접 수집 가능)
#pragma once
#include <stdint.h>

enum Confidence : uint8_t {
  CONF_NONE = 0,
  CONF_LOW = 1,     // 이름만 비슷함
  CONF_MEDIUM = 2,  // 제조사 ID는 맞지만 안경 외 기기(예: Quest 헤드셋)일 수 있음
  CONF_HIGH = 3,    // 안경 전용 제조사 ID, 또는 제조사 ID + 이름 둘 다 일치
};

struct CompanySignature {
  uint16_t companyId;
  const char* vendor;
  Confidence confidence;
};

// Bluetooth SIG Assigned Numbers - Company Identifiers
static const CompanySignature COMPANY_SIGNATURES[] = {
    {0x0D53, "Luxottica (Ray-Ban/Oakley Meta)", CONF_HIGH},
    {0x01AB, "Meta Platforms", CONF_MEDIUM},
    {0x058E, "Meta Platforms Technologies", CONF_MEDIUM},
    {0x03C2, "Snap (Spectacles)", CONF_HIGH},
};

struct NameSignature {
  const char* keyword;  // 대소문자 구분 없이 부분 일치
  const char* vendor;
};

static const NameSignature NAME_SIGNATURES[] = {
    {"ray-ban", "Ray-Ban Meta"},
    {"rb meta", "Ray-Ban Meta"},
    {"oakley", "Oakley Meta"},
    {"meta glass", "Meta"},
    {"even g1", "Even Realities G1"},
    {"even g2", "Even Realities G2"},
    {"even realities", "Even Realities"},
    {"spectacles", "Snap Spectacles"},
    {"xreal", "XREAL"},
    {"rokid", "Rokid"},
    {"rayneo", "RayNeo"},
    {"inmo", "INMO"},
    {"vuzix", "Vuzix"},
    {"brilliant", "Brilliant Labs"},
    {"mentra", "Mentra"},
    {"halliday", "Halliday"},
    {"solos", "Solos"},
};

static const size_t COMPANY_SIGNATURE_COUNT = sizeof(COMPANY_SIGNATURES) / sizeof(COMPANY_SIGNATURES[0]);
static const size_t NAME_SIGNATURE_COUNT = sizeof(NAME_SIGNATURES) / sizeof(NAME_SIGNATURES[0]);
