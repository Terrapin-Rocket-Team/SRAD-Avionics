#include "../tests/test_i2c.h"
#include "../tests/test_menu.h"

// I2C pins
#define I2C_SDA PB9
#define I2C_SCL PB8

// Sensor I2C addresses
#define SAM_M10Q_ADDR       0x42  // u-blox GNSS module
#define BMI088_ACCEL_ADDR   0x18  // BMI088 Accelerometer (or 0x19 if SDO high)
#define BMI088_GYRO_ADDR    0x68  // BMI088 Gyroscope (or 0x69 if SDO high)
#define H3LIS331DL_ADDR     0x18  // H3LIS331DL High-g Accelerometer (or 0x19 if SDO high)
#define MMC5603_ADDR        0x30  // MMC5603 Magnetometer
#define DPS368_ADDR         0x77  // DPS368 Barometer (or 0x76 if SDO low)

// WHO_AM_I / ID registers
#define BMI088_ACCEL_CHIP_ID_REG  0x00
#define BMI088_ACCEL_CHIP_ID      0x1E
#define BMI088_GYRO_CHIP_ID_REG   0x00
#define BMI088_GYRO_CHIP_ID       0x0F
#define H3LIS331DL_WHO_AM_I_REG   0x0F
#define H3LIS331DL_WHO_AM_I       0x32
#define MMC5603_PRODUCT_ID_REG    0x39
#define MMC5603_PRODUCT_ID        0x10
#define DPS368_PROD_ID_REG        0x0D
#define DPS368_PROD_ID            0x10

namespace I2CTest {
    void setup() {
        Console.println("[I2C] Initializing I2C test...");
        Console.println("[I2C] SDA: PB9, SCL: PB8");

        // Initialize I2C with custom pins
        Wire.setSDA(I2C_SDA);
        Wire.setSCL(I2C_SCL);
        Wire.begin();
        Wire.setClock(400000);  // 400kHz Fast Mode

        Console.println("[I2C] I2C bus configured at 400kHz");
        Console.println("[I2C] Setup complete");
    }

    void scanBus() {
        Console.println("[I2C] Scanning I2C bus...");
        byte count = 0;

        for (byte addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            byte error = Wire.endTransmission();

            if (error == 0) {
                Console.print("[I2C] Device found at 0x");
                if (addr < 16) Console.print("0");
                Console.println(addr, HEX);
                count++;
            }
        }

        if (count == 0) {
            Console.println("[I2C] No devices found");
        } else {
            Console.print("[I2C] Found ");
            Console.print(count);
            Console.println(" device(s)");
        }
    }

    // Helper function to read a single register
    uint8_t readRegister(uint8_t addr, uint8_t reg) {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(addr, (uint8_t)1);
        if (Wire.available()) {
            return Wire.read();
        }
        return 0xFF;  // Error value
    }

    // Test individual sensor by reading its ID register
    bool testSensor(const char* name, uint8_t addr, uint8_t idReg, uint8_t expectedId) {
        Console.print("[I2C] Testing ");
        Console.print(name);
        Console.print(" at 0x");
        if (addr < 16) Console.print("0");
        Console.print(addr, HEX);
        Console.print("... ");

        uint8_t id = readRegister(addr, idReg);

        if (id == expectedId) {
            Console.print("✓ ID: 0x");
            if (id < 16) Console.print("0");
            Console.println(id, HEX);
            return true;
        } else if (id == 0xFF) {
            Console.println("✗ No response");
            return false;
        } else {
            Console.print("✗ Wrong ID: 0x");
            if (id < 16) Console.print("0");
            Console.print(id, HEX);
            Console.print(" (expected 0x");
            if (expectedId < 16) Console.print("0");
            Console.print(expectedId, HEX);
            Console.println(")");
            return false;
        }
    }

