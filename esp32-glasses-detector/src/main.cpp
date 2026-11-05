// ESP32 휴대용 스마트글래스 탐지/추적기
//
// - BLE 광고를 듀티 100%로 계속 스캔해서 스마트글래스만 골라냅니다.
// - 폰에 연결되어 광고가 드문 안경도 잡도록 오래 기억하고, 주소가 바뀌어도 이어서 추적합니다.
// - OLED에 목록을 보여주고, 한 대를 골라 "추적 모드"로 들어가면
//   신호 세기에 따라 부저가 가이거 계수기처럼 빨라져서 걸으면서 위치를 찾을 수 있습니다.
//
// 버튼(BOOT, GPIO0)
//   목록 화면: 짧게 = 다음 대상,  길게(0.7초) = 선택한 대상 추적
//   추적 화면: 짧게 = 소리 켜기/끄기, 길게 = 목록으로
//   아무 화면: 아주 길게(3초) = 소리 켜기/끄기

#include <Arduino.h>

#include "config.h"
#include "detector.h"
#include "display.h"

namespace {

enum Mode { MODE_LIST, MODE_TRACK };
Mode mode = MODE_LIST;
uint16_t selectedId = 0;
uint16_t trackId = 0;
bool muted = false;
uint32_t alertUntil = 0;
Target alertTarget;

// ---------------- LED / 부저 (non-blocking) ----------------
struct Beeper {
  uint8_t remaining = 0;
  bool on = false;
  uint32_t nextMs = 0;
  uint16_t onMs = 0, offMs = 0;
} beeper;

void setOutputs(bool on) {
  if (LED_PIN >= 0) digitalWrite(LED_PIN, (on != (bool)LED_ACTIVE_LOW) ? HIGH : LOW);
  if (BUZZER_PIN >= 0) digitalWrite(BUZZER_PIN, (on && !muted) ? HIGH : LOW);
}

void beep(uint8_t count, uint16_t onMs, uint16_t offMs) {
  beeper.remaining = count;
  beeper.onMs = onMs;
  beeper.offMs = offMs;
  beeper.on = false;
  beeper.nextMs = millis();
}

void beeperUpdate(uint32_t now) {
  if (beeper.remaining == 0 && !beeper.on) return;
  if ((int32_t)(now - beeper.nextMs) < 0) return;
  if (beeper.on) {
    beeper.on = false;
    setOutputs(false);
    beeper.nextMs = now + beeper.offMs;
  } else if (beeper.remaining > 0) {
    beeper.remaining--;
    beeper.on = true;
    setOutputs(true);
    beeper.nextMs = now + beeper.onMs;
  }
}

// ---------------- 배터리 ----------------
int batteryPercent() {
  if (BAT_ADC_PIN < 0) return -1;
  static int cached = -1;
  static uint32_t lastMs = 0;
  if (cached >= 0 && millis() - lastMs < 5000) return cached;
  lastMs = millis();
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) sum += analogReadMilliVolts(BAT_ADC_PIN);
  float mv = (sum / 8.0f) * BAT_DIVIDER;
  // LiPo 1셀 방전 곡선 근사
  static const int16_t curve[][2] = {{4200, 100}, {4100, 90}, {4000, 80}, {3900, 65}, {3800, 50},
                                     {3700, 35},  {3600, 20}, {3500, 10}, {3300, 0}};
  int pct = 0;
  if (mv >= curve[0][0]) {
    pct = 100;
  } else {
    for (size_t i = 1; i < sizeof(curve) / sizeof(curve[0]); i++) {
      if (mv >= curve[i][0]) {
        float f = (mv - curve[i][0]) / (float)(curve[i - 1][0] - curve[i][0]);
        pct = curve[i][1] + f * (curve[i - 1][1] - curve[i][1]);
        break;
      }
    }
  }
  cached = pct;
  return pct;
}

// ---------------- 입력 ----------------
// 화면/버튼 구성에 상관없이 loop는 Action만 처리합니다.
//  단일 버튼(BOOT): 짧게=다음/음소거, 길게=선택/뒤로, 아주길게=음소거
//  3버튼(TFT 보드): A=다음, B=선택/뒤로, C=음소거
enum Action { ACT_NONE, ACT_NEXT, ACT_SELECT, ACT_BACK, ACT_MUTE };
enum Press { PRESS_NONE, PRESS_SHORT, PRESS_LONG, PRESS_VERY_LONG };

#if defined(THREE_BUTTONS)
// 3버튼: 각 핀 눌림 에지를 디바운스해서 감지 (active-low, 외부 풀업)
bool edge(int pin, bool& last, uint32_t& tms, uint32_t now) {
  bool pressed = digitalRead(pin) == LOW;
  bool fired = false;
  if (pressed && !last && now - tms > 150) { fired = true; tms = now; }
  last = pressed;
  return fired;
}
Action pollInput(uint32_t now, bool inTrack) {
  static bool la = false, lb = false, lc = false;
  static uint32_t ta = 0, tb = 0, tc = 0;
  if (edge(BTN_A_PIN, la, ta, now)) return ACT_NEXT;
  if (edge(BTN_B_PIN, lb, tb, now)) return inTrack ? ACT_BACK : ACT_SELECT;
  if (edge(BTN_C_PIN, lc, tc, now)) return ACT_MUTE;
  return ACT_NONE;
}
#else
Press readButton(uint32_t now) {
  static bool down = false;
  static uint32_t downMs = 0;
  static bool longFired = false;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  if (pressed && !down) {
    down = true;
    downMs = now;
    longFired = false;
  } else if (pressed && down) {
    if (!longFired && now - downMs >= 3000) {
      longFired = true;
      return PRESS_VERY_LONG;
    }
  } else if (!pressed && down) {
    down = false;
    uint32_t held = now - downMs;
    if (longFired || held < 40) return PRESS_NONE;  // 3초 처리 완료 또는 채터링
    return held >= 700 ? PRESS_LONG : PRESS_SHORT;
  }
  return PRESS_NONE;
}
Action pollInput(uint32_t now, bool inTrack) {
  switch (readButton(now)) {
    case PRESS_SHORT: return inTrack ? ACT_MUTE : ACT_NEXT;
    case PRESS_LONG: return inTrack ? ACT_BACK : ACT_SELECT;
    case PRESS_VERY_LONG: return ACT_MUTE;
    default: return ACT_NONE;
  }
}
#endif

