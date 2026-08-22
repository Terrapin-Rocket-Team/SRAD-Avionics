#include "../tests/test_emmc.h"
#include "../tests/test_menu.h"
#include <STM32EMMC.h>

// eMMC pins (SDMMC1 interface)
// PC8  = DAT0
// PC9  = DAT1
// PC10 = DAT2
// PC11 = DAT3
// PC12 = CLK
// PD2  = CMD

namespace EMMCTest {
    namespace {
        constexpr uint32_t kDat0 = PC8;
        constexpr uint32_t kDat1 = PC9;
        constexpr uint32_t kDat2 = PC10;
        constexpr uint32_t kDat3 = PC11;
        constexpr uint32_t kClk = PC12;
        constexpr uint32_t kCmd = PD2;

        bool emmcInitialized = false;
        bool filesystemMounted = false;
        uint64_t cardSize = 0;

        void printErrorCode() {
            Console.print("[EMMC] HAL error code: 0x");
            Console.println(static_cast<uint32_t>(EMMC.lastError()), HEX);
        }

        void printFsError() {
            Console.print("[EMMC] FatFs error: ");
            Console.println(EMMC.lastFsError());
        }

        void printCardInfo() {
            BSP_SD_CardInfo info = {};
            EMMC.getCardInfo(&info);

            cardSize = static_cast<uint64_t>(info.LogBlockNbr) * info.LogBlockSize;

            Console.println("[EMMC] Card info:");
            Console.print("[EMMC]   Card type: 0x");
            Console.println(info.CardType, HEX);
            Console.print("[EMMC]   RCA: 0x");
            Console.println(info.RelCardAdd, HEX);
            Console.print("[EMMC]   Block size: ");
            Console.print(info.LogBlockSize);
            Console.println(" bytes");
            Console.print("[EMMC]   Block count: ");
            Console.println(info.LogBlockNbr);
            Console.print("[EMMC]   Capacity: ");
            Console.print(static_cast<unsigned long>(cardSize / (1024ULL * 1024ULL)));
            Console.println(" MiB");
        }

        void configurePins4Bit() {
            EMMC.setDx(kDat0, kDat1, kDat2, kDat3);
            EMMC.setCK(kClk);
            EMMC.setCMD(kCmd);
        }

        void configurePins1Bit() {
            EMMC.setDx(kDat0, NC, NC, NC);
            EMMC.setCK(kClk);
            EMMC.setCMD(kCmd);
        }

        String readConsoleLine(unsigned long timeoutMs) {
            String input;
            unsigned long startTime = millis();

            while (millis() - startTime < timeoutMs) {
                while (Console.available()) {
                    char c = static_cast<char>(Console.read());

                    if (c == '\r' || c == '\n') {
                        if (input.length() != 0) {
                            Console.println();
                            return input;
                        }
                        continue;
                    }

                    input += c;
                    Console.print(c);
                }
                delay(10);
            }

            return input;
        }
    }

    void setup() {
        Console.println("[EMMC] Initializing eMMC test...");
        Console.println("[EMMC] Pins: PC8-11(DAT0-3), PC12(CLK), PD2(CMD)");
        Console.println("[EMMC] Using SDMMC1 peripheral");
        Console.println("[EMMC] Setup complete");
        Console.println("[EMMC] WARNING: eMMC tests will initialize and");
        Console.println("[EMMC]   read from the device. No writes will be");
        Console.println("[EMMC]   performed in basic tests for safety.");
    }

