#include "../tests/test_i2c.h"

#include "../tests/test_menu.h"

namespace I2CTest
{
    namespace
    {
        TwoWire &bus()
        {
            return Wire;
        }
    } // namespace

    void setup()
    {
        Console.println("[I2C] Initializing default I2C bus...");
        bus().begin();
        bus().setClock(100000);
        Console.println("[I2C] Bus: Wire");
        Console.println("[I2C] Clock: 100 kHz");
        Console.println("[I2C] Ready");
    }

    void scanBus()
    {
        uint8_t foundCount = 0;

        Console.println("[I2C] Scanning addresses 0x01 through 0x7E...");

        for (uint8_t address = 1; address < 127; ++address)
        {
            bus().beginTransmission(address);
            const uint8_t error = bus().endTransmission();

            if (error == 0)
            {
                Console.print("[I2C] Found device at 0x");
                if (address < 16)
                {
                    Console.print('0');
                }
                Console.println(address, HEX);
                ++foundCount;
            }
        }

        if (foundCount == 0)
        {
            Console.println("[I2C] No devices found.");
        }
        else
        {
            Console.print("[I2C] Found ");
            Console.print(foundCount);
            Console.println(" device(s).");
        }
    }

    void run()
    {
        scanBus();
        Console.println();
        Console.println("[I2C] Scan complete. Press '1' to rescan or '0' for menu.");
        Wire.end();
        pinMode(18, OUTPUT);
        pinMode(19, OUTPUT);
        digitalWrite(18, LOW);
        digitalWrite(19, LOW);
    }
} // namespace I2CTest
