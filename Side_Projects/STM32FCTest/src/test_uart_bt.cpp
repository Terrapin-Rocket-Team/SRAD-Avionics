#include "../tests/test_uart_bt.h"
#include "../tests/test_menu.h"

// Bluetooth module pins
#define BT_TX PC6  // STM32 TX -> BT RX
#define BT_RX PC7  // STM32 RX -> BT TX
#define BT_BAUD 115200

// Create HardwareSerial instance for Bluetooth
// Note: Check your STM32H7 variant to confirm which USART is on PC6/PC7
// Typically USART6 uses these pins on STM32H7
HardwareSerial BTSerial(BT_RX, BT_TX);

namespace UARTBTTest {

    // Helper function to send AT command and wait for response
    String sendATCommand(const char* cmd, int timeout = 1000) {
        // Clear any pending data
        while (BTSerial.available()) {
            BTSerial.read();
        }

        // Send command
        Console.print("[BT] Sending: ");
        Console.println(cmd);
        BTSerial.println(cmd);

        // Wait for response
        unsigned long startTime = millis();
        String response = "";

        while (millis() - startTime < timeout) {
            if (BTSerial.available()) {
                char c = BTSerial.read();
                response += c;
                // Reset timeout if we're receiving data
                startTime = millis();
            }
            delay(10);
        }

        return response;
    }

    void setup() {
        Console.println("[BT] Initializing UART Bluetooth test...");
        Console.print("[BT] TX: PC6, RX: PC7, Baud: ");
        Console.println(BT_BAUD);

        // Initialize UART for Bluetooth module
        BTSerial.begin(BT_BAUD);
        delay(100);  // Give UART time to stabilize

        Console.println("[BT] UART configured");
        Console.println("[BT] Setup complete");
    }

