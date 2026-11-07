#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <MPU6050.h>

// ===================== Wi-Fi and GeoLinker =====================
// Credentials live in secrets.h (not committed to Git).
// Copy secrets.example.h to secrets.h and fill in your own values.
#include "secrets.h"

const char* ssid        = WIFI_SSID;
const char* password    = WIFI_PASSWORD;
const char* GEO_API_URL = "https://www.circuitdigest.cloud/geolinker";
const char* GEO_API_KEY = GEOLINKER_API_KEY;

// ===================== Hardware Setup =====================
HardwareSerial GPSSerial(2); // GPS serial
TinyGPSPlus gps;
MPU6050 mpu;

// ===================== GPS Tracking Variables =====================
float prevLat = 0, prevLng = 0;
unsigned long prevTime = 0;

// ===================== Helper Functions =====================
float calculateSpeed(float lat1, float lon1, float lat2, float lon2, float timeSeconds) {
  if (timeSeconds <= 0) return 0;
  const float R = 6371.0; // km
  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);
  float a = sin(dLat / 2) * sin(dLat / 2) +
            cos(radians(lat1)) * cos(radians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  float distance = R * c; // km
  return distance / (timeSeconds / 3600.0); // km/h
}

String speedText(float speed) {
  if (speed < 3.0) return "Not Moving";
  else if (speed <= 50.0) return "Normal";
  else return "OverSpeed";
}

String statusText(float ax, float ay, float az) {
  float threshold = 5.0; // sudden movement threshold
  if (abs(ax) > threshold || abs(ay) > threshold || abs(az - 9.8) > threshold) return "Crash";
  return "Normal";
}

// ===================== Setup =====================
void setup() {
  Serial.begin(115200);

  // GPS Serial
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17

  // MPU6050 I2C
  Wire.begin(21, 22);
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected");
  } else {
    Serial.println("MPU6050 connection failed!");
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
}

// ===================== Loop =====================
void loop() {
  // -------- Read GPS --------
  while (GPSSerial.available() > 0) gps.encode(GPSSerial.read());
  float lat = gps.location.isValid() ? gps.location.lat() : 2.989945;
  float lng = gps.location.isValid() ? gps.location.lng() : 101.705803;

  unsigned long currentTime = millis();
  float speedKmh = 0;
  if (prevLat != 0 && prevLng != 0) {
    speedKmh = calculateSpeed(prevLat, prevLng, lat, lng, (currentTime - prevTime) / 1000.0);
  }
  prevLat = lat;
  prevLng = lng;
  prevTime = currentTime;

  // -------- Read MPU6050 --------
  int16_t ax_raw, ay_raw, az_raw;
  mpu.getAcceleration(&ax_raw, &ay_raw, &az_raw);
  float ax = ax_raw / 16384.0 * 9.8;
  float ay = ay_raw / 16384.0 * 9.8;
  float az = az_raw / 16384.0 * 9.8;

  String movingStatus = speedText(speedKmh);
  String crashStatus = statusText(ax, ay, az);

  // -------- Prepare JSON Payload --------
  String payload = "{";
  payload += "\"device_id\":\"BAS KTP/KPZ\",";
  payload += "\"timestamp\":[\"2026-01-10 12:00:00\"],"; // Replace with GPS time if available
  payload += "\"lat\":[" + String(lat, 6) + "],";
  payload += "\"long\":[" + String(lng, 6) + "],";
  payload += "\"payload\":[{";
  payload += "\"SpeedValue\":" + String(speedKmh, 1) + ",";
  payload += "\"MovingStatus\":\"" + movingStatus + "\",";
  payload += "\"CrashStatus\":\"" + crashStatus + "\"";
  payload += "}]}";

  // -------- PRINT payload to Serial --------
  Serial.println("\n=== Payload to GeoLinker ===");
  Serial.println(payload);

  // -------- Send HTTP POST to GeoLinker --------
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(GEO_API_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", GEO_API_KEY);

    int httpResponse = http.POST(payload);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponse);

    if (httpResponse > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error sending POST");
    }
    http.end();
  }

  delay(5000); // Send every 5 seconds
}
