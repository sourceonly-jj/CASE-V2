// ==========================
// Helpers
// ==========================

String jsonQuotedValue(const String& json, const String& key) {
  int keyPos = json.indexOf("\"" + key + "\"");
  if (keyPos == -1) return "";
  int colonPos = json.indexOf(':', keyPos);
  if (colonPos == -1) return "";
  int firstQuote = json.indexOf('\"', colonPos + 1);
  if (firstQuote == -1) return "";
  int secondQuote = json.indexOf('\"', firstQuote + 1);
  if (secondQuote == -1) return "";
  return json.substring(firstQuote + 1, secondQuote);
}

String getFormattedDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 2000)) return "time_unavailable";
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

bool getTimeString(char* out, size_t outSize) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 2000)) return false;
  strftime(out, outSize, "%H:%M:%S", &timeinfo);
  return true;
}

String getLocalIPString() {
  if (WiFi.status() != WL_CONNECTED) return "Not connected";
  return WiFi.localIP().toString();
}

String buildHistoryJSON(float arr[]) {
  String json = "[";
  int count = historyFilled ? HISTORY_SIZE : historyIndex;
  for (int i = 0; i < count; i++) {
    int idx = historyFilled ? (historyIndex + i) % HISTORY_SIZE : i;
    json += String(arr[idx], 1);
    if (i < count - 1) json += ",";
  }
  json += "]";
  return json;
}

float getHistoryMin(float arr[]) {
  int count = historyFilled ? HISTORY_SIZE : historyIndex;
  if (count == 0) return NAN;
  int startIdx = historyFilled ? historyIndex % HISTORY_SIZE : 0;
  float minVal = arr[startIdx];
  for (int i = 1; i < count; i++) {
    int idx = historyFilled ? (historyIndex + i) % HISTORY_SIZE : i;
    if (arr[idx] < minVal) minVal = arr[idx];
  }
  return minVal;
}

float getHistoryMax(float arr[]) {
  int count = historyFilled ? HISTORY_SIZE : historyIndex;
  if (count == 0) return NAN;
  int startIdx = historyFilled ? historyIndex % HISTORY_SIZE : 0;
  float maxVal = arr[startIdx];
  for (int i = 1; i < count; i++) {
    int idx = historyFilled ? (historyIndex + i) % HISTORY_SIZE : i;
    if (arr[idx] > maxVal) maxVal = arr[idx];
  }
  return maxVal;
}

float calculateAverage(float arr[]) {
  int count = historyFilled ? HISTORY_SIZE : historyIndex;
  if (count == 0) return NAN;
  float sum = 0;
  for (int i = 0; i < count; i++) {
    int idx = historyFilled ? (historyIndex + i) % HISTORY_SIZE : i;
    sum += arr[idx];
  }
  return sum / count;
}

float calculateDewPoint(float T, float H) {
  if (isnan(T) || isnan(H) || H <= 0) return NAN;
  const float a = 17.27;
  const float b = 237.7;
  float alpha = ((a * T) / (b + T)) + log(H / 100.0);
  return (b * alpha) / (a - alpha);
}

String calculateTrend(float arr[]) {
  int count = historyFilled ? HISTORY_SIZE : historyIndex;
  if (count < 6) return "Steady";
  int newestIdx = historyFilled ? (historyIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE : (count - 1);
  int olderIdx  = historyFilled ? (historyIndex - 6 + HISTORY_SIZE) % HISTORY_SIZE : max(0, count - 6);
  float diff = arr[newestIdx] - arr[olderIdx];
  if (diff > 0.5) return "Rising";
  if (diff < -0.5) return "Falling";
  return "Steady";
}

String formatUptime(unsigned long ms) {
  unsigned long totalSeconds = ms / 1000;
  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "%lud %02lu:%02lu:%02lu", days, hours, minutes, seconds);
  return String(buf);
}

void updateDerivedData() {
  avgTemperature = calculateAverage(tempHistory);
  avgHumidity = calculateAverage(humHistory);
  avgPressure = calculateAverage(presHistory);
  dewPointC = calculateDewPoint(temperature, humidity);
  tempTrend = calculateTrend(tempHistory);
  humTrend = calculateTrend(humHistory);
  presTrend = calculateTrend(presHistory);
}