    void run() {
        emmcInitialized = false;
        filesystemMounted = false;
        cardSize = 0;
        bool doWriteTest = false;

        Console.println("\n[EMMC] Running eMMC test...");
        Console.println("[EMMC] This is a conservative test.");
        Console.println("[EMMC] Raw MMC bring-up and FAT mount are checked separately.");
        Console.println();

        Console.println("[EMMC] === Test 1: Raw eMMC Initialization ===");
        Console.println("[EMMC] Configuring SDMMC1 pins...");
        Console.println("[EMMC]   DAT0-3: PC8, PC9, PC10, PC11");
        Console.println("[EMMC]   CLK:    PC12");
        Console.println("[EMMC]   CMD:    PD2");

        EMMC.endRaw();

        Console.println("[EMMC] Try 1: 4-bit mode...");
        configurePins4Bit();
        if (EMMC.beginRaw()) {
            Console.println("[EMMC] OK: raw MMC link initialized in 4-bit mode");
            emmcInitialized = true;
        } else {
            Console.println("[EMMC] FAIL: 4-bit raw initialization failed");
            printErrorCode();
            EMMC.endRaw();
            delay(50);

            Console.println("[EMMC] Try 2: 1-bit mode (DAT0 only)...");
            configurePins1Bit();
            if (EMMC.beginRaw()) {
                Console.println("[EMMC] OK: raw MMC link initialized in 1-bit mode");
                Console.println("[EMMC] Note: this confirms the device is responding,");
                Console.println("[EMMC]   but some data lines may still be suspect.");
                emmcInitialized = true;
            } else {
                Console.println("[EMMC] FAIL: all raw initialization attempts failed");
                printErrorCode();
                Console.println("[EMMC] ");
                Console.println("[EMMC] Possible causes:");
                Console.println("[EMMC]   - Check 3.3 V power and decoupling at the eMMC");
                Console.println("[EMMC]   - Verify CMD, CLK, and DAT0 continuity");
                Console.println("[EMMC]   - Check for solder bridges on DAT/CMD/CLK");
                Console.println("[EMMC]   - Confirm CMD has a pull-up available");
                Console.println("[EMMC]   - Verify the eMMC is properly soldered");
                Console.println("[EMMC]   - The filesystem is not the issue yet;");
                Console.println("[EMMC]     raw MMC init fails before FAT is involved");
                Console.println("\n[EMMC] Test aborted. Press '0' for menu.\n");
                return;
            }
        }

        delay(100);

        Console.println("\n[EMMC] === Test 2: Card Information ===");
        printCardInfo();

        Console.println("\n[EMMC] === Test 3: FAT Filesystem Mount ===");
        if (EMMC.mount()) {
            Console.println("[EMMC] OK: FAT filesystem mounted");
            filesystemMounted = true;
        } else {
            Console.println("[EMMC] FAIL: raw MMC link is up, but FAT mount failed");
            printFsError();
            Console.println("[EMMC] This usually means the user area is blank,");
            Console.println("[EMMC] unformatted, or formatted with an unsupported FS.");

            Console.println("[EMMC] ");
            Console.println("[EMMC] Type FORMAT and press Enter within 15 seconds");
            Console.println("[EMMC] to erase the eMMC user area and create a FAT volume.");
            Console.print("[EMMC] Confirmation: ");

            String confirmation = readConsoleLine(15000);
            Console.println();

            if (confirmation.equalsIgnoreCase("FORMAT")) {
                Console.println("[EMMC] Formatting eMMC user area as FAT...");
                if (EMMC.format()) {
                    Console.println("[EMMC] OK: format completed and filesystem mounted");
                    filesystemMounted = true;
                } else {
                    Console.println("[EMMC] FAIL: format attempt failed");
                    printFsError();
                }
            } else {
                Console.println("[EMMC] Format skipped");
            }
        }

        if (filesystemMounted) {
            Console.println("\n[EMMC] === Test 4: Root Directory Contents ===");
            File root = EMMC.open("/");
            if (root) {
                Console.println("[EMMC] Files in root directory:");
                int fileCount = 0;
                File entry = root.openNextFile();
                while (entry && fileCount < 10) {
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
                if (entry) {
                    entry.close();
                }
                if (fileCount == 0) {
                    Console.println("[EMMC]   (empty)");
                }
                root.close();
            } else {
                Console.println("[EMMC] FAIL: filesystem mounted but root directory open failed");
            }

            Console.println("\n[EMMC] === Test 5: Write Test (OPTIONAL) ===");
            Console.println("[EMMC] Write test will create a small test file.");
            Console.println("[EMMC] Type 'y' to proceed, 'n' to skip, or wait 10 seconds.");
            Console.print("[EMMC] Your choice: ");

            unsigned long startTime = millis();
            while (millis() - startTime < 10000) {
                if (Console.available()) {
                    char c = Console.read();
                    Console.println(c);
                    if (c == 'y' || c == 'Y') {
                        doWriteTest = true;
                    }
                    break;
                }
                delay(10);
            }

            if (doWriteTest) {
                Console.println("[EMMC] Performing write test...");

                File testFile = EMMC.open("/emmc_test.txt", FILE_WRITE);
                if (testFile) {
                    const char *testData = "STM32H723 eMMC Test - Write OK\n";
                    size_t bytesWritten = testFile.print(testData);
                    testFile.close();

                    Console.print("[EMMC] OK: wrote ");
                    Console.print(bytesWritten);
                    Console.println(" bytes");

                    testFile = EMMC.open("/emmc_test.txt", FILE_READ);
                    if (testFile) {
                        Console.print("[EMMC] OK: read back: ");
                        while (testFile.available()) {
                            Console.write(testFile.read());
                        }
                        testFile.close();

                        if (EMMC.remove("/emmc_test.txt")) {
                            Console.println("[EMMC] OK: test file removed");
                        }
                    } else {
                        Console.println("[EMMC] FAIL: could not reopen test file");
                    }
                } else {
                    Console.println("[EMMC] FAIL: could not create test file");
                }
            } else {
                Console.println("[EMMC] Write test skipped");
            }
        }

        Console.println("\n[EMMC] ========================================");
        Console.println("[EMMC] eMMC TEST SUMMARY");
        Console.println("[EMMC] ========================================");
        Console.print("[EMMC] Raw MMC link: ");
        Console.println(emmcInitialized ? "OK" : "FAIL");
        Console.print("[EMMC] FAT mount: ");
        Console.println(filesystemMounted ? "OK" : "NOT MOUNTED");
        if (cardSize != 0) {
            Console.print("[EMMC] Capacity: ");
            Console.print(static_cast<unsigned long>(cardSize / (1024ULL * 1024ULL)));
            Console.println(" MiB");
        }
        if (filesystemMounted) {
            Console.print("[EMMC] Write test: ");
            Console.println(doWriteTest ? "RUN" : "SKIPPED");
        }
        Console.println("[EMMC] ========================================");

        EMMC.end();
        Console.println("\n[EMMC] Test complete. Press '0' for menu.\n");
    }
}
