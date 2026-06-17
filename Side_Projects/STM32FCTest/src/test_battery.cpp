#include "../tests/test_battery.h"
#include "../tests/test_menu.h"

// Battery voltage sensing pin configuration
#define BATTERY_SENSE_PIN PC2_C  // ADC3 IN0

// Resistor divider values (from schematic)
// NOTE: Adjust these based on actual measured resistor values for best accuracy
#define R1 422000.0  // 422k ohms (top resistor) - nominal value
#define R2 102000.0  // 102k ohms (bottom resistor) - nominal value

// ADC configuration
#define ADC_MAX_VALUE 4096.0    // 12-bit ADC (2^12)
#define ADC_VREF 3.3            // 3.3V reference voltage (adjust if needed for calibration)

// Linear calibration: V_actual = CALIBRATION_GAIN * V_measured + CALIBRATION_OFFSET
// To calibrate with 2 known voltages (V1_actual, V1_measured) and (V2_actual, V2_measured):
//   GAIN = (V1_actual - V2_actual) / (V1_measured - V2_measured)
//   OFFSET = V1_actual - GAIN * V1_measured
//
// Current calibrated measurements with old constants (1.0267, 0.326):
//   6V actual → 6.02V measured (error: +0.02V, +0.33%)
//   17V actual → 16.94V measured (error: -0.06V, -0.35%)
//
// Recalibrating with new 2-point measurements:
//   V1: 17V actual, 16.94V measured
//   V2: 6V actual, 6.02V measured
//   GAIN = (17 - 6) / (16.94 - 6.02) = 11 / 10.92 = 1.00733
//   OFFSET = 17 - 1.00733 * 16.94 = 17 - 17.064 = -0.064
#define CALIBRATION_GAIN 1.00733   // Slope correction
#define CALIBRATION_OFFSET -0.064  // Offset correction (in volts)

namespace BatteryTest {
    void setup() {
        Console.println("[BATTERY] Initializing battery voltage test...");

        // Configure ADC pin - use analog mode
        pinMode(BATTERY_SENSE_PIN, INPUT_ANALOG);

        // Set ADC resolution to 12-bit (STM32H7 default, more reliable than 16-bit)
        analogReadResolution(12);

        Console.println("[BATTERY] ADC configured on PC2_C (ADC3 IN0)");
        Console.print("[BATTERY] Resistor divider: R1=");
        Console.print((int)(R1/1000));
        Console.print("k, R2=");
        Console.print((int)(R2/1000));
        Console.println("k");

        float dividerRatio = (R1 + R2) / R2;
        Console.print("[BATTERY] Divider ratio: ");
        Console.println(dividerRatio, 2);

        Console.println("[BATTERY] Setup complete");
    }

    void run() {
        Console.println("\n[BATTERY] Running battery voltage test...");
        Console.println("[BATTERY] Reading battery voltage...\n");

        // Take multiple readings and average
        const int numReadings = 20;
        uint32_t sum = 0;

        Console.print("[BATTERY] Taking ");
        Console.print(numReadings);
        Console.println(" samples...");

        for (int i = 0; i < numReadings; i++) {
            uint16_t reading = analogRead(BATTERY_SENSE_PIN);
            sum += reading;

            // Debug: show first few readings
            if (i < 3) {
                Console.print("  Sample ");
                Console.print(i + 1);
                Console.print(": ");
                Console.println(reading);
            }
            delay(20);
        }

        uint16_t adcValue = sum / numReadings;

        Console.println();

        // Convert ADC reading to voltage at the ADC pin
        float adcVoltage = (adcValue / ADC_MAX_VALUE) * ADC_VREF;

        // Calculate battery voltage using resistor divider formula
        // V_battery = V_adc * (R1 + R2) / R2
        float batteryVoltageRaw = adcVoltage * ((R1 + R2) / R2);

        // Apply linear calibration: V_actual = GAIN * V_measured + OFFSET
        float batteryVoltage = CALIBRATION_GAIN * batteryVoltageRaw + CALIBRATION_OFFSET;

        // Display results
        Console.println("[BATTERY] Measurement Results:");
        Console.print("  ADC Raw Value: ");
        Console.print(adcValue);
        Console.print(" / ");
        Console.print((int)ADC_MAX_VALUE);

        float percentage = (adcValue / ADC_MAX_VALUE) * 100.0;
        Console.print(" (");
        Console.print(percentage, 1);
        Console.println("%)");

        Console.print("  ADC Pin Voltage: ");
        Console.print(adcVoltage, 3);
        Console.println(" V");

        float dividerRatio = (R1 + R2) / R2;
        Console.print("  Multiplier: ");
        Console.println(dividerRatio, 2);

        Console.print("  Battery Voltage (raw): ");
        Console.print(batteryVoltageRaw, 2);
        Console.println(" V");

        Console.print("  Battery Voltage (calibrated): ");
        Console.print(batteryVoltage, 2);
        Console.println(" V");

        if (CALIBRATION_GAIN != 1.0 || CALIBRATION_OFFSET != 0.0) {
            Console.print("  (Linear cal: ");
            Console.print(CALIBRATION_GAIN, 4);
            Console.print("x + ");
            Console.print(CALIBRATION_OFFSET, 3);
            Console.println("V)");
        }

        // Battery status indication
        Console.print("\n[BATTERY] Status: ");
        if (batteryVoltage > 11.0) {
            Console.println("Good (>11V)");
        } else if (batteryVoltage > 10.0) {
            Console.println("Fair (10-11V)");
        } else if (batteryVoltage > 9.0) {
            Console.println("Low (9-10V)");
        } else {
            Console.println("Critical (<9V)");
        }

        Console.println("\n[BATTERY] Test complete. Press '0' for menu.\n");
    }
}
