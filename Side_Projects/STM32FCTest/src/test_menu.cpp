#include "../tests/test_menu.h"

void TestMenu::displayMenu() {
    Console.println("\n========================================");
    Console.println("    STM32H723 Hardware Test Suite");
    Console.println("========================================");
    Console.println("Select a test to run:");
    Console.println("1. EMMC Test");
    Console.println("2. Buzzer Test");
    Console.println("3. LED Test");
    Console.println("4. Pyrotechnics Test");
    Console.println("5. I2C Sensors Test");
    Console.println("6. UART Bluetooth Module Test");
    Console.println("7. USB CDC Test");
    Console.println("8. Battery Voltage Test");
    Console.println("9. Radio (SX1262) Test");
    Console.println("a. Run All Tests");
    Console.println("0. Display Menu");
    Console.println("========================================");
    Console.print("Enter test number: ");
}

TestID TestMenu::getSelectedTest() {
    if (Console.available() > 0) {
        char input = Console.read();
        // Clear any remaining characters
        while (Console.available() > 0) {
            Console.read();
        }

        Console.println(input);

        switch (input) {
            case '1': return TEST_EMMC;
            case '2': return TEST_BUZZER;
            case '3': return TEST_LEDS;
            case '4': return TEST_PYROTECHNICS;
            case '5': return TEST_I2C_SENSORS;
            case '6': return TEST_UART_BT;
            case '7': return TEST_USB;
            case '8': return TEST_BATTERY;
            case '9': return TEST_RADIO;
            case 'a': case 'A': return TEST_ALL;
            case '0': displayMenu(); return TEST_NONE;
            default:
                Console.println("Invalid selection. Press '0' for menu.");
                return TEST_NONE;
        }
    }
    return TEST_NONE;
}

void TestMenu::printHeader(const char* testName) {
    Console.println("\n========================================");
    Console.print("Running: ");
    Console.println(testName);
    Console.println("========================================");
}
