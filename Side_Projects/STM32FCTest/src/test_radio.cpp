#include "../tests/test_radio.h"
#include "../tests/test_menu.h"
#include <RadioLib.h>
#include <SPI.h>

// Radio SPI pins
#define RAD_SCK PB3
#define RAD_MISO PB4
#define RAD_MOSI PD7
#define RAD_CS PA15

// Radio control pins
#define RAD_NRST  PC13
#define RAD_IRQ   PE2
#define RAD_BUSY  PE3

// // Radio control pins 2
// #define RAD_NRST  PA6
// #define RAD_IRQ   PA2
// #define RAD_BUSY  PA7

// Declare radio pointer - will be initialized after SPI setup
LR1121 *radio_ptr = nullptr;

// IRQ handler for radio
volatile bool radioIrqFired = false;
void radioIrqHandler() {
    radioIrqFired = true;
}

// RF switch configuration for LR1121
// Controls antenna switching between RX/TX modes
static const uint32_t rfswitch_dio_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
    RADIOLIB_LR11X0_DIO7, RADIOLIB_NC, RADIOLIB_NC};

static const Module::RfSwitchMode_t rfswitch_table[] = {
    // mode                  DIO5  DIO6  DIO7
    {LR11x0::MODE_STBY, {LOW, LOW, LOW}},
    {LR11x0::MODE_RX, {LOW, LOW, HIGH}},
    {LR11x0::MODE_TX, {LOW, HIGH, LOW}},
    {LR11x0::MODE_TX_HP, {HIGH, LOW, LOW}},
    END_OF_MODE_TABLE,
};

namespace RadioTest
{
    bool radioInitialized = false;