    void run() {
        Console.println("\n[I2C] Running I2C sensors test...");
        Console.println("[I2C] Testing 5 sensors on the bus\n");

        // First, scan the entire bus
        Console.println("[I2C] === Step 1: Bus Scan ===");
        scanBus();
        Console.println();

        // Test each sensor individually
        Console.println("[I2C] === Step 2: Sensor Identification ===");

        int passCount = 0;
        int totalSensors = 5;

        // Test 1: SAM-M10Q GNSS
        Console.println("\n[I2C] [1/5] SAM-M10Q GNSS Module");
        Wire.beginTransmission(SAM_M10Q_ADDR);
        byte error = Wire.endTransmission();
        if (error == 0) {
            Console.println("[I2C] ✓ SAM-M10Q detected at 0x42");
            Console.println("[I2C] Note: GNSS modules don't have WHO_AM_I registers");
            passCount++;
        } else {
            Console.println("[I2C] ✗ SAM-M10Q not responding");
        }

        // Test 2: BMI088 Accelerometer
        Console.println("\n[I2C] [2/5] BMI088 Accelerometer");
        if (testSensor("BMI088 Accel", BMI088_ACCEL_ADDR, BMI088_ACCEL_CHIP_ID_REG, BMI088_ACCEL_CHIP_ID)) {
            passCount++;
        } else {
            // Try alternate address
            Console.println("[I2C] Trying alternate address 0x19...");
            if (testSensor("BMI088 Accel", 0x19, BMI088_ACCEL_CHIP_ID_REG, BMI088_ACCEL_CHIP_ID)) {
                passCount++;
            }
        }

        // Test 3: BMI088 Gyroscope
        Console.println("\n[I2C] [3/5] BMI088 Gyroscope");
        if (testSensor("BMI088 Gyro", BMI088_GYRO_ADDR, BMI088_GYRO_CHIP_ID_REG, BMI088_GYRO_CHIP_ID)) {
            passCount++;
        } else {
            // Try alternate address
            Console.println("[I2C] Trying alternate address 0x69...");
            if (testSensor("BMI088 Gyro", 0x69, BMI088_GYRO_CHIP_ID_REG, BMI088_GYRO_CHIP_ID)) {
                passCount++;
            }
        }

        // Test 4: H3LIS331DL High-g Accelerometer
        Console.println("\n[I2C] [4/5] H3LIS331DL High-g Accelerometer");
        // Note: H3LIS331DL shares address with BMI088 accel, need to check based on ID
        if (testSensor("H3LIS331DL", H3LIS331DL_ADDR, H3LIS331DL_WHO_AM_I_REG, H3LIS331DL_WHO_AM_I)) {
            passCount++;
        } else {
            // Try alternate address
            Console.println("[I2C] Trying alternate address 0x19...");
            if (testSensor("H3LIS331DL", 0x19, H3LIS331DL_WHO_AM_I_REG, H3LIS331DL_WHO_AM_I)) {
                passCount++;
            }
        }

        // Test 5: MMC5603 Magnetometer
        Console.println("\n[I2C] [5/5] MMC5603 Magnetometer");
        if (testSensor("MMC5603", MMC5603_ADDR, MMC5603_PRODUCT_ID_REG, MMC5603_PRODUCT_ID)) {
            passCount++;
        }

        // Test 6: DPS368 Barometer
        Console.println("\n[I2C] [6/6] DPS368 Barometric Pressure Sensor");
        if (testSensor("DPS368", DPS368_ADDR, DPS368_PROD_ID_REG, DPS368_PROD_ID)) {
            passCount++;
            totalSensors++;  // We actually have 6 sensors
        } else {
            // Try alternate address
            Console.println("[I2C] Trying alternate address 0x76...");
            if (testSensor("DPS368", 0x76, DPS368_PROD_ID_REG, DPS368_PROD_ID)) {
                passCount++;
                totalSensors++;
            }
        }

        // Summary
        Console.println("\n[I2C] ========================================");
        Console.println("[I2C] SENSOR TEST SUMMARY");
        Console.println("[I2C] ========================================");
        Console.print("[I2C] Sensors detected: ");
        Console.print(passCount);
        Console.print("/");
        Console.println(totalSensors);

        if (passCount == totalSensors) {
            Console.println("[I2C] ✓ All sensors working!");
        } else {
            Console.print("[I2C] ⚠ ");
            Console.print(totalSensors - passCount);
            Console.println(" sensor(s) not detected");
            Console.println("[I2C] Check wiring and power");
        }
        Console.println("[I2C] ========================================");

        Console.println("\n[I2C] Test complete. Press '0' for menu.\n");
    }
}
