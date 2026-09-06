/**
 * Smart Factory Energy Guardian - ESP32 DS18B20 + INA219 + ACS712 Sensor Test
 * Reads temperature from DS18B20 (GPIO 4)
 * Reads voltage/current/power from INA219 (I2C: SDA=GPIO 21, SCL=GPIO 22)
 * Reads current from ACS712 (ADC: GPIO 34)
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

// ADC pin for ACS712 current sensor
#define ACS712_ADC_PIN 34

// ACS712 Calibration Constants
#define ACS712_SENSITIVITY 0.185      // V/A (185 mV/A for 5A version)
#define ACS712_OFFSET 2.5             // Midpoint voltage at 0A (2.5V)
#define ADC_REFERENCE 3.3             // ESP32 ADC reference voltage
#define ADC_RESOLUTION 4096           // 12-bit ADC (0-4095)
#define ACS712_SAMPLES 10             // Number of samples to average

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
    Serial.println("[OK] INA219 configured for 32V, 2A range\n");
  }
  
  // Initialize ACS712
  Serial.println("[INIT] Initializing ACS712 Current Sensor (ADC GPIO 34)...");
  pinMode(ACS712_ADC_PIN, INPUT);
  analogSetAttenuation(ADC_11db);  // 0-3.3V range on ESP32
  Serial.println("[OK] ACS712 ADC configured");
  Serial.println("[OK] ACS712 sensitivity: " + String(ACS712_SENSITIVITY, 3) + " V/A");
  Serial.println("[OK] ACS712 offset: " + String(ACS712_OFFSET, 2) + " V\n");
  
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

// Function to read ACS712 current sensor
float readACS712Current() {
  float voltageSum = 0.0;
  
  // Read multiple samples and average
  for (int i = 0; i < ACS712_SAMPLES; i++) {
    int rawADC = analogRead(ACS712_ADC_PIN);
    
    // Convert ADC value (0-4095) to voltage (0-3.3V)
    float voltage = (rawADC / (float)ADC_RESOLUTION) * ADC_REFERENCE;
    voltageSum += voltage;
    
    delay(5);  // Small delay between samples
  }
  
  // Calculate average voltage
  float avgVoltage = voltageSum / ACS712_SAMPLES;
  
  // Convert voltage to current using ACS712 formula: I = (V - Offset) / Sensitivity
  float current_A = (avgVoltage - ACS712_OFFSET) / ACS712_SENSITIVITY;
  
  // Filter out noise at zero crossing (values less than 0.05A)
  if (abs(current_A) < 0.05) {
    current_A = 0.0;
  }
  
  return current_A;
}

void loop() {
  // Read DS18B20 temperature
  tempSensor.requestTemperatures();
  delay(100);  // Wait for conversion to complete
  
  float tempC = tempSensor.getTempCByIndex(0);
  
  // Validate temperature reading
  if (tempC == -127.0 || tempC == 85.0) {
    tempC = -999.0;  // Error code
  } else if (tempC < -50 || tempC > 125) {
    tempC = -999.0;  // Out of range
  }
  
  // Read ACS712 current sensor
  float current_A = readACS712Current();
  
  // Read INA219 measurements
  float voltage_V = ina219.getBusVoltage_V();
  float ina_current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();
  
  // Print all readings
  Serial.print("DS18B20 - Temperature: ");
  if (tempC == -999.0) {
    Serial.print("ERROR");
  } else {
    Serial.print(tempC, 2);
    Serial.print(" °C");
  }
  Serial.print(" | ");
  
  Serial.print("ACS712 - Current: ");
  Serial.print(current_A, 3);
  Serial.print(" A (");
  Serial.print(current_A * 1000, 1);
  Serial.println(" mA)");
  
  // Optional: Also print INA219 readings
  Serial.print("INA219 - Voltage: ");
  Serial.print(voltage_V, 2);
  Serial.print(" V | Current: ");
  Serial.print(ina_current_mA, 2);
  Serial.print(" mA | Power: ");
  Serial.print(power_mW, 2);
  Serial.println(" mW");
  
  Serial.println();  // Blank line for readability
  delay(2000);  // Read every 2 seconds
}
