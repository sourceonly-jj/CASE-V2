// ==========================
// ==========================

static void clearField(int x, int y, int w, int h) {
  gfx->fillRect(x, y, w, h, C_BLACK);
}

// ---- Screen 0: main readout ----
void drawMainScreen() {
  char buf[16];
  int16_t x1, y1; uint16_t w, h;

  if (screenEntered) {
    gfx->fillScreen(C_BLACK);
    // static labels
    gfx->setTextSize(2);
    gfx->setTextColor(C_GREY);
    gfx->setCursor(24, 96);  gfx->print("T");
    gfx->setCursor(24, 126); gfx->print("H");
    gfx->setCursor(24, 156); gfx->print("P");
  }

  clearField(0, 40, 240, 30);
  gfx->setTextSize(3);
  gfx->setTextColor(C_WHITE);
  gfx->getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  gfx->setCursor((240 - w) / 2, 44);
  gfx->print(timeStr);

  gfx->setTextSize(2);

  if (isnan(temperature)) strcpy(buf, "N/A"); else snprintf(buf, sizeof buf, "%.1f C", temperature);
  clearField(60, 96, 170, 18);
  gfx->setTextColor(C_RED);    gfx->setCursor(60, 96);  gfx->print(buf);

  if (isnan(humidity)) strcpy(buf, "N/A"); else snprintf(buf, sizeof buf, "%.0f %%", humidity);
  clearField(60, 126, 170, 18);
  gfx->setTextColor(C_CYAN);   gfx->setCursor(60, 126); gfx->print(buf);

  if (isnan(pressure)) strcpy(buf, "N/A"); else snprintf(buf, sizeof buf, "%.0f hPa", pressure);
  clearField(60, 156, 170, 18);
  gfx->setTextColor(C_YELLOW); gfx->setCursor(60, 156); gfx->print(buf);

  clearField(0, 196, 240, 12);
  gfx->setTextSize(1);

  const char *wifiTxt = (WiFi.status() == WL_CONNECTED) ? "WiFi OK" : "No WiFi";
  const char *sep     = "  |  ";
  const char *fbTxt   = (firebaseIdToken.length() > 0)  ? "FB OK"  : "FB ...";

  int16_t bx, by; uint16_t ww, hh, sw2, sh2, fw, fh;
  gfx->getTextBounds(wifiTxt, 0, 0, &bx, &by, &ww, &hh);
  gfx->getTextBounds(sep,     0, 0, &bx, &by, &sw2, &sh2);
  gfx->getTextBounds(fbTxt,   0, 0, &bx, &by, &fw, &fh);
  int startX = (240 - (int)(ww + sw2 + fw)) / 2;

  gfx->setCursor(startX, 196);
  gfx->setTextColor((WiFi.status() == WL_CONNECTED) ? C_GREEN : C_RED);
  gfx->print(wifiTxt);
  gfx->setTextColor(C_GREY);
  gfx->print(sep);
  gfx->setTextColor((firebaseIdToken.length() > 0) ? C_GREEN : C_ORANGE);
  gfx->print(fbTxt);
}

