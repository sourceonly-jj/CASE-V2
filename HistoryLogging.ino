// ==========================
// History / logging
// ==========================

void addToHistory(float t, float h, float p) {
  tempHistory[historyIndex] = t;
  humHistory[historyIndex] = h;
  presHistory[historyIndex] = p;

  historyIndex++;
  if (historyIndex >= HISTORY_SIZE) {
    historyIndex = 0;
    historyFilled = true;
  }
}

void logData(float t, float h, float p) {
  File file = SPIFFS.open("/log.csv", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open /log.csv");
    return;
  }
  file.printf("%s,%.1f,%.1f,%.1f\n", getFormattedDateTime().c_str(), t, h, p);
  file.close();
}

String getCSVLog() {
  File file = SPIFFS.open("/log.csv", FILE_READ);
  if (!file) return "DateTime,Temperature,Humidity,Pressure\n";
  String content;
  while (file.available()) {
    content += char(file.read());
  }
  file.close();
  return content;
}
