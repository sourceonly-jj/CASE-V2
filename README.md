# CASE — Compact Ambient Sensing Environment

An ESP32-C3 environmental monitor that reads temperature, humidity, and barometric
pressure, displays them on a round LCD, logs history locally, serves a live web
dashboard, and syncs readings to Firebase. Network I/O runs on a dedicated FreeRTOS
task so the display and web server stay responsive during blocking uploads.

## Hardware

| Component | Part | Interface |
|-----------|------|-----------|
| MCU | ESP32-C3 SuperMini | — |
| Display | GC9A01 240×240 round IPS | SPI |
| Temp / humidity | AHT20 | I²C (`0x38`) |
| Pressure | BMP280 | I²C (`0x77`) |
| Charging | TP4056 | — |
| Regulation | HT7333 (3.3 V LDO) | — |
| Power | Single-cell Li-ion / LiPo | — |

## Wiring

Display (SPI):

| GC9A01 | ESP32-C3 |
|--------|----------|
| SCL | GPIO4 |
| SDA | GPIO6 |
| RES | GPIO10 |
| DC | GPIO5 |
| CS | GPIO7 |
| BLK | GPIO3 |
| VCC | 3V3 |
| GND | GND |

Sensors (shared I²C bus):

| Signal | ESP32-C3 |
|--------|----------|
| SDA | GPIO8 |
| SCL | GPIO9 |
| VCC | 3V3 |
| GND | GND |

The AHT20 and BMP280 share one I²C bus at addresses `0x38` and `0x77`.

## Features

- Three live measurements: temperature, relative humidity, barometric pressure.
- Round-display UI with five auto-rotating screens: a combined readout, per-channel
  arc gauges (temperature, humidity, pressure), and a statistics screen showing
  running averages, dew point, and a rise/fall/steady trend indicator.
- 60-sample rolling history buffer per channel, feeding both the on-device gauges
  and the web dashboard charts.
- Local CSV logging to SPIFFS.
- Live web dashboard served from the device, refreshing every 3 seconds, with a
  JSON endpoint (`/data`) and a CSV download endpoint (`/download`).
- Firebase Realtime Database sync (anonymous auth) for current readings and history.
- NTP-synchronised clock.

## Architecture

The firmware is a multi-file Arduino sketch. Responsibilities are split across tabs:

| File | Responsibility |
|------|----------------|
| `WeatherStation.ino` | Globals, `setup()`, main loop, FreeRTOS network task |
| `Sensor.ino` | AHT20 + BMP280 acquisition |
| `DisplayScreens.ino` | GC9A01 rendering (readout, gauges, stats) |
| `Helpers.ino` | Averages, dew point, trend, history serialisation |
| `HistoryLogging.ino` | Rolling buffer + SPIFFS CSV |
| `WifiTime.ino` | WiFi connection and NTP sync |
| `Firebase.ino` | Anonymous authentication |
| `FirebaseDatabase.ino` | Realtime Database uploads |
| `WebServerPage.ino` | HTTP server, dashboard, `/data` and `/download` |

### Concurrency model

Firebase uploads and WiFi reconnection are blocking operations. In a single
`loop()` they stall everything else — an upload can freeze the display and the web
server for the duration of the HTTP/TLS exchange.

Network I/O therefore runs on a dedicated FreeRTOS task. The main loop keeps the
sensor reads, display rendering, and web-server handling; the network task owns WiFi
reconnection, Firebase authentication, and uploads. Because the ESP32-C3 is
single-core, this provides concurrency via time-slicing rather than true
parallelism — the display keeps updating during a network stall instead of blocking
on it.

Shared sensor state (current readings and history) is protected by a mutex using a
snapshot pattern: a task takes the mutex, copies the values it needs into locals,
releases the mutex, then performs the slow network or I²C work on the copies. The
mutex is never held across a blocking call, which avoids both data races and
priority-inversion stalls.

Display rendering is also decoupled from the loop rate. Static layout is drawn once
on screen entry; only changed values are repainted, each within its own cleared
rectangle rather than by clearing the whole frame. This removes redraw flicker.

## Build

Requires the Arduino IDE with the ESP32 board package.

Libraries:

- GFX Library for Arduino (moononournation)
- Adafruit AHTX0
- Adafruit BMP280 Library
- Adafruit Unified Sensor

Steps:

1. Copy `secrets.example.h` to `secrets.h` and fill in WiFi and Firebase
   credentials.
2. Open `WeatherStation.ino` in the Arduino IDE. All tabs must sit in a folder named
   `WeatherStation`.
3. Select the ESP32-C3 board, enable **USB CDC On Boot**, and upload.

`secrets.h` is git-ignored; credentials are never committed.

## Configuration

Key timing intervals (in `WeatherStation.ino`):

| Interval | Value |
|----------|-------|
| Sensor read | 2 s |
| Display redraw | 0.5 s |
| Screen rotation | 6 s |
| Firebase upload | 20 s |
| CSV log | 60 s |
| Auth refresh | 50 min |

Gauge ranges use each sensor's rated measurement span. History depth is 60 samples
per channel.

## Endpoints

| Route | Response |
|-------|----------|
| `/` | Dashboard (HTML) |
| `/data` | Current readings + history (JSON) |
| `/download` | Full CSV log |

## License

MIT
