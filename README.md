# ESP32 GPS Tracker with Speed & Crash Detection

An IoT vehicle tracker built on the **ESP32**. It reads location from a **GPS module**, calculates speed, detects sudden impacts with an **MPU6050** accelerometer, and sends the data every 5 seconds over Wi-Fi to the **GeoLinker** cloud map (CircuitDigest).

## Demo video

[▶ Watch the demo video](media/demo-video.mp4)

## Features

- Live GPS position (falls back to a default location when there is no fix)
- Speed calculated from consecutive GPS points (Haversine formula)
- Movement status: `Not Moving` (< 3 km/h), `Normal` (≤ 50 km/h), `OverSpeed` (> 50 km/h)
- Crash detection when acceleration on any axis changes by more than 5 m/s²
- JSON payload posted to GeoLinker over HTTPS

## Hardware

| Component | ESP32 pins |
|-----------|-----------|
| GPS module (e.g. NEO-6M) | RX = GPIO16, TX = GPIO17 (UART2, 9600 baud) |
| MPU6050 accelerometer | SDA = GPIO21, SCL = GPIO22 |

## Libraries (Arduino IDE)

- `TinyGPSPlus`
- `MPU6050` (by Electronic Cats / i2cdevlib)
- `WiFi`, `HTTPClient`, `Wire` (included with the ESP32 board package)

## Setup

1. Open `firmware/gps_crash_tracker/gps_crash_tracker.ino` in the Arduino IDE.
2. Copy `secrets.example.h` to `secrets.h` in the same folder and fill in your Wi-Fi name, password and GeoLinker API key.
3. Select your ESP32 board and port, then upload.
4. Open the Serial Monitor at **115200 baud** to see the payload and server responses.

## Example payload

```json
{
  "device_id": "BAS KTP/KPZ",
  "timestamp": ["2026-01-10 12:00:00"],
  "lat": [2.989945],
  "long": [101.705803],
  "payload": [{ "SpeedValue": 0.0, "MovingStatus": "Not Moving", "CrashStatus": "Normal" }]
}
```

## Project structure

```
firmware/gps_crash_tracker/
  gps_crash_tracker.ino   main sketch
  secrets.example.h       template for Wi-Fi / API credentials
media/
  demo-video.mp4          project demo
```
