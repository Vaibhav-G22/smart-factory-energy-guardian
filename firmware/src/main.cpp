/**
 * Smart Factory Energy Guardian - ESP32 Test Program
 * Minimal sketch to verify ESP32 setup
 */

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  Serial.println("Smart Factory Energy Guardian");
  Serial.println("ESP32 online");
  delay(2000);
}
