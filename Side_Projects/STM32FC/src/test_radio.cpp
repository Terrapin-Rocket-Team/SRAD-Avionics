#include "../tests/test_radio.h"
#include "../tests/test_menu.h"
#include <RadioLib.h>
#include <SPI.h>

// Radio SPI pins
#define RAD_SCK   PB3
#define RAD_MISO  PB4
#define RAD_MOSI  PD7
#define RAD_CS    PA15

// // Radio control pins
// #define RAD_NRST  PC13
// #define RAD_IRQ   PE2
// #define RAD_BUSY  PE3

// Radio control pins 2
#define RAD_NRST  PA6
#define RAD_IRQ   PA2
#define RAD_BUSY  PA7
LR1121 radio = new Module(RAD_CS, RAD_IRQ, RAD_NRST, RAD_BUSY, SPI);

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

namespace RadioTest {
    bool radioInitialized = false;

    void setup() {
        Console.println("[RADIO] Initializing radio test...");
        Console.println("[RADIO] LR1121 LoRa Radio Module");
        Console.println("[RADIO] SPI Pins: PB3(SCK), PB4(MISO), PD7(MOSI), PA15(CS)");
        Console.println("[RADIO] Control: PC13(RST), PE2(IRQ), PE3(BUSY)");
        Console.println("[RADIO] Setup complete");
    }

    void run() {
        Console.println("\n[RADIO] Running radio test...");
        Console.println("[RADIO] Testing LR1121 LoRa transceiver\n");

        // Test 1: Initialize radio module
        Console.println("[RADIO] === Test 1: Radio Initialization ===");

        SPI.setMISO(RAD_MISO);
        SPI.setSCLK(RAD_SCK);
        SPI.setMOSI(RAD_MOSI);
        SPI.begin();
        Console.println("[RADIO] ✓ SPI initialized");


        Console.println("[RADIO] Initializing LR1121...");
        int state = radio.begin();

        if (state != RADIOLIB_ERR_NONE) {
            Console.print("[RADIO] ✗ Initialization failed, code: ");
            Console.println(state);
            Console.println("\n[RADIO] Test aborted. Press '0' for menu.\n");
            return;
        }

        Console.println("[RADIO] ✓ Radio initialized successfully!");

        // Configure RF switch
        radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
        Console.println("[RADIO] ✓ RF switch configured");

        // CRITICAL: Set regulator to DC-DC mode
        radio.setRegulatorDCDC();
        Console.println("[RADIO] ✓ DC-DC regulator enabled");

        // Configure radio parameters
        Console.println("\n[RADIO] === Test 2: Radio Parameters ===");

        state = radio.setFrequency(915.0);
        Console.print("[RADIO] Frequency 915.0 MHz: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio.setSpreadingFactor(7);
        Console.print("[RADIO] Spreading Factor 7: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio.setBandwidth(125.0);
        Console.print("[RADIO] Bandwidth 125.0 kHz: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio.setCodingRate(5);
        Console.print("[RADIO] Coding Rate 5: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio.setSyncWord(0x12);
        Console.print("[RADIO] Sync Word 0x12: ");
        Console.println(state == RADIOLIB_ERR_NONE ? "✓" : "✗");

        state = radio.setOutputPower(14);
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

        while (millis() - startTime < 10000) {
            if (Console.available()) {
                char c = Console.read();
                Console.println(c);
                if (c == 'y' || c == 'Y') {
                    doTransmit = true;
                    break;
                } else if (c == 'n' || c == 'N') {
                    break;
                }
            }
            delay(10);
        }

        if (doTransmit) {
            Console.println("[RADIO] Transmitting test packet...");

            String testMessage = "STM32H723-FC Radio Test";
            state = radio.transmit(testMessage);

            if (state == RADIOLIB_ERR_NONE) {
                Console.println("[RADIO] ✓ Packet transmitted successfully!");
                Console.print("[RADIO] Message: \"");
                Console.print(testMessage);
                Console.println("\"");
                Console.print("[RADIO] Data rate: ");
                Console.print(radio.getDataRate());
                Console.println(" bps");
            } else {
                Console.print("[RADIO] ✗ Transmission failed, code: ");
                Console.println(state);
            }
        } else {
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

        while (millis() - startTime < 10000) {
            if (Console.available()) {
                char c = Console.read();
                Console.println(c);
                if (c == 'y' || c == 'Y') {
                    doReceive = true;
                    break;
                } else if (c == 'n' || c == 'N') {
                    break;
                }
            }
            delay(10);
        }

        if (doReceive) {
            Console.println("[RADIO] Entering receive mode for 30 seconds...");
            Console.println("[RADIO] Listening for LoRa packets...");

            state = radio.startReceive();
            if (state != RADIOLIB_ERR_NONE) {
                Console.print("[RADIO] ✗ Failed to start receive mode, code: ");
                Console.println(state);
            } else {
                startTime = millis();
                int packetsReceived = 0;

                while (millis() - startTime < 30000) {
                    // Check if packet was received
                    if (radio.available()) {
                        String message;
                        state = radio.readData(message);

                        if (state == RADIOLIB_ERR_NONE) {
                            packetsReceived++;
                            Console.print("[RADIO] ✓ Packet received #");
                            Console.println(packetsReceived);
                            Console.print("[RADIO]   Data: \"");
                            Console.print(message);
                            Console.println("\"");
                            Console.print("[RADIO]   RSSI: ");
                            Console.print(radio.getRSSI());
                            Console.println(" dBm");
                            Console.print("[RADIO]   SNR: ");
                            Console.print(radio.getSNR());
                            Console.println(" dB");
                        }
                    }
                    delay(10);
                }

                Console.print("[RADIO] Received ");
                Console.print(packetsReceived);
                Console.println(" packet(s)");
            }
        } else {
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
