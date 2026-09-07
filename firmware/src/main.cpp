/**
 * Smart Factory Energy Guardian - ESP32 Simplified Firmware
 * ESP32 + INA219 (I2C voltage/current/power) + DS18B20 (OneWire temperature)
 * 
 * Sensors:
 * - INA219: I2C (SDA=GPIO21, SCL=GPIO22) - voltage, current, power measurement
 * - DS18B20: OneWire (GPIO4) - temperature measurement
 * 
 * Output: Serial Monitor at 115200 baud every 2 seconds
 */

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin Definitions
#define DS18B20_PIN 4        // GPIO4 - DS18B20 OneWire data line
#define INA219_SDA 21        // GPIO21 - I2C SDA
#define INA219_SCL 22        // GPIO22 - I2C SCL

// Initialize sensors
Adafruit_INA219 ina219;
OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Smart Factory Energy Guardian ===");
  Serial.println("ESP32 + INA219 + DS18B20");
  Serial.println("Initialization started...\n");
  
  // Initialize I2C
  Serial.println("[INIT] Configuring I2C (SDA: GPIO21, SCL: GPIO22)");
  Wire.begin(INA219_SDA, INA219_SCL);
  Wire.setClock(400000);  // 400kHz I2C clock
  delay(500);
  
  // Initialize INA219
  Serial.println("[INIT] Initializing INA219...");
  if (!ina219.begin()) {
    Serial.println("[ERROR] INA219 not found! Check I2C connections.");
    Serial.println("[ERROR] Expected address: 0x40");
  } else {
    Serial.println("[OK] INA219 connected (I2C address: 0x40)");
    Serial.println("[OK] Range: 32V, 2A\n");
  }
  
  // Initialize DS18B20
  Serial.println("[INIT] Initializing DS18B20 Temperature Sensor...");
  tempSensor.begin();
  
  int deviceCount = tempSensor.getDeviceCount();
  if (deviceCount == 0) {
    Serial.println("[ERROR] DS18B20 not found! Check OneWire connection on GPIO4.");
  } else {
    Serial.println("[OK] DS18B20 connected");
    Serial.println("[OK] Found " + String(deviceCount) + " device(s)");
    tempSensor.setResolution(12);  // 12-bit resolution (0.0625°C)
    Serial.println("[OK] Resolution: 12-bit\n");
  }
  
  Serial.println("=== Sensor Readings (every 2 seconds) ===\n");
}

void loop() {
  // Read INA219
  float voltage_V = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();
  
  // Read DS18B20
  tempSensor.requestTemperatures();
  delay(100);  // Wait for temperature conversion
  float tempC = tempSensor.getTempCByIndex(0);
  
  // Validate temperature reading
  if (tempC == -127.0 || tempC == 85.0 || tempC < -50 || tempC > 125) {
    tempC = -999.0;  // Error indicator
  }
  
  // Print readings
  Serial.print("Voltage: ");
  Serial.print(voltage_V, 2);
  Serial.print(" V | Current: ");
  Serial.print(current_mA, 2);
  Serial.print(" mA | Power: ");
  Serial.print(power_mW, 2);
  Serial.print(" mW | Temperature: ");
  
  if (tempC == -999.0) {
    Serial.println("ERROR");
  } else {
    Serial.print(tempC, 2);
    Serial.println(" °C");
  }
  
  delay(2000);  // 2-second interval
}