    void run() {
        Console.println("\n[BT] Running UART Bluetooth test...");
        Console.println("[BT] Testing Feasycom BT691 module with AT commands\n");

        // Test 1: Basic AT command
        Console.println("[BT] === Test 1: Module Detection ===");
        String response = sendATCommand("AT");
        if (response.length() > 0) {
            Console.print("[BT] Response: ");
            Console.println(response);
            Console.println("[BT] ✓ Module detected");
        } else {
            Console.println("[BT] ✗ No response from module");
            Console.println("[BT] Check connections and power");
        }
        delay(500);

        // Test 2: Get firmware version (Feasycom command)
        Console.println("\n[BT] === Test 2: Firmware Version ===");
        response = sendATCommand("AT+VER");
        if (response.length() > 0) {
            Console.print("[BT] Version: ");
            Console.println(response);
        } else {
            Console.println("[BT] ✗ No response");
        }
        delay(500);

        // Test 3: Get MAC address
        Console.println("\n[BT] === Test 3: MAC Address ===");
        response = sendATCommand("AT+ADDR");
        if (response.length() > 0) {
            Console.print("[BT] MAC Address: ");
            Console.println(response);
        } else {
            Console.println("[BT] ✗ No response");
        }
        delay(500);

        // Test 4: Get current module name
        Console.println("\n[BT] === Test 4: Current Device Name ===");
        response = sendATCommand("AT+NAME");
        if (response.length() > 0) {
            Console.print("[BT] Current Name: ");
            Console.println(response);
        } else {
            Console.println("[BT] ✗ No response");
        }
        delay(500);

        // Test 5: Set custom advertising name
        Console.println("\n[BT] === Test 5: Set Custom Advertising Name ===");
        const char* customName = "STM32H723-FC";
        Console.print("[BT] Setting name to: ");
        Console.println(customName);

        String nameCommand = "AT+NAME=";
        nameCommand += customName;
        response = sendATCommand(nameCommand.c_str());

        if (response.indexOf("OK") >= 0) {
            Console.println("[BT] ✓ Name set successfully");

            // Verify the new name
            delay(500);
            Console.println("[BT] Verifying new name...");
            response = sendATCommand("AT+NAME");
            Console.print("[BT] Verified Name: ");
            Console.println(response);
        } else {
            Console.println("[BT] ✗ Failed to set name");
            Console.print("[BT] Response: ");
            Console.println(response);
        }
        delay(500);

        // Test 6: Get advertising interval
        Console.println("\n[BT] === Test 6: Advertising Interval ===");
        response = sendATCommand("AT+ADVIN");
        if (response.length() > 0) {
            Console.print("[BT] Advertising Interval: ");
            Console.println(response);
        } else {
            Console.println("[BT] ✗ No response");
        }
        delay(500);

        // Test 7: Set advertising interval (500ms for better visibility)
        Console.println("\n[BT] === Test 7: Set Advertising Interval ===");
        Console.println("[BT] Setting advertising interval to 500ms");
        response = sendATCommand("AT+ADVIN=500");

        if (response.indexOf("OK") >= 0) {
            Console.println("[BT] ✓ Advertising interval set successfully");
        } else {
            Console.print("[BT] Response: ");
            Console.println(response);
        }
        delay(500);

        // Test 8: Get TX power
        Console.println("\n[BT] === Test 8: TX Power Level ===");
        response = sendATCommand("AT+TXPOWER");
        if (response.length() > 0) {
            Console.print("[BT] TX Power: ");
            Console.println(response);
        } else {
            Console.println("[BT] ✗ No response");
        }
        delay(500);

        // Test 9: Get baud rate
        Console.println("\n[BT] === Test 9: UART Baud Rate ===");
        response = sendATCommand("AT+BAUD");
        if (response.length() > 0) {
            Console.print("[BT] Baud Rate: ");
            Console.println(response);
        } else {
            Console.println("[BT] ✗ No response");
        }
        delay(500);

        // Summary
        Console.println("\n[BT] ========================================");
        Console.println("[BT] ADVERTISING CONFIGURATION SUMMARY");
        Console.println("[BT] ========================================");
        Console.print("[BT] Device Name: ");
        Console.println(customName);
        Console.println("[BT] Advertising: ACTIVE");
        Console.println("[BT] Interval: 500ms");
        Console.println("[BT] ");
        Console.println("[BT] The module should now be visible on");
        Console.println("[BT] Bluetooth scanners as 'STM32H723-FC'");
        Console.println("[BT] ========================================\n");

        // Test 10: Interactive mode
        Console.println("[BT] === Test 10: Interactive Mode ===");
        Console.println("[BT] Enter your own AT commands (30 second timeout)");
        Console.println("[BT] Type commands and press Enter");
        Console.println("[BT] Press '0' to exit and return to menu\n");

        unsigned long startTime = millis();
        String userInput = "";

        while (millis() - startTime < 30000) {
            // Check for user input from USB Console
            if (Console.available()) {
                char c = Console.read();

                if (c == '0' && userInput.length() == 0) {
                    Console.println("\n[BT] Exiting interactive mode...");
                    break;
                }

                if (c == '\n' || c == '\r') {
                    if (userInput.length() > 0) {
                        // Send the command
                        response = sendATCommand(userInput.c_str());
                        if (response.length() > 0) {
                            Console.print("[BT] Response: ");
                            Console.println(response);
                        } else {
                            Console.println("[BT] (no response)");
                        }
                        userInput = "";
                        Console.println();
                    }
                } else if (c >= 32 && c <= 126) {  // Printable characters
                    userInput += c;
                    Console.write(c);  // Echo to console
                }
            }

            // Check for unsolicited data from BT module
            if (BTSerial.available()) {
                Console.print("[BT] Unsolicited: ");
                while (BTSerial.available()) {
                    Console.write(BTSerial.read());
                }
                Console.println();
            }

            delay(10);
        }

        // Clean up: Restore original name and settings
        Console.println("\n[BT] === Cleanup: Restoring Settings ===");
        Console.println("[BT] Restoring original name 'Feasycom'...");
        response = sendATCommand("AT+NAME=Feasycom");
        if (response.indexOf("OK") >= 0) {
            Console.println("[BT] ✓ Name restored");
        }

        delay(500);
        Console.println("[BT] Resetting advertising interval to default...");
        response = sendATCommand("AT+ADVIN=687");
        if (response.indexOf("OK") >= 0) {
            Console.println("[BT] ✓ Advertising interval restored to default (687ms)");
        }
        delay(500);
        Console.println("[BT] Rebooting");
        response = sendATCommand("AT+REBOOT");
        if (response.indexOf("OK") >= 0) {
            Console.println("[BT] ✓ BT System Rebooted to stop advertising and release connections.");
        }

        Console.println("\n[BT] Test complete. Settings restored. Press '0' for menu.\n");
    }
}
