// 흑백 OLED(SSD1306/SH1106)용. 컬러 TFT 보드에서는 display_tft.cpp가 대신 쓰입니다.
#include "config.h"
#ifndef DISPLAY_TFT

#include "display.h"

#include <U8g2lib.h>
#include <Wire.h>

namespace {

#if OLED_SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
#else
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
#endif

void drawBattery(const UiStatus& st) {
  char buf[12];
  int x = 128;
  if (st.muted) {
    u8g2.drawStr(x - 6, 8, "M");
    x -= 9;
  }
  if (st.batteryPct >= 0) {
    snprintf(buf, sizeof(buf), "%d%%", st.batteryPct);
    int w = u8g2.getStrWidth(buf);
    u8g2.drawStr(x - w, 8, buf);
  }
}

char trendChar(const Target& t) {
  float d = trendDb(t);
  if (d >= 3) return '^';
  if (d <= -3) return 'v';
  return ' ';
}

}  // namespace

void displayBegin() {
  u8g2.begin();
  u8g2.setBusClock(400000);
  u8g2.setContrast(160);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 28, "Glasses Detector");
  u8g2.drawStr(28, 44, "starting...");
  u8g2.sendBuffer();
}

void displayList(const Target* list, size_t n, size_t selected, const UiStatus& st) {
  char buf[32];
  uint32_t now = millis();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  snprintf(buf, sizeof(buf), "GLASSES: %u", (unsigned)n);
  u8g2.drawStr(0, 8, buf);
  drawBattery(st);
  u8g2.drawHLine(0, 10, 128);

  if (n == 0) {
    static const char spin[] = "|/-\\";
    snprintf(buf, sizeof(buf), "Scanning %c", spin[(now / 250) % 4]);
    u8g2.drawStr(28, 30, buf);
    snprintf(buf, sizeof(buf), "%lu BLE pkt/s", (unsigned long)st.pps);
    u8g2.drawStr((128 - u8g2.getStrWidth(buf)) / 2, 44, buf);
    u8g2.drawStr(10, 60, "No glasses nearby");
    u8g2.sendBuffer();
    return;
  }

  // 선택 항목이 보이도록 4줄 창을 스크롤
  const size_t rows = 4;
  size_t first = (selected >= rows) ? selected - rows + 1 : 0;
  for (size_t r = 0; r < rows && first + r < n; r++) {
    const Target& t = list[first + r];
    int y = 21 + r * 10;
    bool active = now - t.lastSeen < ACTIVE_MS;
    char right[10];
    if (active) {
      snprintf(right, sizeof(right), "%4d%c", (int)t.emaRssi, trendChar(t));
    } else {
      snprintf(right, sizeof(right), "%3lus", (unsigned long)((now - t.lastSeen) / 1000));
    }
    char mark = usageState(t) == USAGE_CONNECTED ? '*' : ' ';
    snprintf(buf, sizeof(buf), "%c%-14.14s%s", mark, t.vendor, right);
    if (first + r == selected) {
      u8g2.drawBox(0, y - 9, 128, 10);
      u8g2.setDrawColor(0);
      u8g2.drawStr(1, y - 1, buf);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(1, y - 1, buf);
    }
  }
  u8g2.drawStr(0, 63, "*=in use  hold=track");
  u8g2.sendBuffer();
}

void displayTrack(const Target* t, const UiStatus& st) {
  char buf[32];
  uint32_t now = millis();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);

  if (!t) {
    u8g2.drawStr(0, 8, "TRACK");
    drawBattery(st);
    u8g2.drawStr(25, 32, "TARGET LOST");
    u8g2.drawStr(4, 60, "hold = back to list");
    u8g2.sendBuffer();
    return;
  }

  snprintf(buf, sizeof(buf), "%.15s", t->vendor);
  u8g2.drawStr(0, 8, buf);
  drawBattery(st);
  u8g2.drawHLine(0, 10, 128);

  uint32_t age = now - t->lastSeen;
  bool stale = age > 8000;

  // 큰 RSSI 숫자
  u8g2.setFont(u8g2_font_logisoso22_tn);
  snprintf(buf, sizeof(buf), "%d", (int)t->emaRssi);
  u8g2.drawStr(0, 37, buf);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(48, 24, "dBm");
  u8g2.drawStr(48, 36, proximityLabel(t->emaRssi));

  const char* trend = "STEADY";
  float d = trendDb(*t);
  if (stale) trend = "NO SIGNAL";
  else if (d >= 3) trend = "CLOSER";
  else if (d <= -3) trend = "FARTHER";
  u8g2.drawStr(128 - u8g2.getStrWidth(trend), 24, trend);
  const char* usage = usageLabel(usageState(*t));
  u8g2.drawStr(128 - u8g2.getStrWidth(usage), 36, usage);

  // 신호 막대 (-100 ~ -40 dBm)
  int w = (int)((t->emaRssi + 100) * 128 / 60);
  w = constrain(w, 0, 128);
  u8g2.drawFrame(0, 42, 128, 9);
  if (!stale) u8g2.drawBox(0, 42, w, 9);

  if (age < 1000) {
    snprintf(buf, sizeof(buf), "live  %s  #%u", confidenceLabel(t->confidence), t->id);
  } else {
    snprintf(buf, sizeof(buf), "seen %lus ago  #%u", (unsigned long)(age / 1000), t->id);
  }
  u8g2.drawStr(0, 62, buf);
  u8g2.sendBuffer();
}

void displayAlert(const Target& t) {
  char buf[32];
  u8g2.clearBuffer();
  u8g2.drawBox(0, 0, 128, 64);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 12, "! SMART GLASSES !");
  snprintf(buf, sizeof(buf), "%.20s", t.vendor);
  u8g2.drawStr((128 - u8g2.getStrWidth(buf)) / 2, 30, buf);
  snprintf(buf, sizeof(buf), "%d dBm  %s", (int)t.emaRssi, proximityLabel(t.emaRssi));
  u8g2.drawStr((128 - u8g2.getStrWidth(buf)) / 2, 44, buf);
  snprintf(buf, sizeof(buf), "conf %s  %s", confidenceLabel(t.confidence), usageLabel(usageState(t)));
  u8g2.drawStr((128 - u8g2.getStrWidth(buf)) / 2, 58, buf);
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}

#endif  // !DISPLAY_TFT
