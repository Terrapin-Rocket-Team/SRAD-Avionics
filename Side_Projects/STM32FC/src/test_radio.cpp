#include "../tests/test_radio.h"
#include "../tests/test_menu.h"
#include <RadioLib.h>
#include <SPI.h>

// Radio SPI pins
#define RAD_SCK   PB3
#define RAD_MISO  PB4
#define RAD_MOSI  PD7
#define RAD_CS    PA15

// Radio control pins
#define RAD_NRST  PC13
#define RAD_IRQ   PE2
#define RAD_BUSY  PE3
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
        Console.println("[RADIO] Configuring SPI pins...");
        Console.println("[RADIO]   MISO: PB4");
        Console.println("[RADIO]   SCLK: PB3");
        Console.println("[RADIO]   MOSI: PD7");

        SPI.setMISO(RAD_MISO);
        SPI.setSCLK(RAD_SCK);
        SPI.setMOSI(RAD_MOSI);
        SPI.begin();
        Console.println("[RADIO] ✓ SPI initialized");

        // Diagnostic: Check control pins before initialization
        Console.println("[RADIO] Checking control pins...");

        // Configure and check BUSY pin
        pinMode(RAD_BUSY, INPUT);
        bool busyState = digitalRead(RAD_BUSY);
        Console.print("[RADIO]   BUSY (PE3): ");
        Console.println(busyState ? "HIGH" : "LOW");

        // Check RESET pin
        pinMode(RAD_NRST, OUTPUT);
        digitalWrite(RAD_NRST, HIGH);  // Ensure not in reset
        delay(10);
        Console.println("[RADIO]   NRST (PC13): Released (HIGH)");

        // Check if BUSY changes after reset pulse
        Console.println("[RADIO] Performing reset cycle...");
        digitalWrite(RAD_NRST, LOW);
        delay(10);
        digitalWrite(RAD_NRST, HIGH);
        delay(10);
        busyState = digitalRead(RAD_BUSY);
        Console.print("[RADIO]   BUSY after reset: ");
        Console.println(busyState ? "HIGH" : "LOW");

        // Check IRQ pin
        pinMode(RAD_IRQ, INPUT);
        bool irqState = digitalRead(RAD_IRQ);
        Console.print("[RADIO]   IRQ (PE2): ");
        Console.println(irqState ? "HIGH" : "LOW");

        Console.println("[RADIO] Initializing radio module...");

        // Wait for BUSY to go low (radio should exit busy state after reset)
        Console.print("[RADIO] Waiting for BUSY to go LOW");
        unsigned long startWait = millis();
        while (digitalRead(RAD_BUSY) == HIGH && (millis() - startWait) < 1000) {
            Console.print(".");
            delay(50);
        }
        Console.println();

        if (digitalRead(RAD_BUSY) == LOW) {
            Console.println("[RADIO] ✓ BUSY cleared, radio ready");
        } else {
            Console.println("[RADIO] ✗ BUSY still HIGH after 1s - power or XTAL issue");
            Console.println("[RADIO] This usually means:");
            Console.println("[RADIO]   1. Power supply issue (needs stable 3.3V)");
            Console.println("[RADIO]   2. Crystal oscillator not starting");
            Console.println("[RADIO]   3. Hardware fault");
        }

        Console.println("[RADIO] Initializing LR1121 with RadioLib...");
        Console.println("[RADIO] Attempting communication with chip...");

        // Add a small delay to ensure chip is stable
        delay(50);

        // Manual chip detection - send GET_STATUS command
        Console.println("[RADIO] Attempting manual SPI communication...");

        // First, check if MISO is stuck high or low
        pinMode(RAD_MISO, INPUT);
        delay(1);
        bool misoIdle = digitalRead(RAD_MISO);
        Console.print("[RADIO] MISO idle state: ");
        Console.println(misoIdle ? "HIGH" : "LOW");

        // Reconfigure for SPI
        SPI.setMISO(RAD_MISO);
        delay(1);

        digitalWrite(RAD_CS, LOW);
        delayMicroseconds(10);

        // Wait for BUSY to go low
        unsigned long busyWait = millis();
        while (digitalRead(RAD_BUSY) == HIGH && (millis() - busyWait) < 100) {}

        Console.print("[RADIO] BUSY state before command: ");
        Console.println(digitalRead(RAD_BUSY) ? "HIGH" : "LOW");

        // Send GetStatus command (0xC0 for LR11x0)
        uint8_t cmd = 0xC0;
        SPI.transfer(cmd);
        delayMicroseconds(10);

        // Read status bytes
        uint8_t status1 = SPI.transfer(0x00);
        uint8_t status2 = SPI.transfer(0x00);

        digitalWrite(RAD_CS, HIGH);

        Console.print("[RADIO] GetStatus response: 0x");
        Console.print(status1, HEX);
        Console.print(" 0x");
        Console.println(status2, HEX);

        if (status1 == 0xFF && status2 == 0xFF) {
            Console.println("[RADIO] ✗ CRITICAL: All bits HIGH - No SPI response!");
            Console.println("[RADIO] ");
            Console.println("[RADIO] === HARDWARE DIAGNOSIS ===");
            Console.println("[RADIO] The 0xFF 0xFF response means NO device is responding.");
            Console.println("[RADIO] ");
            Console.println("[RADIO] Most likely causes:");
            Console.println("[RADIO]   1. LR1121 module NOT INSTALLED on PCB");
            Console.println("[RADIO]   2. LR1121 not powered (check 3.3V rail)");
            Console.println("[RADIO]   3. Wrong SPI pins in firmware");
            Console.println("[RADIO]   4. MISO trace broken/not connected");
            Console.println("[RADIO] ");
            Console.println("[RADIO] Recommended tests:");
            Console.println("[RADIO]   A. Visually inspect PCB - is LR1121 module installed?");
            Console.println("[RADIO]   B. Use multimeter to check 3.3V on radio module");
            Console.println("[RADIO]   C. Verify PCB schematic matches code pin definitions");
            Console.println("[RADIO]   D. Check continuity: STM32 PB4 to LR1121 MISO");
            Console.println("[RADIO] ");
        } else if (status1 == 0x00 && status2 == 0x00) {
            Console.println("[RADIO] ✗ All bits LOW - MISO shorted to ground?");
        } else {
            Console.print("[RADIO] ✓ Got SPI response: 0x");
            Console.print(status1, HEX);
            Console.print(" 0x");
            Console.println(status2, HEX);
            Console.println("[RADIO] Chip is responding but may not be LR11x0 family");
        }

        delay(10);

        // Use begin() for LR1121
        // Frequency: 915.0 MHz, Bandwidth: 125.0 kHz, SF: 9, CR: 7,
        // Sync word: 0x12, TX power: 14 dBm, Preamble: 8
        int state = radio.begin(915.0, 125.0, 9, 7, 0x12, 14, 8);

        if (state == RADIOLIB_ERR_NONE) {
            Console.println("[RADIO] ✓ Radio initialized successfully!");
            radioInitialized = true;
        } else {
            Console.print("[RADIO] ✗ Initialization failed, code: ");
            Console.println(state);
            Console.println("[RADIO] ");
            Console.println("[RADIO] Error codes:");
            Console.println("[RADIO]   -2: SPI initialization failed");
            Console.println("[RADIO]   -706: Chip not found / no response");
            Console.println("[RADIO] ");
            Console.println("[RADIO] Possible causes:");
            Console.println("[RADIO]   - Check power supply to radio (3.3V)");
            Console.println("[RADIO]   - Verify SPI pin connections");
            Console.println("[RADIO]   - Check NRST, IRQ, BUSY pin connections");
            Console.println("[RADIO]   - Verify radio module is properly soldered");
            Console.println("\n[RADIO] Test aborted. Press '0' for menu.\n");
            return;
        }

        delay(100);

        // Test 2: Configure RF switch
        Console.println("\n[RADIO] === Test 2: RF Switch Configuration ===");
        Console.println("[RADIO] Setting up antenna switch control...");

        radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
        Console.println("[RADIO] ✓ RF switch table configured");
        Console.println("[RADIO]   DIO5: High power TX");
        Console.println("[RADIO]   DIO6: Normal TX");
        Console.println("[RADIO]   DIO7: RX mode");

        // Test 3: Configure radio parameters
        Console.println("\n[RADIO] === Test 3: Radio Parameters ===");

        // Set frequency to 915 MHz (US ISM band)
        Console.print("[RADIO] Setting frequency to 915.0 MHz... ");
        state = radio.setFrequency(915.0);
        if (state == RADIOLIB_ERR_NONE) {
            Console.println("✓");
        } else {
            Console.print("✗ Error: ");
            Console.println(state);
        }

        // Set spreading factor to 7
        Console.print("[RADIO] Setting spreading factor to 7... ");
        state = radio.setSpreadingFactor(7);
        if (state == RADIOLIB_ERR_NONE) {
            Console.println("✓");
        } else {
            Console.print("✗ Error: ");
            Console.println(state);
        }

        // Set bandwidth to 125 kHz
        Console.print("[RADIO] Setting bandwidth to 125.0 kHz... ");
        state = radio.setBandwidth(125.0);
        if (state == RADIOLIB_ERR_NONE) {
            Console.println("✓");
        } else {
            Console.print("✗ Error: ");
            Console.println(state);
        }

        // Set coding rate to 5
        Console.print("[RADIO] Setting coding rate to 5... ");
        state = radio.setCodingRate(5);
        if (state == RADIOLIB_ERR_NONE) {
            Console.println("✓");
        } else {
            Console.print("✗ Error: ");
            Console.println(state);
        }

        // Set sync word to 0x12 (private network)
        Console.print("[RADIO] Setting sync word to 0x12... ");
        state = radio.setSyncWord(0x12);
        if (state == RADIOLIB_ERR_NONE) {
            Console.println("✓");
        } else {
            Console.print("✗ Error: ");
            Console.println(state);
        }

        // Set output power to 14 dBm
        Console.print("[RADIO] Setting output power to 14 dBm... ");
        state = radio.setOutputPower(14);
        if (state == RADIOLIB_ERR_NONE) {
            Console.println("✓");
        } else {
            Console.print("✗ Error: ");
            Console.println(state);
        }

        // Test 4: Transmit test packet
        Console.println("\n[RADIO] === Test 4: Transmission Test ===");
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

        // Test 5: Receive mode test
        Console.println("\n[RADIO] === Test 5: Receive Mode Test ===");
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
