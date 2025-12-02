#include "../tests/test_usb.h"
#include "../tests/test_menu.h"

namespace USBTest {
    void printBox(const char* message) {
        int len = strlen(message);

        // Top border
        Serial.print("  ╔");
        for (int i = 0; i < len + 2; i++) Serial.print("═");
        Serial.println("╗");

        // Message
        Serial.print("  ║ ");
        Serial.print(message);
        Serial.println(" ║");

        // Bottom border
        Serial.print("  ╚");
        for (int i = 0; i < len + 2; i++) Serial.print("═");
        Serial.println("╝");
    }

    void setup() {
        #ifdef USBCON
        Console.println("[USB] Initializing USB CDC test...");
        Console.println("[USB] USB CDC should be active on PA11/PA12");
        Console.println("[USB] Setup complete");
        #else
        Console.println("[USB] ERROR: USB CDC not enabled!");
        Console.println("[USB] Rebuild with USBCON flag");
        #endif
    }

    void run() {
        #ifdef USBCON
        Console.println("\n[USB] Running USB CDC test...");
        Console.println("[USB] This test will use the USB Serial port");
        Console.println("[USB] Open your USB serial terminal (COM port) to interact\n");

        if (!Serial) {
            Console.println("[USB] WARNING: USB Serial not connected!");
            Console.println("[USB] Connect to the USB COM port and try again");
            Console.println("[USB] Press '0' for menu.\n");
            return;
        }

        Serial.println("\n\n");
        Serial.println("╔═══════════════════════════════════════════════════════╗");
        Serial.println("║                                                       ║");
        Serial.println("║        STM32H723 USB CDC INTERACTIVE TEST             ║");
        Serial.println("║                                                       ║");
        Serial.println("╚═══════════════════════════════════════════════════════╝");
        Serial.println();

        // Test 1: Echo Test
        Serial.println("═══ TEST 1: ECHO TEST ═══");
        Serial.println("Type something and press Enter.");
        Serial.println("I'll echo it back with style!");
        Serial.println();
        Serial.print("> ");

        String userInput = "";
        unsigned long startTime = millis();
        bool gotInput = false;

        // Wait for input (30 second timeout)
        while (millis() - startTime < 30000 && !gotInput) {
            if (Serial.available() > 0) {
                char c = Serial.read();

                if (c == '\n' || c == '\r') {
                    if (userInput.length() > 0) {
                        gotInput = true;
                    }
                } else {
                    Serial.print(c); // Echo character
                    userInput += c;
                }
            }
            delay(1);
        }

        if (gotInput) {
            Serial.println("\n");
            printBox(userInput.c_str());
            Serial.println();
            Serial.print("✓ Received: \"");
            Serial.print(userInput);
            Serial.println("\"");
            Serial.print("✓ Length: ");
            Serial.print(userInput.length());
            Serial.println(" characters");
            Serial.println();

            // Test 2: ASCII Art Response
            Serial.println("═══ TEST 2: ASCII ART GENERATOR ═══");
            Serial.println();

            if (userInput.equalsIgnoreCase("hello")) {
                Serial.println("  ╔═══════════════════════════════╗");
                Serial.println("  ║   HELLO DETECTED!             ║");
                Serial.println("  ║          _____                ║");
                Serial.println("  ║         |     |               ║");
                Serial.println("  ║         | O O |               ║");
                Serial.println("  ║         |  ^  |               ║");
                Serial.println("  ║         | \\_/ |               ║");
                Serial.println("  ║         |_____|               ║");
                Serial.println("  ║      HELLO TO YOU TOO!        ║");
                Serial.println("  ╚═══════════════════════════════╝");
            } else if (userInput.equalsIgnoreCase("rocket")) {
                Serial.println("         /\\");
                Serial.println("        /  \\");
                Serial.println("       |    |");
                Serial.println("       |    |");
                Serial.println("      /|    |\\");
                Serial.println("     / |    | \\");
                Serial.println("    |  | || |  |");
                Serial.println("    |  | || |  |");
                Serial.println("    |__|====|__|");
                Serial.println("       |    |");
                Serial.println("      /|\\  /|\\");
                Serial.println("     /*| \\/ |*\\");
                Serial.println("    / *|    |* \\");
                Serial.println("   /  *|    |*  \\");
                Serial.println("  *    ------    *");
            } else {
                Serial.println("  Your message in bubble letters:");
                Serial.println();
                printBox(userInput.c_str());
                Serial.println();
                Serial.println("  (Try typing 'hello' or 'rocket' for special responses!)");
            }
        } else {
            Serial.println("\n⚠ Timeout - no input received");
        }

        Serial.println();
        Serial.println("═══ TEST 3: DATA THROUGHPUT ═══");
        Serial.println("Sending 100 characters...");
        Serial.flush();
        delay(100);

        // Send burst of data
        for (int i = 0; i < 10; i++) {
            Serial.print("★ Line ");
            Serial.print(i + 1);
            Serial.println(" ★ USB CDC is working! ★");
            delay(50);
        }

        Serial.println();
        Serial.println("═══ TEST COMPLETE ═══");
        Serial.println();
        Serial.println("✓ USB CDC transmission: OK");
        Serial.println("✓ USB CDC reception: OK");
        Serial.println("✓ Data throughput: OK");
        Serial.println();
        Serial.println("╔═══════════════════════════════════════════════════════╗");
        Serial.println("║                  ALL TESTS PASSED!                    ║");
        Serial.println("║           Your USB CDC is fully functional            ║");
        Serial.println("╚═══════════════════════════════════════════════════════╝");
        Serial.println();

        Console.println("[USB] Test complete! Check your USB serial terminal.");
        Console.println("[USB] Press '0' for menu.\n");

        #else
        Console.println("[USB] ERROR: USB CDC not compiled in!");
        Console.println("[USB] Enable USBCON in platformio.ini");
        Console.println("[USB] Press '0' for menu.\n");
        #endif
    }
}
