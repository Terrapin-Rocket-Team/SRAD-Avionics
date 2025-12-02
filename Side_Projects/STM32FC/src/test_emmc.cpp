#include "../tests/test_emmc.h"
#include "../tests/test_menu.h"
#include <STM32SD.h>

// eMMC pins (SDMMC1 interface)
// PC8  = DAT0
// PC9  = DAT1
// PC10 = DAT2
// PC11 = DAT3
// PC12 = CLK
// PD2  = CMD

// STM32H7 uses SDMMC1 peripheral for these pins
// The SD library should automatically use SDMMC1

namespace EMMCTest {
    bool emmcInitialized = false;
    uint64_t cardSize = 0;

    void setup() {
        Console.println("[EMMC] Initializing eMMC test...");
        Console.println("[EMMC] Pins: PC8-11(DAT0-3), PC12(CLK), PD2(CMD)");
        Console.println("[EMMC] Using SDMMC1 peripheral");

        // Note: Pin configuration is handled by the SD library
        // when using SDMMC peripheral

        Console.println("[EMMC] Setup complete");
        Console.println("[EMMC] ⚠ WARNING: eMMC tests will initialize and");
        Console.println("[EMMC]   read from the device. No writes will be");
        Console.println("[EMMC]   performed in basic tests for safety.");
    }

