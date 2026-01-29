#include <Arduino.h>

#ifdef STM32

#include <RadioLib.h>

// ==================== PIN DEFINITIONS ====================
#define STATUS_LED PB12

// Radio pins
#define RADIO_NRST PC13
#define RADIO_BUSY PE3
#define RADIO_NCS PA15
#define RADIO_IO9 PE2  // IRQ pin
#define RADIO_MOSI PD7
#define RADIO_MISO PB4
#define RADIO_SCK PB3

// ==================== RADIO SETUP ====================
SPIClass Radio_SPI(RADIO_MOSI, RADIO_MISO, RADIO_SCK);
LR1121 radio = new Module(RADIO_NCS, RADIO_IO9, RADIO_NRST, RADIO_BUSY, Radio_SPI);

// RF switch configuration for LR1121
static const uint32_t rfswitch_dio_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
    RADIOLIB_LR11X0_DIO7, RADIOLIB_NC, RADIOLIB_NC};

static const Module::RfSwitchMode_t rfswitch_table[] = {
    {LR11x0::MODE_STBY, {LOW, LOW, LOW}},
    {LR11x0::MODE_RX, {LOW, LOW, HIGH}},
    {LR11x0::MODE_TX, {LOW, HIGH, LOW}},
    {LR11x0::MODE_TX_HP, {HIGH, LOW, LOW}},
    END_OF_MODE_TABLE,
};

volatile bool radioTransmitFlag = false;

void radioIrqHandler() {
    radioTransmitFlag = true;
}

// ==================== TELEMETRY VARIABLES ====================
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 100;  // Send telemetry every 100ms (10 Hz)
uint32_t telemetryCounter = 0;

// ==================== RADIO INITIALIZATION ====================
void setupRadio() {
    Serial.println("Initializing LR1121 radio...");

    int rc = radio.begin();
    if (rc != RADIOLIB_ERR_NONE) {
        Serial.print("Radio init failed, code: ");
        Serial.println(rc);
        return;
    }

    // Configure RF switch
    radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
    radio.setRegulatorDCDC();

    // Configure radio to match ESP32FC ground station settings
    radio.setFrequency(915.0);
    radio.setSpreadingFactor(7);
    radio.setBandwidth(125.0);
    radio.setCodingRate(5);           // 4/5
    radio.setSyncWord(0x34);          // Match ESP32FC sync word
    radio.setPreambleLength(8);
    radio.setCRC(true);
    radio.setOutputPower(14);         // 14 dBm
    radio.explicitHeader();
    radio.invertIQ(false);

    // Set up IRQ handler
    radio.setIrqAction(radioIrqHandler);

    Serial.println("Radio initialized successfully!");
}

// ==================== TELEM2 TRANSMISSION ====================
void sendTelemetry() {
    // Generate pseudo-random telemetry data
    float flightTime = millis() / 1000.0;
    float altitude = random(0, 10000) / 10.0;     // 0-1000 feet
    float velocity = random(000, 500) / 10.0;    // -50 to 50 knots
    float acceleration = random(-100, 400) / 10.0; // -10 to 40 m/s²
    double latitude = random(38999000, 39000000) / 1000000.0;   // ~38-39 degrees
    double longitude = random(-78000000, -77999000) / 1000000.0; // ~-78 to -77 degrees

    // Format TELEM2 packet: "TELEM2/time,alt,vel,acc,lat,lon"
    char telemetryPacket[128];
    snprintf(telemetryPacket, sizeof(telemetryPacket),
             "TELEM2/%.2f,%.2f,%.2f,%.2f,%.6f,%.6f",
             flightTime, altitude, velocity, acceleration, latitude, longitude);

    // Transmit via LoRa
    int rc = radio.startTransmit(telemetryPacket);
    if (rc == RADIOLIB_ERR_NONE) {
        digitalWrite(STATUS_LED, HIGH);
        telemetryCounter++;

        // Print to serial for debugging
        Serial.print("TX [");
        Serial.print(telemetryCounter);
        Serial.print("]: ");
        Serial.println(telemetryPacket);
    } else {
        Serial.print("TX failed, code: ");
        Serial.println(rc);
    }
}

// ==================== SETUP ====================
void setup() {
    // Initialize status LED
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);

    // Initialize Serial
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n========================================");
    Serial.println("  STM32 LoRa Telemetry Test");
    Serial.println("  Pseudo-Random Data");
    Serial.println("========================================\n");

    // Initialize radio
    setupRadio();
    delay(500);

    Serial.println("\nTransmitter ready!");
    Serial.println("Sending telemetry packets...\n");

    digitalWrite(STATUS_LED, HIGH);
    delay(200);
    digitalWrite(STATUS_LED, LOW);
}

// ==================== MAIN LOOP ====================
void loop() {
    // Handle radio IRQ
    if (radioTransmitFlag) {
        radioTransmitFlag = false;
        digitalWrite(STATUS_LED, LOW);
    }

    // Send telemetry at regular intervals
    unsigned long currentTime = millis();
    if (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL) {
        lastTelemetryTime = currentTime;
        sendTelemetry();
    }

    delay(10);
}

#endif  // STM32
