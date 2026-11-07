// 컬러 TFT(ST7789 135x240) 디스플레이 — XIAO ESP32-S3 Plus 1.14" IPS 보드용.
// display.h의 인터페이스를 그대로 구현하므로 main.cpp는 바뀌지 않습니다.
// 라이브러리: Arduino_GFX (moononournation). 깜빡임 없이 그리려고 캔버스(프레임버퍼)에 그린 뒤 flush.
#ifdef DISPLAY_TFT

#include <Arduino_GFX_Library.h>

#include "config.h"
#include "display.h"

namespace {

Arduino_DataBus* bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX* panel = new Arduino_ST7789(bus, TFT_RST, TFT_ROTATION, true /*IPS*/, TFT_W, TFT_H,
                                        TFT_COL_OFFSET, TFT_ROW_OFFSET, TFT_COL_OFFSET + 1, TFT_ROW_OFFSET);
// 가로 모드 캔버스(240x135). 내부 RAM 약 64KB.
Arduino_Canvas* gfx = new Arduino_Canvas(TFT_H, TFT_W, panel);

const int16_t W = TFT_H;  // 240
const int16_t H = TFT_W;  // 135

// 색 (RGB565)
const uint16_t C_BG = RGB565(10, 12, 18);
const uint16_t C_FG = RGB565(230, 232, 238);
const uint16_t C_DIM = RGB565(120, 128, 140);
const uint16_t C_HI = RGB565(255, 212, 64);    // 강조(노랑)
const uint16_t C_INUSE = RGB565(255, 80, 80);  // 사용 중(빨강)
const uint16_t C_IDLE = RGB565(90, 200, 120);  // 대기(초록)
const uint16_t C_BAR = RGB565(64, 170, 255);   // 신호 막대(파랑)
const uint16_t C_SEL = RGB565(32, 44, 70);     // 선택 행 배경

void text(int16_t x, int16_t y, const char* s, uint16_t color, uint8_t size = 1) {
  gfx->setTextColor(color);
  gfx->setTextSize(size);
  gfx->setCursor(x, y);
  gfx->print(s);
}
int16_t textW(const char* s, uint8_t size) { return (int16_t)strlen(s) * 6 * size; }

uint16_t rssiColor(float r) {
  if (r >= -60) return C_IDLE;
  if (r >= -78) return C_HI;
  return C_INUSE;
}

uint16_t usageColor(UsageState s) {
  return s == USAGE_CONNECTED ? C_INUSE : (s == USAGE_PAIRING ? C_IDLE : C_DIM);
}

void drawHeader(const char* title, const UiStatus& st) {
  gfx->fillRect(0, 0, W, 16, C_SEL);
  text(4, 4, title, C_FG, 1);
  char buf[16];
  int16_t x = W - 4;
  if (st.batteryPct >= 0) {
    snprintf(buf, sizeof(buf), "%d%%", st.batteryPct);
    x -= textW(buf, 1);
    text(x, 4, buf, st.batteryPct <= 15 ? C_INUSE : C_FG, 1);
    x -= 6;
  }
  if (st.muted) {
    x -= textW("MUTE", 1);
    text(x, 4, "MUTE", C_DIM, 1);
  }
}

}  // namespace

void displayBegin() {
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif
  gfx->begin();
#if TFT_INVERT
  panel->invertDisplay(true);
#endif
  gfx->fillScreen(C_BG);
  text(28, 50, "Glasses Detector", C_HI, 2);
  text(90, 80, "scanning...", C_DIM, 1);
  gfx->flush();
}

