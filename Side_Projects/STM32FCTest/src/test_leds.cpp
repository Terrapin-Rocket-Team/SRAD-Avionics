#include "../tests/test_leds.h"
#include "../tests/test_menu.h"

// LED pin configuration
#define LED_1 PC0
#define LED_2 PC1

// PWM configuration
#define PWM_MAX_VALUE 255
#define BREATHING_STEPS 50
#define BREATHING_DELAY 20  // ms between steps

namespace LEDTest {
    // Smooth breathing effect using sine wave approximation
    void breathingEffect(int led1, int led2, int cycles) {
        Console.print("[LED] Alternating breathing effect (");
        Console.print(cycles);
        Console.println(" cycles)");

        for (int cycle = 0; cycle < cycles; cycle++) {
            // LED1 breathes in while LED2 breathes out
            for (int i = 0; i <= BREATHING_STEPS; i++) {
                // Calculate brightness using smooth ease-in-out curve
                float progress = (float)i / BREATHING_STEPS;
                float easeInOut = progress < 0.5
                    ? 2 * progress * progress
                    : 1 - pow(-2 * progress + 2, 2) / 2;

                int brightness1 = (int)(easeInOut * PWM_MAX_VALUE);
                int brightness2 = PWM_MAX_VALUE - brightness1;  // Inverse for LED2

                analogWrite(led1, brightness1);
                analogWrite(led2, brightness2);
                delay(BREATHING_DELAY);
            }

            // LED1 breathes out while LED2 breathes in
            for (int i = BREATHING_STEPS; i >= 0; i--) {
                float progress = (float)i / BREATHING_STEPS;
                float easeInOut = progress < 0.5
                    ? 2 * progress * progress
                    : 1 - pow(-2 * progress + 2, 2) / 2;

                int brightness1 = (int)(easeInOut * PWM_MAX_VALUE);
                int brightness2 = PWM_MAX_VALUE - brightness1;

                analogWrite(led1, brightness1);
                analogWrite(led2, brightness2);
                delay(BREATHING_DELAY);
            }
        }

        // Turn off both LEDs
        analogWrite(led1, 0);
        analogWrite(led2, 0);
    }

    // Individual breathing effect for a single LED
    void singleBreathingEffect(int led, const char* name, int cycles) {
        Console.print("[LED] ");
        Console.print(name);
        Console.print(" breathing (");
        Console.print(cycles);
        Console.println(" cycles)");

        for (int cycle = 0; cycle < cycles; cycle++) {
            // Breathe in
            for (int i = 0; i <= BREATHING_STEPS; i++) {
                float progress = (float)i / BREATHING_STEPS;
                float easeInOut = progress < 0.5
                    ? 2 * progress * progress
                    : 1 - pow(-2 * progress + 2, 2) / 2;

                int brightness = (int)(easeInOut * PWM_MAX_VALUE);
                analogWrite(led, brightness);
                delay(BREATHING_DELAY);
            }

            // Breathe out
            for (int i = BREATHING_STEPS; i >= 0; i--) {
                float progress = (float)i / BREATHING_STEPS;
                float easeInOut = progress < 0.5
                    ? 2 * progress * progress
                    : 1 - pow(-2 * progress + 2, 2) / 2;

                int brightness = (int)(easeInOut * PWM_MAX_VALUE);
                analogWrite(led, brightness);
                delay(BREATHING_DELAY);
            }
        }

        analogWrite(led, 0);
    }

    // Knight Rider style sweep effect
    void knightRiderEffect(int led1, int led2, int cycles) {
        Console.print("[LED] Knight Rider sweep (");
        Console.print(cycles);
        Console.println(" cycles)");

        for (int cycle = 0; cycle < cycles; cycle++) {
            // Sweep left to right
            for (int brightness = 0; brightness <= PWM_MAX_VALUE; brightness += 15) {
                analogWrite(led1, brightness);
                analogWrite(led2, PWM_MAX_VALUE - brightness);
                delay(10);
            }

            // Sweep right to left
            for (int brightness = PWM_MAX_VALUE; brightness >= 0; brightness -= 15) {
                analogWrite(led1, brightness);
                analogWrite(led2, PWM_MAX_VALUE - brightness);
                delay(10);
            }
        }

        analogWrite(led1, 0);
        analogWrite(led2, 0);
    }

    void setup() {
        Console.println("[LED] Initializing LED test...");

        pinMode(LED_1, OUTPUT);
        pinMode(LED_2, OUTPUT);

        // Start with LEDs off
        digitalWrite(LED_1, LOW);
        digitalWrite(LED_2, LOW);

        Console.println("[LED] LED 1 configured on PC0");
        Console.println("[LED] LED 2 configured on PC1");
        Console.println("[LED] PWM breathing effects ready");
        Console.println("[LED] Setup complete");
    }

    void run() {
        Console.println("\n[LED] Running LED test...");
        Console.println("[LED] Demonstrating PWM breathing effects\n");

        // Test 1: Basic blink to verify LEDs work
        Console.println("[LED] Test 1: Basic blink verification");
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_1, HIGH);
            Console.println("  LED1: ON");
            delay(200);
            digitalWrite(LED_1, LOW);
            Console.println("  LED1: OFF");
            delay(200);

            digitalWrite(LED_2, HIGH);
            Console.println("  LED2: ON");
            delay(200);
            digitalWrite(LED_2, LOW);
            Console.println("  LED2: OFF");
            delay(200);
        }
        Console.println();

        delay(500);

        // Test 2: Individual breathing
        Console.println("[LED] Test 2: Individual LED breathing");
        singleBreathingEffect(LED_1, "LED1 (PC0)", 2);
        delay(300);
        singleBreathingEffect(LED_2, "LED2 (PC1)", 2);
        Console.println();

        delay(500);

        // Test 3: Alternating breathing (the cool one!)
        Console.println("[LED] Test 3: Alternating breathing");
        breathingEffect(LED_1, LED_2, 3);
        Console.println();

        delay(500);

        // Test 4: Knight Rider sweep
        Console.println("[LED] Test 4: Knight Rider sweep");
        knightRiderEffect(LED_1, LED_2, 3);
        Console.println();

        delay(500);

        // Test 5: PWM fade test (0-100%)
        Console.println("[LED] Test 5: PWM fade test (both LEDs)");
        Console.println("  Fading up...");
        for (int brightness = 0; brightness <= PWM_MAX_VALUE; brightness += 5) {
            analogWrite(LED_1, brightness);
            analogWrite(LED_2, brightness);
            delay(20);
        }

        Console.println("  Fading down...");
        for (int brightness = PWM_MAX_VALUE; brightness >= 0; brightness -= 5) {
            analogWrite(LED_1, brightness);
            analogWrite(LED_2, brightness);
            delay(20);
        }
        Console.println();

        // Ensure LEDs are off at the end
        analogWrite(LED_1, 0);
        analogWrite(LED_2, 0);

        Console.println("[LED] All tests complete. Press '0' for menu.\n");
    }
}
