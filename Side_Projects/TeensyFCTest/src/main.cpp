#include <Arduino.h>

#include "../tests/test_i2c.h"
#include "../tests/test_menu.h"

namespace {
TestID currentTest = TEST_NONE;
bool testInitialized = false;

void setupTest(TestID test) {
    switch (test) {
        case TEST_I2C_SCANNER:
            TestMenu::printHeader("I2C Bus Scanner");
            I2CTest::setup();
            break;
        default:
            break;
    }
}

void runTest(TestID test) {
    switch (test) {
        case TEST_I2C_SCANNER:
            I2CTest::run();
            break;
        default:
            break;
    }
}
}  // namespace

void setup() {
    Console.begin(TestMenu::kConsoleBaud);

    const unsigned long startTime = millis();
    while (!Console && (millis() - startTime < 4000)) {
        delay(10);
    }

    delay(100);

    Console.println();
    Console.println("========================================");
    Console.println("    Teensy Hardware Test Framework");
    Console.println("========================================");
    Console.println("Console: USB Serial");
    Console.println("Board: Teensy 4.1");
    Console.println();

    TestMenu::displayMenu();
}

void loop() {
    const TestID selectedTest = TestMenu::getSelectedTest();

    if (selectedTest != TEST_NONE) {
        if (selectedTest != currentTest || !testInitialized) {
            currentTest = selectedTest;
            setupTest(currentTest);
            testInitialized = true;
        }

        runTest(currentTest);
        testInitialized = false;
        currentTest = TEST_NONE;
    }

    delay(10);
}
