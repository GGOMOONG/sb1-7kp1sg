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
  CONF_LOW = 1,     // 이름이 다른 제품과 겹칠 수 있음
  CONF_MEDIUM = 2,  // 안경 전용 이름, 또는 안경 외 기기(Quest 등)와 공유하는 제조사 ID
  CONF_HIGH = 3,    // 안경 전용 제조사 ID, 또는 제조사 ID + 이름 둘 다 일치
};

struct CompanySignature {
  uint16_t companyId;
  const char* vendor;
  Confidence confidence;
};

// Bluetooth SIG Assigned Numbers - Company Identifiers
static const CompanySignature COMPANY_SIGNATURES[] = {
    // 이름은 OLED 한 줄(약 20자)에 들어가도록 짧게 씁니다
    {0x0D53, "Ray-Ban/Oakley Meta", CONF_HIGH},  // Luxottica
    {0x01AB, "Meta device", CONF_MEDIUM},        // Meta Platforms (Quest와 공유)
    {0x058E, "Meta device", CONF_MEDIUM},        // Meta Platforms Technologies
    {0x03C2, "Snap Spectacles", CONF_HIGH},
};

struct NameSignature {
  const char* keyword;  // 대소문자 구분 없이 부분 일치
  const char* vendor;
  Confidence confidence;  // 이름만 일치했을 때의 신뢰도 (안경 전용 이름이면 MEDIUM)
};

static const NameSignature NAME_SIGNATURES[] = {
    {"ray-ban", "Ray-Ban Meta", CONF_MEDIUM},
    {"rb meta", "Ray-Ban Meta", CONF_MEDIUM},
    {"oakley", "Oakley Meta", CONF_MEDIUM},
    {"meta glass", "Meta glasses", CONF_MEDIUM},
    // Even Realities G1은 양쪽 다리가 각각 "Even G1_xx_L_..." / "_R_..." 로 따로 광고합니다
    {"even g1", "Even G1", CONF_MEDIUM},
    {"even g2", "Even G2", CONF_MEDIUM},
    {"even realities", "Even Realities", CONF_MEDIUM},
    {"spectacles", "Snap Spectacles", CONF_MEDIUM},
    {"xreal", "XREAL", CONF_MEDIUM},
    {"rokid", "Rokid", CONF_MEDIUM},
    {"rayneo", "RayNeo", CONF_MEDIUM},
    {"vuzix", "Vuzix", CONF_MEDIUM},
    {"mentra", "Mentra", CONF_MEDIUM},
    {"halliday", "Halliday", CONF_MEDIUM},
    // 다른 제품에도 쓰일 수 있는 일반적인 이름은 LOW (화면에는 안 뜨고 시리얼 로그에만 남음)
    {"inmo", "INMO", CONF_LOW},
    {"brilliant", "Brilliant Labs", CONF_LOW},
    {"solos", "Solos", CONF_LOW},
};

static const size_t COMPANY_SIGNATURE_COUNT = sizeof(COMPANY_SIGNATURES) / sizeof(COMPANY_SIGNATURES[0]);
static const size_t NAME_SIGNATURE_COUNT = sizeof(NAME_SIGNATURES) / sizeof(NAME_SIGNATURES[0]);
