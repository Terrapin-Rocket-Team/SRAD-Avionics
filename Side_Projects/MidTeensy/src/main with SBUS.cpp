#include <Arduino.h>

// Teensy 4.1 Serial8 TX = pin 35
// Wire pin 35 -> FC's SBUS RX pad, plus common GND

static constexpr uint16_t SBUS_CENTER = 992;
static constexpr uint16_t SBUS_LOW    = 172;
static constexpr uint16_t SBUS_HIGH   = 1811;
static constexpr unsigned long SBUS_FRAME_INTERVAL_US = 14000; // 14ms between frames

static uint16_t channels[16];
static uint8_t sbusPacket[25];
static elapsedMicros frameClock;

void buildSbusPacket(const uint16_t ch[16], uint8_t out[25]) {
    // Header
    out[0] = 0x0F;

    // Pack 16 channels × 11 bits into bytes 1–22
    memset(&out[1], 0, 22);
    for (int i = 0; i < 16; i++) {
        unsigned bitpos = i * 11;
        unsigned byte_idx = bitpos / 8;
        unsigned bit_idx  = bitpos % 8;
        uint32_t val = ch[i] & 0x7FF;
        // Write into bytes 1..22 (offset by 1 for header)
        out[1 + byte_idx]     |= (val << bit_idx) & 0xFF;
        out[1 + byte_idx + 1] |= (val >> (8 - bit_idx)) & 0xFF;
        if (bit_idx > 5) {
            out[1 + byte_idx + 2] |= (val >> (16 - bit_idx)) & 0xFF;
        }
    }

    // Flags: no failsafe, no frame lost
    out[23] = 0x00;
    // Footer
    out[24] = 0x00;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // USB serial for user input
    Serial.begin(115200);
    while (!Serial && millis() < 4000) {}
    Serial.println("SBUS Camera Switch");
    Serial.println("Send 0 = Camera 1, 1 = Camera 2");

    // SBUS: 100000 baud, 8E2, inverted TX
    Serial8.begin(100000, SERIAL_8E2_TXINV);

    // Initialize all channels to center, sticks neutral
    for (int i = 0; i < 16; i++) {
        channels[i] = SBUS_CENTER;
    }
    // Start with camera 1 (AUX1 = channel index 4, low value)
    channels[7] = SBUS_LOW;

    frameClock = 0;
}

void loop() {
    // Handle serial input
    if (Serial.available()) {
        const char input = static_cast<char>(Serial.read());
        if (input == '1') {
            channels[7] = SBUS_HIGH;
            Serial.println("Camera 2 (AUX1 HIGH)");
            digitalWrite(LED_BUILTIN, HIGH);
        } else if (input == '0') {
            channels[7] = SBUS_LOW;
            Serial.println("Camera 1 (AUX1 LOW)");
            digitalWrite(LED_BUILTIN, LOW);
        } else if (input != '\n' && input != '\r') {
            Serial.println("Type 0 or 1");
        }
    }

    // Send SBUS frame at regular intervals
    if (frameClock >= SBUS_FRAME_INTERVAL_US) {
        frameClock -= SBUS_FRAME_INTERVAL_US;
        buildSbusPacket(channels, sbusPacket);
        Serial8.write(sbusPacket, 25);
    }
}