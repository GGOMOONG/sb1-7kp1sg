// 하드웨어 핀과 동작 설정
// 보드마다 다른 핀은 platformio.ini의 build_flags(-D...)로 덮어씁니다.
#pragma once
#include <cstdint>

// ---------- 화면 (SSD1306 / SH1106 128x64 I2C OLED) ----------
#ifndef OLED_SDA
#define OLED_SDA 21
#endif
#ifndef OLED_SCL
#define OLED_SCL 22
#endif
// 1.3인치 OLED는 대부분 SH1106 칩입니다. 화면이 2픽셀 밀리거나 깨지면 1로 바꾸세요.
#ifndef OLED_SH1106
#define OLED_SH1106 0
#endif

// ---------- 컬러 TFT (XIAO ESP32-S3 Plus 1.14" IPS, ST7789 135x240) ----------
// platformio.ini의 xiao_s3_plus_114 환경에서 -DDISPLAY_TFT로 켜집니다.
// 핀은 Seeed 위키 + arduino-esp32 XIAO_ESP32S3_Plus variant 기준(raw GPIO).
#ifdef DISPLAY_TFT
  #define TFT_SCLK 7    // D8
  #define TFT_MOSI 9    // D10
  #define TFT_CS   3    // D2
  #define TFT_DC   4    // D3
  #define TFT_RST  13   // D17
  #define TFT_BL   12   // D18 (백라이트)
  #define TFT_W    135
  #define TFT_H    240
  #ifndef TFT_ROTATION
  #define TFT_ROTATION 3   // 가로 모드(240x135). 화면이 뒤집히면 1로
  #endif
  #ifndef TFT_COL_OFFSET
  #define TFT_COL_OFFSET 52  // 135x240 패널 오프셋. 화면이 밀리면 조정
  #endif
  #ifndef TFT_ROW_OFFSET
  #define TFT_ROW_OFFSET 40
  #endif
  #ifndef TFT_INVERT
  #define TFT_INVERT 1       // ST7789 IPS는 보통 반전 켜짐. 색이 이상하면 0
  #endif
#endif

// ---------- 버튼 ----------
// 단일 버튼(기본): 모든 ESP32 보드의 BOOT 버튼(GPIO0). 추가 배선 불필요.
// 3버튼(TFT 보드): THREE_BUTTONS + BTN_A/B/C 를 build_flags로 지정.
#ifndef BUTTON_PIN
#define BUTTON_PIN 0
#endif
#ifndef BTN_A_PIN
#define BTN_A_PIN -1   // 다음/위
#endif
#ifndef BTN_B_PIN
#define BTN_B_PIN -1   // 선택(추적 진입)/뒤로
#endif
#ifndef BTN_C_PIN
#define BTN_C_PIN -1   // 음소거
#endif

// ---------- 알림 ----------
#ifndef LED_PIN
#define LED_PIN 2  // -1이면 사용 안 함
#endif
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 0  // XIAO처럼 LOW일 때 켜지는 LED는 1
#endif
#ifndef BUZZER_PIN
#define BUZZER_PIN -1  // 액티브 부저나 진동모터(트랜지스터 경유) GPIO. -1이면 사용 안 함
#endif

// ---------- 배터리 ----------
// LiPo(+) -- 100k -- [BAT_ADC_PIN] -- 100k -- GND  (1/2 분압)
#ifndef BAT_ADC_PIN
#define BAT_ADC_PIN -1  // -1이면 배터리 표시 안 함
#endif
#ifndef BAT_DIVIDER
#define BAT_DIVIDER 2.0f
#endif

// ---------- 탐지 ----------
static const int RSSI_THRESHOLD = -95;       // 폰에 연결된 안경은 신호가 약하고 드물어서 낮게 잡습니다
static const uint32_t REALERT_MS = 30000;    // 같은 대상을 다시 알리는 최소 간격
static const uint32_t FORGET_MS = 180000;    // 연결된 안경은 광고가 드물어서 3분간 기억합니다
static const uint32_t ACTIVE_MS = 20000;     // 이 시간 안에 보였으면 "지금 근처에 있음"
static const bool RAW_DUMP = false;          // true면 모든 광고 패킷을 시리얼로 출력
static const bool LOG_OBSERVATIONS = true;   // 안경 패킷을 CSV로 시리얼 출력 (연결 상태 행동 분석용)
