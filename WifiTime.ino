// ==========================
// ==========================

void syncTime() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  gfx->fillScreen(C_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(30, 110);
  gfx->println("WiFi...");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov", "time.google.com");
  }
}

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  firebaseIdToken = "";
  firebaseRefreshToken = "";
  firebaseLocalId = "";

  wifiReconnectCount++;

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov", "time.google.com");
  }
}
