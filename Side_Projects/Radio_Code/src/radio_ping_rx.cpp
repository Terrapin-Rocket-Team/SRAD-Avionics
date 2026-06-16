#include <Arduino.h>
#include <RadioLib.h>

#define STATUS_LED PB12
#define RADIO_NRST PA6
#define RADIO_BUSY PA7
#define RADIO_NCS PA15
#define RADIO_IO9 PA2
#define RADIO_MOSI PD7
#define RADIO_MISO PB4
#define RADIO_SCK PB3

SPIClass Radio_SPI(RADIO_MOSI, RADIO_MISO, RADIO_SCK);
LR1121 radio = new Module(RADIO_NCS, RADIO_IO9, RADIO_NRST, RADIO_BUSY, Radio_SPI);

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

volatile uint32_t g_loraOpDone = 0;
volatile uint32_t g_irqCount = 0;

static void onLoraIrq()
{
    g_loraOpDone++;
    g_irqCount++;
}

static void printRc(const char *label, int rc)
{
    Serial.printf("PING-RX %-14s -> %d\n", label, rc);
}

void setup()
{
    Serial.setRx(PB7_ALT1);
    Serial.setTx(PB6_ALT2);
    Serial.begin(115200);
    delay(100);

    pinMode(STATUS_LED, OUTPUT);
    Serial.println("\n=== LR1121 raw ping RX ===");

    printRc("begin", radio.begin());
    printRc("setFrequency", radio.setFrequency(915.0));
    printRc("setSF", radio.setSpreadingFactor(7));
    printRc("setBW", radio.setBandwidth(125.0));
    printRc("setCR", radio.setCodingRate(5));
    printRc("setSync", radio.setSyncWord(0x12));
    printRc("setPreamble", radio.setPreambleLength(8));
    printRc("setCRC", radio.setCRC(true));
    printRc("setPower", radio.setOutputPower(14));

    radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
    radio.setRegulatorDCDC();
    radio.setIrqAction(onLoraIrq);
    printRc("explicitHdr", radio.explicitHeader());
    printRc("invertIQ", radio.invertIQ(false));
    printRc("startReceive", radio.startReceive());
}

void loop()
{
    static uint32_t lastHb = 0;
    const uint32_t now = millis();
    if (now - lastHb >= 1000)
    {
        lastHb = now;
        Serial.printf("PING-RX alive ms=%lu irq=%lu pending=%lu\n",
                      (unsigned long)now,
                      (unsigned long)g_irqCount,
                      (unsigned long)g_loraOpDone);
    }

    while (g_loraOpDone > 0)
    {
        noInterrupts();
        g_loraOpDone--;
        interrupts();

        uint8_t payload[255] = {};
        const size_t packetLen = radio.getPacketLength();
        const int st = radio.readData(payload, packetLen);
        Serial.printf("PING-RX readData=%d len=%u rssi=%.1f snr=%.1f data='",
                      st, (unsigned)packetLen, radio.getRSSI(), radio.getSNR());
        for (size_t i = 0; i < packetLen; ++i)
        {
            const char c = (char)payload[i];
            Serial.write((c >= 32 && c <= 126) ? c : '.');
        }
        Serial.println("'");
        printRc("restart RX", radio.startReceive());
    }
}
