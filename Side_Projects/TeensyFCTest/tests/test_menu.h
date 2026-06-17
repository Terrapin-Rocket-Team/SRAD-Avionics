#ifndef TEENSY_FC_TEST_MENU_H
#define TEENSY_FC_TEST_MENU_H

#include <Arduino.h>

#define Console Serial

enum TestID {
    TEST_NONE = 0,
    TEST_I2C_SCANNER
};

class TestMenu {
public:
    static constexpr unsigned long kConsoleBaud = 115200;
    static void displayMenu();
    static TestID getSelectedTest();
    static void printHeader(const char* testName);
};

#endif  // TEENSY_FC_TEST_MENU_H
