// I2C Scanner Test for Arduino Nano ESP32
// This will scan for all I2C devices and report their addresses

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n=== I2C Scanner Test ===");
  Serial.println("Arduino Nano ESP32");
  Serial.println("Scanning for I2C devices...\n");

  // Initialize I2C with Arduino Nano ESP32 pins
  // SDA = GPIO 11, SCL = GPIO 12
  Wire.begin(SDA, SCL);

  Serial.printf("I2C initialized: SDA=GPIO%d, SCL=GPIO%d\n", SDA, SCL);
  Serial.println("Starting scan...\n");

  byte error, address;
  int deviceCount = 0;

  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      // Identify common devices
      if (address == 0x3C || address == 0x3D) {
        Serial.println("  -> This looks like an SSD1306 OLED display!");
      }

      deviceCount++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  Serial.println("\n=== Scan Complete ===");
  if (deviceCount == 0) {
    Serial.println("No I2C devices found!");
    Serial.println("\nTroubleshooting tips:");
    Serial.println("1. Check wiring:");
    Serial.println("   OLED GND -> Arduino GND");
    Serial.println("   OLED VCC -> Arduino 3.3V (or 5V if your OLED requires it)");
    Serial.println("   OLED SDA -> Arduino SDA pin");
    Serial.println("   OLED SCL -> Arduino SCL pin");
    Serial.println("2. Ensure OLED is powered (some have power indicator LEDs)");
    Serial.println("3. Try different I2C pull-up resistors if needed");
    Serial.println("4. Test with a multimeter: VCC should be 3.3V or 5V, GND should be 0V");
  }
  else {
    Serial.printf("Found %d I2C device(s)\n", deviceCount);
  }

  Serial.println("\nScanning continuously every 5 seconds...\n");
}

void loop() {
  delay(5000);

  Serial.println("--- Scanning again ---");
  byte error, address;
  int deviceCount = 0;

  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    Serial.println("No devices found");
  }
}