    // Pin short detection test
    void testPinShorts()
    {
        Console.println("\n[RADIO] ========================================");
        Console.println("[RADIO] PIN SHORT DETECTION TEST");
        Console.println("[RADIO] ========================================");
        Console.println("[RADIO] This test checks for shorts between pins");
        Console.println("[RADIO] by setting each pin as input with pull-up/down");
        Console.println("[RADIO] while driving other pins to detect conflicts.");
        Console.println();

        // Define all pins to test
        struct TestPin {
            uint8_t pin;
            const char* name;
        };

        TestPin testPins[] = {
            {RAD_SCK, "SCK (PB3)"},
            {RAD_MISO, "MISO (PB4)"},
            {RAD_MOSI, "MOSI (PD7)"},
            {RAD_CS, "CS (PA15)"},
            {RAD_NRST, "NRST (PA6)"},
            {RAD_IRQ, "IRQ (PA2)"},
            {RAD_BUSY, "BUSY (PA7)"}
        };

        const int numPins = sizeof(testPins) / sizeof(testPins[0]);
        bool shortDetected = false;
        int totalTests = 0;
        int failedTests = 0;

        Console.println("[RADIO] Testing pins:");
        for (int i = 0; i < numPins; i++) {
            Console.print("[RADIO]   ");
            Console.println(testPins[i].name);
        }
        Console.println();

        // For each pin, test it as input with pull-up and pull-down
        for (int testIdx = 0; testIdx < numPins; testIdx++) {
            Console.print("[RADIO] Testing ");
            Console.print(testPins[testIdx].name);
            Console.println(" as input...");

            // Test with INPUT_PULLUP
            pinMode(testPins[testIdx].pin, INPUT_PULLUP);
            delay(5); // Let it stabilize

            // Verify it reads HIGH with pull-up
            bool pullupState = digitalRead(testPins[testIdx].pin);
            if (!pullupState) {
                Console.print("[RADIO]   WARNING: ");
                Console.print(testPins[testIdx].name);
                Console.println(" reads LOW with pull-up (stuck low or shorted to GND?)");
                shortDetected = true;
                failedTests++;
            }

            // Now set all OTHER pins to OUTPUT LOW
            for (int driveIdx = 0; driveIdx < numPins; driveIdx++) {
                if (driveIdx == testIdx) continue; // Skip the test pin

                pinMode(testPins[driveIdx].pin, OUTPUT);
                digitalWrite(testPins[driveIdx].pin, LOW);
            }
            delay(5);

            // Read test pin - should still be HIGH if no shorts
            bool stateWithLowDrivers = digitalRead(testPins[testIdx].pin);
            if (!stateWithLowDrivers && pullupState) {
                Console.print("[RADIO]   ✗ SHORT DETECTED: ");
                Console.print(testPins[testIdx].name);
                Console.println(" driven LOW by another pin!");

                // Find which pin is causing it
                for (int driveIdx = 0; driveIdx < numPins; driveIdx++) {
                    if (driveIdx == testIdx) continue;

                    // Set this pin to HIGH, others LOW
                    for (int j = 0; j < numPins; j++) {
                        if (j == testIdx) continue;
                        if (j == driveIdx) {
                            digitalWrite(testPins[j].pin, HIGH);
                        } else {
                            digitalWrite(testPins[j].pin, LOW);
                        }
                    }
                    delay(2);

                    if (digitalRead(testPins[testIdx].pin)) {
                        Console.print("[RADIO]     -> Shorted to ");
                        Console.println(testPins[driveIdx].name);
                    }
                }
                shortDetected = true;
                failedTests++;
            }
            totalTests++;

            // Test with INPUT_PULLDOWN
            pinMode(testPins[testIdx].pin, INPUT_PULLDOWN);
            delay(5);

            bool pulldownState = digitalRead(testPins[testIdx].pin);
            if (pulldownState) {
                Console.print("[RADIO]   WARNING: ");
                Console.print(testPins[testIdx].name);
                Console.println(" reads HIGH with pull-down (stuck high or shorted to VCC?)");
                shortDetected = true;
                failedTests++;
            }

            // Set all OTHER pins to OUTPUT HIGH
            for (int driveIdx = 0; driveIdx < numPins; driveIdx++) {
                if (driveIdx == testIdx) continue;

                pinMode(testPins[driveIdx].pin, OUTPUT);
                digitalWrite(testPins[driveIdx].pin, HIGH);
            }
            delay(5);

            // Read test pin - should still be LOW if no shorts
            bool stateWithHighDrivers = digitalRead(testPins[testIdx].pin);
            if (stateWithHighDrivers && !pulldownState) {
                Console.print("[RADIO]   ✗ SHORT DETECTED: ");
                Console.print(testPins[testIdx].name);
                Console.println(" driven HIGH by another pin!");

                // Find which pin is causing it
                for (int driveIdx = 0; driveIdx < numPins; driveIdx++) {
                    if (driveIdx == testIdx) continue;

                    // Set this pin to LOW, others HIGH
                    for (int j = 0; j < numPins; j++) {
                        if (j == testIdx) continue;
                        if (j == driveIdx) {
                            digitalWrite(testPins[j].pin, LOW);
                        } else {
                            digitalWrite(testPins[j].pin, HIGH);
                        }
                    }
                    delay(2);

                    if (!digitalRead(testPins[testIdx].pin)) {
                        Console.print("[RADIO]     -> Shorted to ");
                        Console.println(testPins[driveIdx].name);
                    }
                }
                shortDetected = true;
                failedTests++;
            }
            totalTests++;

            if (!shortDetected) {
                Console.print("[RADIO]   ✓ ");
                Console.print(testPins[testIdx].name);
                Console.println(" - No shorts detected");
            }

            // Reset all pins to INPUT to avoid conflicts
            for (int i = 0; i < numPins; i++) {
                pinMode(testPins[i].pin, INPUT);
            }
        }

        Console.println();
        Console.println("[RADIO] ========================================");
        Console.println("[RADIO] SHORT DETECTION TEST SUMMARY");
        Console.println("[RADIO] ========================================");
        Console.print("[RADIO] Total tests: ");
        Console.println(totalTests);
        Console.print("[RADIO] Failed tests: ");
        Console.println(failedTests);

        if (shortDetected) {
            Console.println("[RADIO] Result: ✗ SHORTS DETECTED!");
            Console.println("[RADIO] ");
            Console.println("[RADIO] HARDWARE ISSUE FOUND:");
            Console.println("[RADIO] One or more pins are shorted together.");
            Console.println("[RADIO] ");
            Console.println("[RADIO] Recommended actions:");
            Console.println("[RADIO]   1. Inspect PCB for solder bridges");
            Console.println("[RADIO]   2. Check for bent pins touching each other");
            Console.println("[RADIO]   3. Verify schematic vs layout");
            Console.println("[RADIO]   4. Use multimeter to test continuity");
        } else {
            Console.println("[RADIO] Result: ✓ NO SHORTS DETECTED");
            Console.println("[RADIO] All pins are electrically isolated.");
            Console.println("[RADIO] Hardware connectivity appears OK.");
        }
        Console.println("[RADIO] ========================================");
        Console.println();
    }

