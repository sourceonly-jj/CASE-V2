// ==========================
// ==========================

void readSensor() {
  float t = NAN, h = NAN, p = NAN;

  if (ahtOK) {
    sensors_event_t hEvt, tEvt;
    aht.getEvent(&hEvt, &tEvt);
    t = tEvt.temperature;
    h = hEvt.relative_humidity;
  }
  if (bmpOK) {
    p = bmp.readPressure() / 100.0f;   // Pa -> hPa
  }

  bool ok = (!isnan(t) && !isnan(h));

  if (ok) {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      temperature = t;
      humidity = h;
      pressure = p;
      addToHistory(t, h, isnan(p) ? 0 : p);
      lastSuccessfulReadMillis = millis();
      updateDerivedData();
      xSemaphoreGive(dataMutex);
    }
  } else {
    sensorErrorCount++;
    Serial.println("Sensor read failed");
  }

  if (!getTimeString(timeStr, sizeof(timeStr))) {
    strcpy(timeStr, "--:--:--");
  }

  Serial.printf(
    "Time: %s | T: %.1fC | H: %.1f%% | P: %.1fhPa | AvgT: %.1f | Dew: %.1f\n",
    timeStr, temperature, humidity, pressure, avgTemperature, dewPointC
  );
}
