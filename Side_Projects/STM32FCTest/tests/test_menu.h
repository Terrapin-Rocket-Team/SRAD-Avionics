#ifndef TEST_MENU_H
#define TEST_MENU_H

#include <Arduino.h>

// Console configuration
// Select console based on compile-time flags
#if defined(USE_UART_CONSOLE)
    // UART Console on PB6 (TX) and PB7 (RX)
    // Serial1 is typically used for hardware UART on STM32
    #define Console Serial1
    #define CONSOLE_BAUD 115200
    #define CONSOLE_TYPE "UART"
    #define CONSOLE_PINS "PB6/PB7"
#elif defined(USE_USB_CONSOLE)
    // USB CDC Console on PA11/PA12
    #define Console Serial
    #define CONSOLE_BAUD 115200
    #define CONSOLE_TYPE "USB CDC"
    #define CONSOLE_PINS "PA11/PA12"
#else
    // Default to USB if nothing specified
    #define Console Serial
    #define CONSOLE_BAUD 115200
    #define CONSOLE_TYPE "USB CDC (default)"
    #define CONSOLE_PINS "PA11/PA12"
    #warning "No console type defined, defaulting to USB CDC"
#endif

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