// ---- Screens 1-3: needle-less arc gauge ----
void drawGauge(const char *title, float value, float minV, float maxV,
               uint16_t colour, const char *unit) {
  const int   cx = 120, cy = 120;
  const int   rad = 106;
  const float A0 = 135.0f;
  const float SWEEP = 270.0f;

  int16_t x1, y1; uint16_t w, h;
  char buf[16];

  if (screenEntered) {
    gfx->fillScreen(C_BLACK);

    float f = (value - minV) / (maxV - minV);
    if (isnan(value)) f = 0;
    if (f < 0) f = 0; if (f > 1) f = 1;
    for (float a = 0; a <= SWEEP; a += 2.0f) {
      float ang = (A0 + a) * 0.017453293f;
      int x = cx + (int)(rad * cos(ang));
      int y = cy + (int)(rad * sin(ang));
      uint16_t c = (a <= SWEEP * f) ? colour : C_GREY;
      gfx->fillCircle(x, y, 4, c);
    }

    // name (hero)
    gfx->setTextSize(3);
    gfx->setTextColor(colour);
    gfx->getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    gfx->setCursor(cx - w / 2, cy - 34);
    gfx->print(title);

    // unit (static)
    gfx->setTextSize(2);
    gfx->setTextColor(C_GREY);
    gfx->getTextBounds(unit, 0, 0, &x1, &y1, &w, &h);
    gfx->setCursor(cx - w / 2, cy + 44);
    gfx->print(unit);

    gfx->setTextSize(1);
    gfx->setTextColor(C_GREY);
    char lbl[10];
    snprintf(lbl, sizeof lbl, "%.0f", minV);
    gfx->setCursor(62, 182); gfx->print(lbl);
    snprintf(lbl, sizeof lbl, "%.0f", maxV);
    gfx->setCursor(166, 182); gfx->print(lbl);
  }

  float f = (value - minV) / (maxV - minV);
  if (isnan(value)) f = 0;
  if (f < 0) f = 0; if (f > 1) f = 1;
  for (float a = 0; a <= SWEEP; a += 2.0f) {
    float ang = (A0 + a) * 0.017453293f;
    int x = cx + (int)(rad * cos(ang));
    int y = cy + (int)(rad * sin(ang));
    uint16_t c = (a <= SWEEP * f) ? colour : C_GREY;
    gfx->fillCircle(x, y, 4, c);
  }

  if (isnan(value)) strcpy(buf, "N/A");
  else snprintf(buf, sizeof buf, "%.1f", value);
  gfx->setTextSize(4);
  gfx->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  clearField(cx - 70, cy + 2, 140, 32);
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(cx - w / 2, cy + 2);
  gfx->print(buf);
}

// ---- Screen 4: stats ----
static void trendArrow(const String &tr, uint16_t &col, const char *&glyph) {
  if (tr == "Rising")      { col = C_RED;    glyph = "^"; }
  else if (tr == "Falling"){ col = C_CYAN;   glyph = "v"; }
  else                     { col = C_GREEN;  glyph = "="; }
}

void drawStatsScreen() {
  char buf[16];

  if (screenEntered) {
    gfx->fillScreen(C_BLACK);
    int16_t sx, sy; uint16_t sw, sh;
    gfx->setTextSize(2);
    gfx->setTextColor(C_YELLOW);
    gfx->getTextBounds("Stats", 0, 0, &sx, &sy, &sw, &sh);
    gfx->setCursor((240 - sw) / 2, 20); gfx->print("Stats");
    // static labels
    gfx->setTextColor(C_GREY);
    gfx->setCursor(24, 56);  gfx->print("AvgT");
    gfx->setCursor(24, 84);  gfx->print("AvgH");
    gfx->setCursor(24, 112); gfx->print("Dew");
    gfx->setCursor(24, 144); gfx->print("Trend");
    gfx->setTextSize(1);
    gfx->setCursor(24, 180); gfx->print("Up:");
  }

  gfx->setTextSize(2);

  if (isnan(avgTemperature)) strcpy(buf, "N/A"); else snprintf(buf, sizeof buf, "%.1fC", avgTemperature);
  clearField(120, 56, 110, 18);
  gfx->setTextColor(C_WHITE); gfx->setCursor(120, 56); gfx->print(buf);

  if (isnan(avgHumidity)) strcpy(buf, "N/A"); else snprintf(buf, sizeof buf, "%.0f%%", avgHumidity);
  clearField(120, 84, 110, 18);
  gfx->setTextColor(C_WHITE); gfx->setCursor(120, 84); gfx->print(buf);

  if (isnan(dewPointC)) strcpy(buf, "N/A"); else snprintf(buf, sizeof buf, "%.1fC", dewPointC);
  clearField(120, 112, 110, 18);
  gfx->setTextColor(C_WHITE); gfx->setCursor(120, 112); gfx->print(buf);

  uint16_t tcol; const char *tg;
  trendArrow(tempTrend, tcol, tg);
  clearField(120, 144, 118, 18);
  gfx->setTextColor(tcol); gfx->setCursor(120, 144);
  gfx->print(tg); gfx->print(" "); gfx->print(tempTrend);

  clearField(52, 180, 120, 10);
  gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(52, 180);
  gfx->print(millis() / 1000); gfx->print("s");
}
