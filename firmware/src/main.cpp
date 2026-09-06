/**
 * Smart Factory Energy Guardian - ESP32 DS18B20 Sensor Test
 * Reads temperature from DS18B20 sensor on GPIO 4
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO pin for DS18B20 data line (OneWire)
#define DS18B20_PIN 4

// Initialize OneWire and DallasTemperature libraries
OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\nSmart Factory Energy Guardian");
  Serial.println("ESP32 online");
  Serial.println("\n[INIT] Initializing DS18B20 Temperature Sensor (GPIO 4)...");
  
  // Initialize temperature sensor
  tempSensor.begin();
  
  int deviceCount = tempSensor.getDeviceCount();
  if (deviceCount == 0) {
    Serial.println("[ERROR] DS18B20 not found! Check wiring on GPIO 4");
  } else {
    Serial.println("[OK] DS18B20 initialized");
    Serial.println("[OK] Found " + String(deviceCount) + " sensor(s)");
    // Set resolution to 12-bit (highest accuracy)
    tempSensor.setResolution(12);
    Serial.println("[OK] Sensor resolution set to 12-bit\n");
  }
}

void loop() {
  // Request temperature conversion
  tempSensor.requestTemperatures();
  delay(100);  // Wait for conversion to complete
  
  // Read temperature in Celsius
  float tempC = tempSensor.getTempCByIndex(0);
  
  // Print to Serial Monitor
  Serial.print("Temperature: ");
  
  // Check for valid reading
  if (tempC == -127.0 || tempC == 85.0) {
    Serial.println("ERROR (check connection)");
  } else if (tempC < -50 || tempC > 125) {
    Serial.println("ERROR (out of range)");
  } else {
    Serial.print(tempC, 2);  // 2 decimal places
    Serial.println(" °C");
  }
  
  delay(2000);  // Read every 2 seconds
}
