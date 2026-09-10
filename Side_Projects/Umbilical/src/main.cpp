#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA260.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- 0.91" OLED is a 128x32 SSD1306 over I2C ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1          // share Teensy reset (no dedicated reset pin)
#define OLED_ADDR 0x3C         // most 0.91" modules; some are 0x3D

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_INA260 ina260;

// How long each metric stays on screen before rotating, in ms.
// Indexed by page: 0 = current, 1 = voltage, 2 = power.
const unsigned long PAGE_INTERVAL[3] = {6000, 2000, 2000};  // current shows 3x as long
// How often we sample + print over serial, in ms.
const unsigned long SAMPLE_INTERVAL = 250;
// Readings within +/- this many mA are treated as zero, so sensor noise when
// nothing is plugged in doesn't flicker between small +/- values.
const float CURRENT_DEADBAND_MA = 2.0;
// Display hysteresis: the shown value only updates when the live reading moves
// past these thresholds, so a steady-ish signal stops jittering on screen.
// (A refresh is also forced once per page rotation, see loop().)
const float CURRENT_HYST_MA = 10.0;
const float VOLTAGE_HYST_V  = 0.01;
const float POWER_HYST_MW   = 100.0;

unsigned long lastPage = 0;
unsigned long lastSample = 0;
uint8_t page = 0;              // 0 = current, 1 = voltage, 2 = power

// Live readings, updated every sample.
float current_mA = 0;
float voltage_V = 0;
float power_mW = 0;

// Held values actually shown on screen; updated via the hysteresis rules above.
float disp_current_mA = 0;
float disp_voltage_V = 0;
float disp_power_mW = 0;

bool needsRedraw = true;

void showPage(uint8_t p) {
  display.clearDisplay();

  // small label on top
  display.setTextSize(1);
  display.setCursor(0, 0);

  // big value below
  char value[16];
  const char *label;

  switch (p) {
    case 0:
      label = "CURRENT";
      snprintf(value, sizeof(value), "%.3f A", disp_current_mA / 1000.0);
      break;
    case 1:
      label = "VOLTAGE";
      snprintf(value, sizeof(value), "%.3f V", disp_voltage_V);
      break;
    default:
      label = "POWER";
      snprintf(value, sizeof(value), "%.3f W", disp_power_mW / 1000.0);
      break;
  }

  display.println(label);
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.println(value);
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!ina260.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1) { delay(10); }
  }
  Serial.println("Found INA260 chip");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 allocation failed");
    while (1) { delay(10); }
  }

  display.setRotation(2);  // screen mounted upside-down; rotate 180 degrees

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("INA260 monitor");
  display.display();
  delay(750);
}

void loop() {
  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL) {
    lastSample = now;

    current_mA = ina260.readCurrent();
    voltage_V = ina260.readBusVoltage() / 1000.0;  // library returns mV
    power_mW = ina260.readPower();

    // Squelch near-zero noise so an unplugged sensor reads a steady 0.
    if (fabs(current_mA) < CURRENT_DEADBAND_MA) {
      current_mA = 0;
      power_mW = 0;
    }

    // Update the held display values only when a reading moves enough.
    if (fabs(current_mA - disp_current_mA) >= CURRENT_HYST_MA) {
      disp_current_mA = current_mA;
      needsRedraw = true;
    }
    if (fabs(voltage_V - disp_voltage_V) >= VOLTAGE_HYST_V) {
      disp_voltage_V = voltage_V;
      needsRedraw = true;
    }
    if (fabs(power_mW - disp_power_mW) >= POWER_HYST_MW) {
      disp_power_mW = power_mW;
      needsRedraw = true;
    }

    // CSV over USB serial: millis,current_A,voltage_V,power_W
    Serial.print(now);
    Serial.print(',');
    Serial.print(current_mA / 1000.0, 3);
    Serial.print(',');
    Serial.print(voltage_V, 3);
    Serial.print(',');
    Serial.println(power_mW / 1000.0, 3);
  }

  if (now - lastPage >= PAGE_INTERVAL[page]) {
    lastPage = now;
    page = (page + 1) % 3;
    // Force a fresh snapshot once per rotation so the screen can't get stuck
    // showing a stale value that never crossed its hysteresis threshold.
    disp_current_mA = current_mA;
    disp_voltage_V = voltage_V;
    disp_power_mW = power_mW;
    needsRedraw = true;
  }

  if (needsRedraw) {
    showPage(page);
    needsRedraw = false;
  }
}
