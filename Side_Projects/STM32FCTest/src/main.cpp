#include <Arduino.h>
#include "../tests/test_menu.h"
#include "../tests/test_emmc.h"
#include "../tests/test_buzzer.h"
#include "../tests/test_leds.h"
#include "../tests/test_pyrotechnics.h"
#include "../tests/test_i2c.h"
#include "../tests/test_uart_bt.h"
#include "../tests/test_usb.h"
#include "../tests/test_battery.h"
#include "../tests/test_radio.h"

// Console interface can be configured to use either USB CDC or UART
// Configure in platformio.ini: USE_USB_CONSOLE or USE_UART_CONSOLE
// Console is defined in test_menu.h based on compile-time flags

// Test state
TestID currentTest = TEST_NONE;
bool testInitialized = false;

void setupTest(TestID test) {
    switch (test) {
        case TEST_EMMC:
            TestMenu::printHeader("EMMC Test");
            EMMCTest::setup();
            break;
        case TEST_BUZZER:
            TestMenu::printHeader("Buzzer Test");
            BuzzerTest::setup();
            break;
        case TEST_LEDS:
            TestMenu::printHeader("LED Test");
            LEDTest::setup();
            break;
        case TEST_PYROTECHNICS:
            TestMenu::printHeader("Pyrotechnics Test");
            PyrotechnicsTest::setup();
            break;
        case TEST_I2C_SENSORS:
            TestMenu::printHeader("I2C Sensors Test");
            I2CTest::setup();
            break;
        case TEST_UART_BT:
            TestMenu::printHeader("UART Bluetooth Test");
            UARTBTTest::setup();
            break;
        case TEST_USB:
            TestMenu::printHeader("USB CDC Test");
            USBTest::setup();
            break;
        case TEST_BATTERY:
            TestMenu::printHeader("Battery Voltage Test");
            BatteryTest::setup();
            break;
        case TEST_RADIO:
            TestMenu::printHeader("Radio Test");
            RadioTest::setup();
            break;
        case TEST_ALL:
            TestMenu::printHeader("All Tests");
            Console.println("Initializing all test modules...\n");
            EMMCTest::setup();
            BuzzerTest::setup();
            LEDTest::setup();
            PyrotechnicsTest::setup();
            I2CTest::setup();
            UARTBTTest::setup();
            USBTest::setup();
            BatteryTest::setup();
            RadioTest::setup();
            break;
        default:
            break;
    }
}

void runTest(TestID test) {
    switch (test) {
        case TEST_EMMC:
            EMMCTest::run();
            break;
        case TEST_BUZZER:
            BuzzerTest::run();
            break;
        case TEST_LEDS:
            LEDTest::run();
            break;
        case TEST_PYROTECHNICS:
            PyrotechnicsTest::run();
            break;
        case TEST_I2C_SENSORS:
            I2CTest::run();
            break;
        case TEST_UART_BT:
            UARTBTTest::run();
            break;
        case TEST_USB:
            USBTest::run();
            break;
        case TEST_BATTERY:
            BatteryTest::run();
            break;
        case TEST_RADIO:
            RadioTest::run();
            break;
        case TEST_ALL:
            Console.println("\n=== Running All Tests ===\n");
            EMMCTest::run();
            delay(1000);
            BuzzerTest::run();
            delay(1000);
            LEDTest::run();
            delay(1000);
            PyrotechnicsTest::run();
            delay(1000);
            I2CTest::run();
            delay(1000);
            UARTBTTest::run();
            delay(1000);
            USBTest::run();
            delay(1000);
            BatteryTest::run();
            delay(1000);
            RadioTest::run();
            Console.println("\n=== All Tests Complete ===");
            Console.println("Press '0' for menu.\n");
            break;
        default:
            break;
    }
}

void setup() {
    // Initialize console (USB CDC or UART based on compile-time config)
    Console.begin(CONSOLE_BAUD);

#if defined(USE_USB_CONSOLE)
    // Wait for USB serial connection (with timeout)
    unsigned long startTime = millis();
    while (!Console && (millis() - startTime < 3000)) {
        delay(10);
    }
#elif defined(USE_UART_CONSOLE)
    // For UART, just give it a short delay to stabilize
    delay(100);
#endif

    // Small delay to allow terminal to stabilize
    delay(100);

    Console.println("\n\n");
    Console.println("╔════════════════════════════════════════╗");
    Console.println("║  STM32H723 Hardware Test Framework     ║");
    Console.print("║  Console: ");
    Console.print(CONSOLE_TYPE);
    // Pad the line to align the border
    int padding = 28 - strlen(CONSOLE_TYPE);
    for (int i = 0; i < padding; i++) {
        Console.print(" ");
    }
    Console.println("║");
    Console.println("╚════════════════════════════════════════╝");
    Console.println();
    Console.print("✓ Console initialized on ");
    Console.println(CONSOLE_PINS);
    Console.println("✓ System ready!");
    Console.println();

    // Display the menu
    TestMenu::displayMenu();
}

void loop() {
    // Check for test selection
    TestID selectedTest = TestMenu::getSelectedTest();

    if (selectedTest != TEST_NONE) {
        // If we're switching tests or starting a new test
        if (selectedTest != currentTest || !testInitialized) {
            currentTest = selectedTest;
            setupTest(currentTest);
            testInitialized = true;
        }

        // Run the test
        runTest(currentTest);

        // Reset test state so it can be run again if selected
        testInitialized = false;
        currentTest = TEST_NONE;
    }

    // Small delay to prevent serial buffer overflow
    delay(10);
}