// ---------------- 알림 ----------------
void onAlert(const Target& t) {
  Serial.println();
  Serial.println("========== 스마트글래스 감지 ==========");
  Serial.printf(" 제조사/모델 : %s\n", t.vendor);
  Serial.printf(" 신뢰도      : %s\n", confidenceLabel(t.confidence));
  Serial.printf(" 신호 세기   : %d dBm (%s)\n", t.lastRssi, proximityLabel(t.lastRssi));
  Serial.printf(" 사용 상태   : %s (추정)\n", usageLabel(usageState(t)));
  Serial.printf(" BLE 주소    : %s\n", t.address);
  if (t.name[0]) Serial.printf(" 기기 이름   : %s\n", t.name);
  if (t.companyId != 0xFFFF) Serial.printf(" Company ID  : 0x%04X\n", t.companyId);
  Serial.println("========================================");

  if (mode == MODE_TRACK) {
    beep(1, 40, 0);  // 추적 중에는 화면을 가리지 않고 짧게만 알림
    return;
  }
  alertTarget = t;
  alertUntil = millis() + 2000;
  if (t.confidence >= CONF_HIGH) beep(3, 120, 120);
  else beep(1, 150, 0);
}

// 추적 모드: 신호가 강할수록 빠르게 삑삑 (가이거 계수기 방식)
void trackerBeep(const Target& t, uint32_t now) {
  static uint32_t lastBeep = 0;
  if (now - t.lastSeen > 3000) return;  // 신호가 끊기면 조용히
  float r = constrain(t.emaRssi, -95.0f, -45.0f);
  uint32_t period = (uint32_t)(80 + (r + 45.0f) / -50.0f * 1420);  // -45dBm: 80ms, -95dBm: 1500ms
  if (now - lastBeep >= period && beeper.remaining == 0 && !beeper.on) {
    lastBeep = now;
    beep(1, 25, 0);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
#if defined(THREE_BUTTONS)
  pinMode(BTN_A_PIN, INPUT_PULLUP);
  pinMode(BTN_B_PIN, INPUT_PULLUP);
  pinMode(BTN_C_PIN, INPUT_PULLUP);
#else
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif
  if (LED_PIN >= 0) pinMode(LED_PIN, OUTPUT);
  if (BUZZER_PIN >= 0) pinMode(BUZZER_PIN, OUTPUT);
  setOutputs(false);
  if (BAT_ADC_PIN >= 0) analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);

  displayBegin();
  detectorBegin();
  Serial.println("스마트글래스 BLE 탐지기 시작 - 스캔 중...");
  if (LOG_OBSERVATIONS) Serial.println("OBS,ms,id,address,company_id,rssi,connectable,legacy,name");
}

void loop() {
  static Target list[16];
  static uint32_t lastDraw = 0;
  uint32_t now = millis();

  detectorProcess(onAlert);
  beeperUpdate(now);
  size_t n = detectorSnapshot(list, 16);

  // 선택 상태는 순서가 바뀌어도 유지되도록 id로 기억합니다
  size_t sel = 0;
  for (size_t i = 0; i < n; i++)
    if (list[i].id == selectedId) sel = i;
  if (n > 0) selectedId = list[sel].id;

  Action act = pollInput(now, mode == MODE_TRACK);
  switch (act) {
    case ACT_NEXT:
      if (mode == MODE_LIST && n > 0) { sel = (sel + 1) % n; selectedId = list[sel].id; }
      break;
    case ACT_SELECT:
      if (mode == MODE_LIST && n > 0) {
        mode = MODE_TRACK;
        trackId = selectedId;
        Serial.printf("[추적 시작] #%u %s\n", trackId, list[sel].vendor);
      }
      break;
    case ACT_BACK:
      mode = MODE_LIST;
      break;
    case ACT_MUTE:
      muted = !muted;
      beep(1, 60, 0);
      break;
    default:
      break;
  }
  if (act != ACT_NONE) alertUntil = 0;

  Target tracked;
  bool haveTracked = (mode == MODE_TRACK) && detectorGet(trackId, tracked);
  if (haveTracked) trackerBeep(tracked, now);

  if (now - lastDraw >= 120) {
    lastDraw = now;
    UiStatus st{batteryPercent(), muted, detectorPacketsPerSec()};
    if (alertUntil && (int32_t)(alertUntil - now) > 0) {
      displayAlert(alertTarget);
    } else if (mode == MODE_TRACK) {
      displayTrack(haveTracked ? &tracked : nullptr, st);
    } else {
      displayList(list, n, sel, st);
    }
  }
  delay(5);
}