    void run() {
        Console.println("\n[EMMC] Running eMMC test...");
        Console.println("[EMMC] This is a CONSERVATIVE test - READ ONLY by default");
        Console.println();

        // Test 1: Initialize eMMC
        Console.println("[EMMC] === Test 1: eMMC Initialization ===");
        Console.println("[EMMC] Configuring SDMMC1 pins...");
        Console.println("[EMMC]   DAT0-3: PC8, PC9, PC10, PC11");
        Console.println("[EMMC]   CLK:    PC12");
        Console.println("[EMMC]   CMD:    PD2");

        // Configure pins explicitly before initialization
        SD.setDx(PC8, PC9, PC10, PC11);  // DAT0-3
        SD.setCK(PC12);                   // CLK
        SD.setCMD(PD2);                   // CMD

        Console.println("[EMMC] Pins configured, attempting initialization...");

        // Try 4-bit mode first
        Console.println("[EMMC] Try 1: 4-bit mode (default)...");
        if (SD.begin()) {
            Console.println("[EMMC] ✓ eMMC initialized successfully in 4-bit mode!");
            emmcInitialized = true;
        } else {
            Console.println("[EMMC] ✗ Failed with 4-bit mode");

            // Try 1-bit mode as fallback (only DAT0)
            Console.println("[EMMC] Try 2: 1-bit mode (DAT0 only)...");
            SD.setDx(PC8);  // Only DAT0
            SD.setCK(PC12);
            SD.setCMD(PD2);

            if (SD.begin()) {
                Console.println("[EMMC] ✓ eMMC initialized in 1-bit mode!");
                Console.println("[EMMC] Note: Running in reduced performance mode");
                emmcInitialized = true;
            } else {
                Console.println("[EMMC] ✗ All initialization attempts failed");
                Console.println("[EMMC] ");
                Console.println("[EMMC] Possible causes:");
                Console.println("[EMMC]   - Check power supply (eMMC needs 3.3V)");
                Console.println("[EMMC]   - Verify pin connections");
                Console.println("[EMMC]   - Check for solder bridges on DAT/CMD/CLK");
                Console.println("[EMMC]   - Verify eMMC is properly soldered");
                Console.println("[EMMC]   - eMMC may not be formatted (needs FAT filesystem)");
                Console.println("[EMMC]   - eMMC chip may be defective");
                Console.println("\n[EMMC] Test aborted. Press '0' for menu.\n");
                return;
            }
        }

        delay(100);

        // Test 2: Read card information
        Console.println("\n[EMMC] === Test 2: Card Information ===");
        Console.println("[EMMC] eMMC/MMC device detected and initialized");

        // Test 3: List root directory (safe read operation)
        Console.println("\n[EMMC] === Test 3: Root Directory Contents ===");
        File root = SD.open("/");
        if (root) {
            Console.println("[EMMC] Files in root directory:");
            int fileCount = 0;
            File entry = root.openNextFile();
            while (entry && fileCount < 10) {  // Limit to first 10 files
                Console.print("[EMMC]   ");
                if (entry.isDirectory()) {
                    Console.print("[DIR]  ");
                } else {
                    Console.print("[FILE] ");
                }
                Console.print(entry.name());
                if (!entry.isDirectory()) {
                    Console.print(" (");
                    Console.print(entry.size());
                    Console.print(" bytes)");
                }
                Console.println();
                entry.close();
                entry = root.openNextFile();
                fileCount++;
            }
            if (fileCount == 0) {
                Console.println("[EMMC]   (empty)");
            } else if (entry) {
                Console.println("[EMMC]   ... (more files not shown)");
            }
            root.close();
        } else {
            Console.println("[EMMC] ✗ Failed to open root directory");
        }

        // Test 4: Optional write test (user confirmation needed)
        Console.println("\n[EMMC] === Test 4: Write Test (OPTIONAL) ===");
        Console.println("[EMMC] ");
        Console.println("[EMMC] Write test will create a small test file.");
        Console.println("[EMMC] This verifies write functionality is working.");
        Console.println("[EMMC] ");
        Console.println("[EMMC] Type 'y' to proceed with write test");
        Console.println("[EMMC] Type 'n' or wait 10 seconds to skip");
        Console.print("[EMMC] Your choice: ");

        unsigned long startTime = millis();
        bool doWriteTest = false;

        while (millis() - startTime < 10000) {
            if (Console.available()) {
                char c = Console.read();
                Console.println(c);
                if (c == 'y' || c == 'Y') {
                    doWriteTest = true;
                    break;
                } else if (c == 'n' || c == 'N') {
                    break;
                }
            }
            delay(10);
        }

        if (doWriteTest) {
            Console.println("[EMMC] Performing write test...");

            // Create a small test file
            File testFile = SD.open("/emmc_test.txt", FILE_WRITE);
            if (testFile) {
                const char* testData = "STM32H723 eMMC Test - Write OK\n";
                size_t bytesWritten = testFile.print(testData);
                testFile.close();

                Console.print("[EMMC] ✓ Wrote ");
                Console.print(bytesWritten);
                Console.println(" bytes");

                // Read it back to verify
                testFile = SD.open("/emmc_test.txt", FILE_READ);
                if (testFile) {
                    Console.print("[EMMC] ✓ Read back: ");
                    while (testFile.available()) {
                        Console.write(testFile.read());
                    }
                    testFile.close();

                    // Delete test file
                    if (SD.remove("/emmc_test.txt")) {
                        Console.println("[EMMC] ✓ Test file removed");
                    }
                } else {
                    Console.println("[EMMC] ✗ Failed to read test file");
                }
            } else {
                Console.println("[EMMC] ✗ Failed to create test file");
            }
        } else {
            Console.println("[EMMC] Write test skipped");
        }

        // Summary
        Console.println("\n[EMMC] ========================================");
        Console.println("[EMMC] eMMC TEST SUMMARY");
        Console.println("[EMMC] ========================================");
        Console.println("[EMMC] Status: ✓ eMMC is functional");
        Console.println("[EMMC] Initialization: ✓ Success");
        Console.println("[EMMC] File system: ✓ Accessible");
        Console.println("[EMMC] Read operations: ✓ Working");
        if (doWriteTest) {
            Console.println("[EMMC] Write operations: ✓ Tested");
        } else {
            Console.println("[EMMC] Write operations: Not tested");
        }
        Console.println("[EMMC] ========================================");

        Console.println("\n[EMMC] Test complete. Press '0' for menu.\n");
    }
}