void displayList(const Target* list, size_t n, size_t selected, const UiStatus& st) {
  uint32_t now = millis();
  gfx->fillScreen(C_BG);
  size_t nG = 0, nU = 0;
  for (size_t i = 0; i < n; i++) (list[i].category == CAT_UNKNOWN ? nU : nG)++;
  char title[24];
  snprintf(title, sizeof(title), "GLASSES %u  ? %u", (unsigned)nG, (unsigned)nU);
  drawHeader(title, st);

  if (n == 0) {
    static const char sp[] = "|/-\\";
    char b[24];
    snprintf(b, sizeof(b), "Scanning %c", sp[(now / 250) % 4]);
    text((W - textW(b, 2)) / 2, 54, b, C_FG, 2);
    snprintf(b, sizeof(b), "%lu BLE pkt/s", (unsigned long)st.pps);
    text((W - textW(b, 1)) / 2, 84, b, C_DIM, 1);
    gfx->flush();
    return;
  }

  const size_t rows = 5;
  const int16_t rowH = 22;
  size_t first = (selected >= rows) ? selected - rows + 1 : 0;
  for (size_t r = 0; r < rows && first + r < n; r++) {
    const Target& t = list[first + r];
    int16_t y = 18 + r * rowH;
    bool active = now - t.lastSeen < ACTIVE_MS;
    if (first + r == selected) gfx->fillRect(0, y, W, rowH - 1, C_SEL);

    bool unknown = (t.category == CAT_UNKNOWN);
    UsageState us = usageState(t);
    gfx->fillRect(3, y + 5, 5, rowH - 11, unknown ? C_DIM : usageColor(us));  // 상태 색 점
    text(13, y + 3, t.vendor, unknown ? C_DIM : C_FG, 1);

    char info[20];
    if (unknown)
      snprintf(info, sizeof(info), "? unidentified");
    else
      snprintf(info, sizeof(info), "%s%s", confidenceLabel(t.confidence), us == USAGE_CONNECTED ? " USE" : "");
    text(13, y + 12, info, C_DIM, 1);

    char right[12];
    if (active) {
      snprintf(right, sizeof(right), "%d", (int)t.emaRssi);
      uint16_t rc = rssiColor(t.emaRssi);
      text(W - textW(right, 2) - 4, y + 3, right, rc, 2);
      float d = trendDb(t);
      const char* tr = d >= 3 ? "UP" : (d <= -3 ? "DN" : "");
      if (tr[0]) text(W - textW(tr, 1) - 4, y + 13, tr, rc, 1);
    } else {
      snprintf(right, sizeof(right), "%lus", (unsigned long)((now - t.lastSeen) / 1000));
      text(W - textW(right, 1) - 4, y + 7, right, C_DIM, 1);
    }
  }
  gfx->flush();
}

void displayTrack(const Target* t, const UiStatus& st) {
  uint32_t now = millis();
  gfx->fillScreen(C_BG);
  if (!t) {
    drawHeader("TRACK", st);
    text((W - textW("TARGET LOST", 2)) / 2, 54, "TARGET LOST", C_INUSE, 2);
    text((W - textW("B = back", 1)) / 2, 90, "B = back", C_DIM, 1);
    gfx->flush();
    return;
  }
  drawHeader(t->vendor, st);
  uint32_t age = now - t->lastSeen;
  bool stale = age > 8000;
  uint16_t rc = stale ? C_DIM : rssiColor(t->emaRssi);

  // 큰 RSSI 숫자
  char buf[24];
  snprintf(buf, sizeof(buf), "%d", (int)t->emaRssi);
  text(8, 30, buf, rc, 4);
  text(8, 70, "dBm", C_DIM, 1);
  text(48, 70, proximityLabel(t->emaRssi), C_FG, 1);

  // 추세 / 사용상태
  const char* trend = stale ? "NO SIGNAL" : "STEADY";
  float d = trendDb(*t);
  if (!stale && d >= 3) trend = "CLOSER";
  else if (!stale && d <= -3) trend = "FARTHER";
  text(W - textW(trend, 2) - 6, 26, trend, rc, 2);
  const char* us = usageLabel(usageState(*t));
  text(W - textW(us, 2) - 6, 52, us, usageColor(usageState(*t)), 2);

  // 신호 막대 (-100~-40)
  int bw = (int)((t->emaRssi + 100) * (W - 12) / 60);
  bw = constrain(bw, 0, W - 12);
  gfx->drawRect(6, 92, W - 12, 16, C_DIM);
  if (!stale) gfx->fillRect(6, 92, bw, 16, C_BAR);

  if (age < 1000) snprintf(buf, sizeof(buf), "live  %s  #%u", confidenceLabel(t->confidence), t->id);
  else snprintf(buf, sizeof(buf), "seen %lus ago  #%u", (unsigned long)(age / 1000), t->id);
  text(6, 118, buf, C_DIM, 1);
  gfx->flush();
}

void displayAlert(const Target& t) {
  gfx->fillScreen(C_INUSE);
  text((W - textW("SMART GLASSES", 2)) / 2, 18, "SMART GLASSES", C_BG, 2);
  char buf[28];
  snprintf(buf, sizeof(buf), "%.18s", t.vendor);
  text((W - textW(buf, 2)) / 2, 50, buf, RGB565(255, 255, 255), 2);
  snprintf(buf, sizeof(buf), "%d dBm  %s", (int)t.emaRssi, proximityLabel(t.emaRssi));
  text((W - textW(buf, 1)) / 2, 82, buf, C_BG, 1);
  snprintf(buf, sizeof(buf), "%s  %s", confidenceLabel(t.confidence), usageLabel(usageState(t)));
  text((W - textW(buf, 1)) / 2, 104, buf, C_BG, 1);
  gfx->flush();
}

#endif  // DISPLAY_TFT
