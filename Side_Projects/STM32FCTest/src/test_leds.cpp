#include "../tests/test_leds.h"
#include "../tests/test_menu.h"

// LED pin configuration
#define LED_1 PE5
#define LED_2 PE6
#define LED_3 PA0

// PWM configuration
#define PWM_MAX_VALUE 255
#define BREATHING_STEPS 50
#define BREATHING_DELAY 20  // ms between steps

namespace LEDTest {
    // Smooth breathing effect using sine wave approximation
    void breathingEffect(int led1, int led2, int led3, int cycles) {
        Console.print("[LED] Three-LED breathing effect (");
        Console.print(cycles);
        Console.println(" cycles)");

        for (int cycle = 0; cycle < cycles; cycle++) {
            // Sweep brightness focus across all three LEDs in a smooth loop.
            for (int i = 0; i <= BREATHING_STEPS; i++) {
                float progress = (float)i / BREATHING_STEPS;
                float easeInOut = progress < 0.5
                    ? 2 * progress * progress
                    : 1 - pow(-2 * progress + 2, 2) / 2;

                int brightness1 = (int)(easeInOut * PWM_MAX_VALUE);
                int brightness2 = (int)((1.0f - easeInOut) * PWM_MAX_VALUE);
                int brightness3 = (int)(fabsf(0.5f - easeInOut) * 2.0f * PWM_MAX_VALUE);

                analogWrite(led1, brightness1);
                analogWrite(led2, brightness2);
                analogWrite(led3, brightness3);
                delay(BREATHING_DELAY);
            }

            for (int i = BREATHING_STEPS; i >= 0; i--) {
                float progress = (float)i / BREATHING_STEPS;
                float easeInOut = progress < 0.5
                    ? 2 * progress * progress
                    : 1 - pow(-2 * progress + 2, 2) / 2;

                int brightness1 = (int)(easeInOut * PWM_MAX_VALUE);
                int brightness2 = (int)((1.0f - easeInOut) * PWM_MAX_VALUE);
                int brightness3 = (int)(fabsf(0.5f - easeInOut) * 2.0f * PWM_MAX_VALUE);

                analogWrite(led1, brightness1);
                analogWrite(led2, brightness2);
                analogWrite(led3, brightness3);
                delay(BREATHING_DELAY);
            }
        }

        // Turn off all LEDs
        analogWrite(led1, 0);
        analogWrite(led2, 0);
        analogWrite(led3, 0);
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

    // Knight Rider style sweep effect across all three LEDs
    void knightRiderEffect(int led1, int led2, int led3, int cycles) {
        Console.print("[LED] Knight Rider sweep (");
        Console.print(cycles);
        Console.println(" cycles)");

        const int leds[] = {led1, led2, led3};

        for (int cycle = 0; cycle < cycles; cycle++) {
            for (int active = 0; active < 3; active++) {
                for (int brightness = 0; brightness <= PWM_MAX_VALUE; brightness += 15) {
                    for (int ledIndex = 0; ledIndex < 3; ledIndex++) {
                        analogWrite(leds[ledIndex], ledIndex == active ? brightness : 0);
                    }
                    delay(10);
                }
                for (int brightness = PWM_MAX_VALUE; brightness >= 0; brightness -= 15) {
                    for (int ledIndex = 0; ledIndex < 3; ledIndex++) {
                        analogWrite(leds[ledIndex], ledIndex == active ? brightness : 0);
                    }
                    delay(10);
                }
            }

            for (int active = 1; active >= 0; active--) {
                for (int brightness = 0; brightness <= PWM_MAX_VALUE; brightness += 15) {
                    for (int ledIndex = 0; ledIndex < 3; ledIndex++) {
                        analogWrite(leds[ledIndex], ledIndex == active ? brightness : 0);
                    }
                    delay(10);
                }
                for (int brightness = PWM_MAX_VALUE; brightness >= 0; brightness -= 15) {
                    for (int ledIndex = 0; ledIndex < 3; ledIndex++) {
                        analogWrite(leds[ledIndex], ledIndex == active ? brightness : 0);
                    }
                    delay(10);
                }
            }
        }

        analogWrite(led1, 0);
        analogWrite(led2, 0);
        analogWrite(led3, 0);
    }

    void setup() {
        Console.println("[LED] Initializing LED test...");

        pinMode(LED_1, OUTPUT);
        pinMode(LED_2, OUTPUT);
        pinMode(LED_3, OUTPUT);

        // Start with LEDs off
        digitalWrite(LED_1, LOW);
        digitalWrite(LED_2, LOW);
        digitalWrite(LED_3, LOW);

        Console.println("[LED] LED 1 (SENS) configured on PE5");
        Console.println("[LED] LED 2 (GPS) configured on PE6");
        Console.println("[LED] LED 3 (BT) configured on PA0");
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

            digitalWrite(LED_3, HIGH);
            Console.println("  LED3: ON");
            delay(200);
            digitalWrite(LED_3, LOW);
            Console.println("  LED3: OFF");
            delay(200);
        }
        Console.println();

        delay(500);

        // Test 2: Individual breathing
        Console.println("[LED] Test 2: Individual LED breathing");
        singleBreathingEffect(LED_1, "LED1 / SENS (PE5)", 2);
        delay(300);
        singleBreathingEffect(LED_2, "LED2 / GPS (PE6)", 2);
        delay(300);
        singleBreathingEffect(LED_3, "LED3 / BT (PA0)", 2);
        Console.println();

        delay(500);

        // Test 3: Alternating breathing (the cool one!)
        Console.println("[LED] Test 3: Three-LED breathing");
        breathingEffect(LED_1, LED_2, LED_3, 3);
        Console.println();

        delay(500);

        // Test 4: Knight Rider sweep
        Console.println("[LED] Test 4: Knight Rider sweep");
        knightRiderEffect(LED_1, LED_2, LED_3, 3);
        Console.println();

        delay(500);

        // Test 5: PWM fade test (0-100%)
        Console.println("[LED] Test 5: PWM fade test (both LEDs)");
        Console.println("  Fading up...");
        for (int brightness = 0; brightness <= PWM_MAX_VALUE; brightness += 5) {
            analogWrite(LED_1, brightness);
            analogWrite(LED_2, brightness);
            analogWrite(LED_3, brightness);
            delay(20);
        }

        Console.println("  Fading down...");
        for (int brightness = PWM_MAX_VALUE; brightness >= 0; brightness -= 5) {
            analogWrite(LED_1, brightness);
            analogWrite(LED_2, brightness);
            analogWrite(LED_3, brightness);
            delay(20);
        }
        Console.println();

        // Ensure LEDs are off at the end
        analogWrite(LED_1, 0);
        analogWrite(LED_2, 0);
        analogWrite(LED_3, 0);

        Console.println("[LED] All tests complete. Press '0' for menu.\n");
    }
}
