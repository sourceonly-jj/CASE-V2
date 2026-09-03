// ==========================
// Firebase database writes
// ==========================

void uploadCurrentToFirebase(float t, float h) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (firebaseIdToken == "") {
    Serial.println("No Firebase auth token");
    failedUploads++;
    return;
  }
  if (isnan(t) || isnan(h)) return;

  HTTPClient http;

  String url = String(FIREBASE_HOST) + "/weather/current.json?auth=" + firebaseIdToken;

  String payload = "{";
  payload += "\"temperature\":" + String(t, 1) + ",";
  payload += "\"humidity\":" + String(h, 1) + ",";
  payload += "\"pressure\":" + (isnan(pressure) ? String("null") : String(pressure, 1)) + ",";
  payload += "\"avgTemperature\":" + (isnan(avgTemperature) ? String("null") : String(avgTemperature, 1)) + ",";
  payload += "\"avgHumidity\":" + (isnan(avgHumidity) ? String("null") : String(avgHumidity, 1)) + ",";
  payload += "\"avgPressure\":" + (isnan(avgPressure) ? String("null") : String(avgPressure, 1)) + ",";
  payload += "\"dewPoint\":" + (isnan(dewPointC) ? String("null") : String(dewPointC, 1)) + ",";
  payload += "\"tempTrend\":\"" + tempTrend + "\",";
  payload += "\"humTrend\":\"" + humTrend + "\",";
  payload += "\"presTrend\":\"" + presTrend + "\",";
  payload += "\"time\":\"" + String(timeStr) + "\",";
  payload += "\"datetime\":\"" + getFormattedDateTime() + "\",";
  payload += "\"ip\":\"" + getLocalIPString() + "\",";
  payload += "\"uid\":\"" + firebaseLocalId + "\",";
  payload += "\"uptime\":\"" + formatUptime(millis()) + "\"";
  payload += "}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.PUT(payload);

  Serial.print("Firebase current PUT code: ");
  Serial.println(httpCode);

  if (httpCode > 0 && httpCode < 300) {
    Serial.println(http.getString());
    successfulUploads++;
    lastSuccessfulUploadMillis = millis();
  } else {
    Serial.println("Firebase current upload failed");
    failedUploads++;
  }

  http.end();
}

void pushHistoryToFirebase(float t, float h) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (firebaseIdToken == "") {
    Serial.println("No Firebase auth token");
    return;
  }
  if (isnan(t) || isnan(h)) return;

  HTTPClient http;

  String url = String(FIREBASE_HOST) + "/weather/history.json?auth=" + firebaseIdToken;

  String payload = "{";
  payload += "\"temperature\":" + String(t, 1) + ",";
  payload += "\"humidity\":" + String(h, 1) + ",";
  payload += "\"pressure\":" + (isnan(pressure) ? String("null") : String(pressure, 1)) + ",";
  payload += "\"avgTemperature\":" + (isnan(avgTemperature) ? String("null") : String(avgTemperature, 1)) + ",";
  payload += "\"avgHumidity\":" + (isnan(avgHumidity) ? String("null") : String(avgHumidity, 1)) + ",";
  payload += "\"avgPressure\":" + (isnan(avgPressure) ? String("null") : String(avgPressure, 1)) + ",";
  payload += "\"dewPoint\":" + (isnan(dewPointC) ? String("null") : String(dewPointC, 1)) + ",";
  payload += "\"tempTrend\":\"" + tempTrend + "\",";
  payload += "\"humTrend\":\"" + humTrend + "\",";
  payload += "\"presTrend\":\"" + presTrend + "\",";
  payload += "\"time\":\"" + String(timeStr) + "\",";
  payload += "\"datetime\":\"" + getFormattedDateTime() + "\"";
  payload += "}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);

  Serial.print("Firebase history POST code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println(http.getString());
  } else {
    Serial.println("Firebase history upload failed");
  }

  http.end();
}