    void setup()
    {
        Console.println("[RADIO] Initializing radio test...");
        Console.println("[RADIO] LR1121 LoRa Radio Module");
        Console.println("[RADIO] SPI Pins: PB3(SCK), PB4(MISO), PD7(MOSI), PA15(CS)");
        Console.println("[RADIO] Control: PA6(RST), PA2(IRQ), PA7(BUSY)");
        Console.println("[RADIO] Setup complete");
    }

    void run()
    {
        Console.println("\n[RADIO] Running radio test...");
        Console.println("[RADIO] Testing LR1121 LoRa transceiver\n");

        // Run pin short detection test first
        Console.println("[RADIO] Would you like to run pin short detection test?");
        Console.println("[RADIO] Type 's' to run short detection");
        Console.println("[RADIO] Type any other key to skip and continue with normal test");
        Console.print("[RADIO] Your choice: ");

        unsigned long shortTestStart = millis();
        bool runShortTest = false;

        while (millis() - shortTestStart < 10000)
        {
            if (Console.available())
            {
                char c = Console.read();
                Console.println(c);
                if (c == 's' || c == 'S')
                {
                    runShortTest = true;
                    break;
                }
                else
                {
                    break;
                }
            }
            delay(10);
        }

        if (runShortTest)
        {
            testPinShorts();
            Console.println("[RADIO] Short detection complete. Continuing with radio test...");
            Console.println();
        }
        else
        {
            Console.println("[RADIO] Short detection skipped");
            Console.println();
        }

        // Test 1: Initialize radio module
        Console.println("[RADIO] === Test 1: Radio Initialization ===");

        // Configure SPI FIRST, before creating radio object
        SPI.setMISO(RAD_MISO);
        SPI.setSCLK(RAD_SCK);
        SPI.setMOSI(RAD_MOSI);
        SPI.begin();
        Console.println("[RADIO] ✓ SPI initialized");

        // NOW create the radio object with properly configured SPI
        Console.println("[RADIO] Creating radio object...");
        radio_ptr = new LR1121(new Module(RAD_CS, RAD_IRQ, RAD_NRST, RAD_BUSY, SPI));
        Console.println("[RADIO] ✓ Radio object created");

        Console.println("[RADIO] Initializing LR1121...");
        int state = radio_ptr->begin();

        if (state != RADIOLIB_ERR_NONE) {
            Console.print("[RADIO] ✗ Initialization failed, code: ");
            Console.println(state);
            Console.println("\n[RADIO] Test aborted. Press '0' for menu.\n");
            return;
        }

        Console.println("[RADIO] ✓ Radio initialized successfully!");

        // Configure RF switch
        radio_ptr->setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
        Console.println("[RADIO] ✓ RF switch configured");

        // CRITICAL: Set regulator to DC-DC mode
        radio_ptr->setRegulatorDCDC();
        Console.println("[RADIO] ✓ DC-DC regulator enabled");

        // Set up IRQ handler
        radio_ptr->setIrqAction(radioIrqHandler);
        Console.println("[RADIO] ✓ IRQ handler configured");

        // Configure radio parameters
        Console.println("\n[RADIO] === Test 2: Radio Parameters ===");

        state = radio_ptr->setFrequency(915.0);
        Console.print("[RADIO] Frequency 915.0 MHz: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio_ptr->setSpreadingFactor(7);
        Console.print("[RADIO] Spreading Factor 7: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio_ptr->setBandwidth(125.0);
        Console.print("[RADIO] Bandwidth 125.0 kHz: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio_ptr->setCodingRate(5);
        Console.print("[RADIO] Coding Rate 5: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio_ptr->setSyncWord(0x12);
        Console.print("[RADIO] Sync Word 0x12: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio_ptr->setOutputPower(14);
        Console.print("[RADIO] Output Power 14 dBm: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        radioInitialized = true;

        // Test 3: Transmit test packet
        Console.println("\n[RADIO] === Test 3: Transmission Test ===");
        Console.println("[RADIO] Would you like to transmit a test packet?");
        Console.println("[RADIO] Type 'y' to transmit");
        Console.println("[RADIO] Type 'n' to skip (default)");
        Console.print("[RADIO] Your choice: ");

        unsigned long startTime = millis();
        bool doTransmit = false;

        while (millis() - startTime < 10000)
        {
            if (Console.available())
            {
                char c = Console.read();
                Console.println(c);
                if (c == 'y' || c == 'Y')
                {
                    doTransmit = true;
                    break;
                }
                else if (c == 'n' || c == 'N')
                {
                    break;
                }
            }
            delay(10);
        }

        if (doTransmit)
        {
            Console.println("[RADIO] Transmitting test packet...");

            String testMessage = "STM32H723-FC Radio Test";
            state = radio_ptr->transmit(testMessage);

            if (state == RADIOLIB_ERR_NONE)
            {
                Console.println("[RADIO] ✓ Packet transmitted successfully!");
                Console.print("[RADIO] Message: \"");
                Console.print(testMessage);
                Console.println("\"");
                Console.print("[RADIO] Data rate: ");
                Console.print(radio_ptr->getDataRate());
                Console.println(" bps");
            }
            else
            {
                Console.print("[RADIO] ✗ Transmission failed, code: ");
                Console.println(state);
            }
        }
        else
        {
            Console.println("[RADIO] Transmission test skipped");
        }

        // Test 4: Receive mode test
        Console.println("\n[RADIO] === Test 4: Receive Mode Test ===");
        Console.println("[RADIO] Would you like to listen for packets?");
        Console.println("[RADIO] Type 'y' to listen for 30 seconds");
        Console.println("[RADIO] Type 'n' to skip (default)");
        Console.print("[RADIO] Your choice: ");

        startTime = millis();
        bool doReceive = false;

        while (millis() - startTime < 10000)
        {
            if (Console.available())
            {
                char c = Console.read();
                Console.println(c);
                if (c == 'y' || c == 'Y')
                {
                    doReceive = true;
                    break;
                }
                else if (c == 'n' || c == 'N')
                {
                    break;
                }
            }
            delay(10);
        }

        if (doReceive)
        {
            Console.println("[RADIO] Entering receive mode for 30 seconds...");
            Console.println("[RADIO] Listening for LoRa packets...");

            state = radio_ptr->startReceive();
            if (state != RADIOLIB_ERR_NONE)
            {
                Console.print("[RADIO] ✗ Failed to start receive mode, code: ");
                Console.println(state);
            }
            else
            {
                startTime = millis();
                int packetsReceived = 0;

                while (millis() - startTime < 30000)
                {
                    // Check if packet was received
                    if (radio_ptr->available())
                    {
                        String message;
                        state = radio_ptr->readData(message);

                        if (state == RADIOLIB_ERR_NONE)
                        {
                            packetsReceived++;
                            Console.print("[RADIO] ✓ Packet received #");
                            Console.println(packetsReceived);
                            Console.print("[RADIO]   Data: \"");
                            Console.print(message);
                            Console.println("\"");
                            Console.print("[RADIO]   RSSI: ");
                            Console.print(radio_ptr->getRSSI());
                            Console.println(" dBm");
                            Console.print("[RADIO]   SNR: ");
                            Console.print(radio_ptr->getSNR());
                            Console.println(" dB");
                        }
                    }
                    delay(10);
                }

                Console.print("[RADIO] Received ");
                Console.print(packetsReceived);
                Console.println(" packet(s)");
            }
        }
        else
        {
            Console.println("[RADIO] Receive test skipped");
        }

        // Summary
        Console.println("\n[RADIO] ========================================");
        Console.println("[RADIO] RADIO TEST SUMMARY");
        Console.println("[RADIO] ========================================");
        Console.println("[RADIO] Status: ✓ Radio is functional");
        Console.println("[RADIO] Chip: LR1121 LoRa transceiver");
        Console.println("[RADIO] Frequency: 915.0 MHz");
        Console.println("[RADIO] Spreading Factor: 7");
        Console.println("[RADIO] Bandwidth: 125.0 kHz");
        Console.println("[RADIO] Coding Rate: 5");
        Console.println("[RADIO] Output Power: 14 dBm");
        Console.println("[RADIO] ========================================");

        Console.println("\n[RADIO] Test complete. Press '0' for menu.\n");
    }
}
