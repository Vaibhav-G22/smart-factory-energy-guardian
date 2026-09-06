/**
 * Smart Factory Energy Guardian - ESP32 DS18B20 + INA219 + ACS712 + DC Voltage Sensor
 * Reads temperature from DS18B20 (GPIO 4)
 * Reads voltage/current/power from INA219 (I2C: SDA=GPIO 21, SCL=GPIO 22)
 * Reads current from ACS712 (ADC: GPIO 34)
 * Reads DC voltage via voltage divider (ADC: GPIO 35)
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

// ADC pin for DC voltage measurement (via voltage divider)
#define DC_VOLTAGE_ADC_PIN 35

// ACS712 Calibration Constants
#define ACS712_SENSITIVITY 0.185      // V/A (185 mV/A for 5A version)
#define ACS712_OFFSET 2.5             // Midpoint voltage at 0A (2.5V)
#define ADC_REFERENCE 3.3             // ESP32 ADC reference voltage
#define ADC_RESOLUTION 4096           // 12-bit ADC (0-4095)
#define ACS712_SAMPLES 10             // Number of samples to average

// DC Voltage Divider Configuration
// WARNING: Do NOT connect to mains voltage!
// Safe DC voltage range: 0-30V recommended
// Voltage divider: R1 (top) = 10kΩ, R2 (bottom) = 10kΩ for 0-6.6V input
// For other ranges: MAX_INPUT_VOLTAGE = ADC_REFERENCE * (R1 + R2) / R2
#define R1_OHMS 10000                 // Top resistor (Ω)
#define R2_OHMS 10000                 // Bottom resistor (Ω)
#define MAX_INPUT_VOLTAGE 6.6          // Maximum safe input voltage (V)
#define DC_VOLTAGE_SAMPLES 10         // Number of samples to average

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
  
  // Initialize DC Voltage Sensor
  Serial.println("[INIT] Initializing DC Voltage Sensor (ADC GPIO 35)...");
  pinMode(DC_VOLTAGE_ADC_PIN, INPUT);
  Serial.println("[OK] DC Voltage ADC configured");
  Serial.println("[OK] Voltage divider: R1=" + String(R1_OHMS) + "Ω, R2=" + String(R2_OHMS) + "Ω");
  Serial.println("[OK] Maximum safe input voltage: " + String(MAX_INPUT_VOLTAGE, 1) + " V");
  Serial.println("[OK] WARNING: Do NOT connect to mains voltage!\n");
  
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

// Function to read DC voltage via voltage divider
float readDCVoltage() {
  float voltageSum = 0.0;
  
  // Read multiple samples and average
  for (int i = 0; i < DC_VOLTAGE_SAMPLES; i++) {
    int rawADC = analogRead(DC_VOLTAGE_ADC_PIN);
    
    // Convert ADC value (0-4095) to voltage at divider output (0-3.3V)
    float adcVoltage = (rawADC / (float)ADC_RESOLUTION) * ADC_REFERENCE;
    
    // Calculate input voltage using voltage divider formula:
    // V_input = V_adc * (R1 + R2) / R2
    float inputVoltage = adcVoltage * ((R1_OHMS + R2_OHMS) / (float)R2_OHMS);
    
    voltageSum += inputVoltage;
    
    delay(5);  // Small delay between samples
  }
  
  // Calculate average voltage
  float avgVoltage = voltageSum / DC_VOLTAGE_SAMPLES;
  
  // Safety check: warn if voltage exceeds maximum
  if (avgVoltage > MAX_INPUT_VOLTAGE) {
    Serial.println("[WARN] DC Voltage exceeds maximum! Check divider resistors.");
  }
  
  return avgVoltage;
}

void loop() {
  // Read DC voltage
  float voltage_V = readDCVoltage();
  
  // Read ACS712 current sensor
  float current_A = readACS712Current();
  
  // Calculate power: P = V × I
  float power_W = voltage_V * current_A;
  
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
  
  // Print all readings with calculated power
  Serial.print("DC Voltage: ");
  Serial.print(voltage_V, 2);
  Serial.print(" V | ACS712 Current: ");
  Serial.print(current_A, 3);
  Serial.print(" A | Calculated Power: ");
  Serial.print(power_W, 3);
  Serial.print(" W | Temperature: ");
  
  if (tempC == -999.0) {
    Serial.println("ERROR");
  } else {
    Serial.print(tempC, 2);
    Serial.println(" °C");
  }
  
  // Optional: Also print INA219 readings
  float ina_voltage_V = ina219.getBusVoltage_V();
  float ina_current_mA = ina219.getCurrent_mA();
  float ina_power_mW = ina219.getPower_mW();
  
  Serial.print("INA219 - Voltage: ");
  Serial.print(ina_voltage_V, 2);
  Serial.print(" V | Current: ");
  Serial.print(ina_current_mA, 2);
  Serial.print(" mA | Power: ");
  Serial.print(ina_power_mW, 2);
  Serial.println(" mW");
  
  Serial.println();  // Blank line for readability
  delay(2000);  // Read every 2 seconds
}
