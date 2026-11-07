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
  CONF_LOW = 1,     // 이름이 다른 제품과 겹칠 수 있음 / 미확인 기기
  CONF_MEDIUM = 2,  // 안경 전용 이름, 또는 안경 외 기기(Quest 등)와 공유하는 제조사 ID
  CONF_HIGH = 3,    // 안경 전용 제조사 ID, 또는 제조사 ID + 이름 둘 다 일치
};

// 분류 결과
enum Category : uint8_t {
  CAT_NONE = 0,
  CAT_GLASSES = 1,  // 안경(알려진 제조사/이름)
  CAT_UNKNOWN = 2,  // 미확인: 무명 BLE 칩 벤더 등 — 싸구려 카메라일 수 있어 따로 표시
};

struct CompanySignature {
  uint16_t companyId;
  const char* vendor;
  Confidence confidence;
};

// ── 안경 화이트리스트 (Bluetooth SIG Company ID). 값은 SIG 공개 목록에서 확인 ──
static const CompanySignature COMPANY_SIGNATURES[] = {
    // 이름은 화면 한 줄(약 20자)에 들어가도록 짧게 씁니다
    {0x0D53, "Ray-Ban/Oakley Meta", CONF_HIGH},  // Luxottica
    {0x01AB, "Meta device", CONF_MEDIUM},        // Meta Platforms (Quest와 공유)
    {0x058E, "Meta device", CONF_MEDIUM},        // Meta Platforms Technologies
    {0x03C2, "Snap Spectacles", CONF_HIGH},
    {0x10F9, "Even Realities", CONF_HIGH},        // SIG 확인됨
    {0x060C, "Vuzix", CONF_HIGH},                 // SIG 확인됨
};

// ── 블랙리스트: 안경이 아닌 게 거의 확실한 소비자 브랜드 → 아예 무시 ──
// 이어폰/헤드폰/워치/폰/스피커 제조사. SIG 공개 목록에서 추출.
static const uint16_t DENY_COMPANIES[] = {
    0x004C,  // Apple
    0x0075,  // Samsung
    0x00E0, 0x018E,  // Google
    0x009E,  // Bose
    0x012D, 0x0056,  // Sony / Sony Ericsson
    0x00CC,  // Beats
    0x0087,  // Garmin
    0x0157,  // Huami/Amazfit
    0x01DA,  // Logitech
    0x027D,  // Huawei
    0x02F2,  // GoPro
    0x038F,  // Xiaomi
    0x0494,  // Sennheiser
    0x05A7,  // Sonos
    0x0055,  // Plantronics
    0x0057, 0x075A,  // Harman / JBL
    0x072F,  // OnePlus
    0x079A,  // Oppo
    0x07C9,  // Skullcandy
    0x08A4,  // Realme
    0x08AA,  // DJI
    0x0CC2,  // Anker
    0x0CCB,  // Nothing
};

// ── 미확인(의심) 칩 벤더: 무명 중국 카메라/안경이 자주 쓰는 BLE 칩. 싸구려 이어폰도 씀 ──
// 안경이라고 확정 못 하지만, '미확인 무선기기(낮은 확신)'로 따로 보여줘서 직접 판단하게 함.
static const CompanySignature SUSPECT_COMPANIES[] = {
    {0x0211, "unknown (Telink)", CONF_LOW},
    {0x05F0, "unknown (Beken)", CONF_LOW},
    {0x0642, "unknown (Bluetrum)", CONF_LOW},
    {0x08E7, "unknown (Barrot)", CONF_LOW},
    {0x005D, "unknown (Realtek)", CONF_LOW},
    {0xFFFF, "unknown (no ID)", CONF_LOW},  // 테스트/미등록 ID
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
static const size_t DENY_COMPANY_COUNT = sizeof(DENY_COMPANIES) / sizeof(DENY_COMPANIES[0]);
static const size_t SUSPECT_COMPANY_COUNT = sizeof(SUSPECT_COMPANIES) / sizeof(SUSPECT_COMPANIES[0]);

// 미확인(의심) 기기를 화면에 표시할지. 끄면 안경만 표시(노이즈 최소).
#ifndef SHOW_UNKNOWN
#define SHOW_UNKNOWN 1
#endif
