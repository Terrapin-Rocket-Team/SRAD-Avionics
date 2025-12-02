#ifndef TEST_MENU_H
#define TEST_MENU_H

#include <Arduino.h>

// USB CDC as primary console
// Use 'Serial' which is USB CDC when USBCON is defined
#define Console Serial

// Test IDs
enum TestID {
    TEST_NONE = 0,
    TEST_EMMC,
    TEST_BUZZER,
    TEST_LEDS,
    TEST_PYROTECHNICS,
    TEST_I2C_SENSORS,
    TEST_UART_BT,
    TEST_USB,
    TEST_BATTERY,
    TEST_RADIO,
    TEST_ALL
};

class TestMenu {
public:
    static void displayMenu();
    static TestID getSelectedTest();
    static void printHeader(const char* testName);
};

#endif // TEST_MENU_H
