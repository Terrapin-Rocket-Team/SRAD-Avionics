#include "../tests/test_menu.h"

void TestMenu::displayMenu() {
    Console.println("========================================");
    Console.println("Select a test to run:");
    Console.println("1. I2C Bus Scanner");
    Console.println("0. Display Menu");
    Console.println("========================================");
    Console.print("Enter test number: ");
}

TestID TestMenu::getSelectedTest() {
    if (Console.available() <= 0) {
        return TEST_NONE;
    }

    const char input = static_cast<char>(Console.read());
    while (Console.available() > 0) {
        Console.read();
    }

    Console.println(input);

    switch (input) {
        case '1':
            return TEST_I2C_SCANNER;
        case '0':
            displayMenu();
            return TEST_NONE;
        default:
            Console.println("Invalid selection. Press '0' for menu.");
            return TEST_NONE;
    }
}

void TestMenu::printHeader(const char* testName) {
    Console.println();
    Console.println("========================================");
    Console.print("Running: ");
    Console.println(testName);
    Console.println("========================================");
}
