/**
 * Smart Factory Energy Guardian - ESP32 DS18B20 + INA219 Sensor Test
 * Reads temperature from DS18B20 (GPIO 4)
 * Reads voltage/current/power from INA219 (I2C: SDA=GPIO 21, SCL=GPIO 22)
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// GPIO pin for DS18B20 data line (OneWire)
#define DS18B20_PIN 4

// I2C pins for INA219 (default: SDA=GPIO 21, SCL=GPIO 22)
#define INA219_SDA 21
#define INA219_SCL 22

// Initialize OneWire and DallasTemperature libraries
OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

// Initialize INA219 (I2C address 0x40 by default)
Adafruit_INA219 ina219;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\nSmart Factory Energy Guardian");
  Serial.println("ESP32 online");
  
  // Initialize I2C for INA219
  Serial.println("\n[INIT] Initializing I2C (SDA: GPIO 21, SCL: GPIO 22)...");
  Wire.begin(INA219_SDA, INA219_SCL);
  Wire.setClock(400000);  // 400kHz I2C clock
  delay(500);
  
  // Initialize INA219
  Serial.println("[INIT] Initializing INA219 Current/Voltage Monitor...");
  if (!ina219.begin()) {
    Serial.println("[ERROR] INA219 not found! Check I2C wiring (SDA: GPIO 21, SCL: GPIO 22)");
    Serial.println("[ERROR] Expected I2C address: 0x40");
  } else {
    Serial.println("[OK] INA219 initialized (I2C 0x40)");
    // Configure for DC current measurement (default range)
    Serial.println("[OK] INA219 configured for 32V, 2A range\n");
  }
  
  // Initialize DS18B20
  Serial.println("[INIT] Initializing DS18B20 Temperature Sensor (GPIO 4)...");
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
  
  Serial.println("=== Sensor Readings (updating every 2 seconds) ===\n");
}

void loop() {
  // Read INA219 measurements
  float voltage_V = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();
  
  // Print INA219 readings
  Serial.print("INA219 - Voltage: ");
  Serial.print(voltage_V, 2);
  Serial.print(" V | Current: ");
  Serial.print(current_mA, 2);
  Serial.print(" mA | Power: ");
  Serial.print(power_mW, 2);
  Serial.println(" mW");
  
  // Request temperature conversion
  tempSensor.requestTemperatures();
  delay(100);  // Wait for conversion to complete
  
  // Read temperature in Celsius
  float tempC = tempSensor.getTempCByIndex(0);
  
  // Print temperature reading
  Serial.print("DS18B20 - Temperature: ");
  
  // Check for valid reading
  if (tempC == -127.0 || tempC == 85.0) {
    Serial.println("ERROR (check connection)");
  } else if (tempC < -50 || tempC > 125) {
    Serial.println("ERROR (out of range)");
  } else {
    Serial.print(tempC, 2);  // 2 decimal places
    Serial.println(" °C");
  }
  
  Serial.println();  // Blank line for readability
  delay(2000);  // Read every 2 seconds
}
