#include <Arduino.h>
#include <RadioLib.h>

SPIClass spi(HSPI);
LR1121 radio = new Module(/*CS*/ 14, /*DIO1*/ 36, /*RST*/ 38, /*BUSY*/ 37, spi);

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

static uint32_t g_seq = 0;

static void printRc(const char *label, int rc)
{
    USBSerial.printf("[PING-TX] %-14s -> %d\n", label, rc);
}

void setup()
{
    USBSerial.begin(115200);
    delay(2500);
    USBSerial.println("\n=== LR1121 raw ping TX ===");

    spi.begin(/*sck*/ 13, /*miso*/ 12, /*mosi*/ 11, /*ss*/ 14);

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
    printRc("explicitHdr", radio.explicitHeader());
    printRc("invertIQ", radio.invertIQ(false));
}

void loop()
{
    static uint32_t lastTx = 0;
    const uint32_t now = millis();
    if (now - lastTx < 1000)
    {
        return;
    }
    lastTx = now;

    char msg[32];
    snprintf(msg, sizeof(msg), "UPPING/%lu", (unsigned long)g_seq++);
    const int rc = radio.transmit(reinterpret_cast<const uint8_t *>(msg), strlen(msg));
    USBSerial.printf("[PING-TX] tx rc=%d data='%s'\n", rc, msg);
}
