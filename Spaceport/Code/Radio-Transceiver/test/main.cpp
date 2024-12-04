#include "Arduino.h"
#include "RadioMessage.h"

void setup()
{
    Serial.begin(9600);

    Message m(3);

    // VideoData test
    Serial.println("Starting video test");
    uint8_t dataIn[2000] = {0};
    uint8_t dataOut[2000] = {0};
    for (int i = 0; i < 2000; i++)
    {
        dataIn[i] = 49; // ascii code for "1"
    }
    VideoData video(dataIn);

    m.encode(&video);
    int totalLen = 0;
    for (unsigned int i = 0; i < (2000u / 50) + 1; i++)
    {
        uint16_t len = 50;
        m.pop(dataOut + totalLen, len);
        totalLen += len;
    }
    Serial.println(totalLen);
    Serial.write(dataIn, 2000);
    Serial.println("");
    Serial.write(dataOut, totalLen);
    Serial.println("");

    /* Expected Output:
    Starting video test
    2000
    11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111
    11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111
    */

    delay(1000);

    // APRSTelem test
    Serial.println("Starting telem test");
    APRSConfig config = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
    double orientTest[3] = {1.0, 110.0, 65.0};
    char out[64];
    APRSTelem telem(config, 39.336896667, -77.337067833, 480.0, 0.0, 31.0, orientTest, (uint32_t)0x15abcdef);
    m.clear();

    m.encode(&telem);
    Serial.println((char *)m.buf);
    Message m2;
    for (int i = 0; i < 3; i++)
    {
        uint16_t len = 20;
        m.pop((uint8_t *)out, len);
        m2.fill((uint8_t *)out, m.size, len); // this builds the message backwards since we're popping
        Serial.write(out, len);               // this shows the backwards output
    }
    Serial.println("");
    Serial.println((char *)m2.buf); // but it's the right way around here

    APRSTelem telemOut(config);
    m2.decode(&telemOut);
    Serial.print("call: ");
    Serial.println(telemOut.config.callsign);
    Serial.print("tocall: ");
    Serial.println(telemOut.config.tocall);
    Serial.print("path: ");
    Serial.println(telemOut.config.path);
    Serial.print("overlay: ");
    Serial.println(telemOut.config.overlay);
    Serial.print("symbol: ");
    Serial.println(telemOut.config.symbol);
    Serial.print("lat: ");
    Serial.println(telemOut.lat);
    Serial.print("lng: ");
    Serial.println(telemOut.lng);
    Serial.print("alt: ");
    Serial.println(telemOut.alt);
    Serial.print("spd: ");
    Serial.println(telemOut.spd);
    Serial.print("hdg: ");
    Serial.println(telemOut.hdg);
    Serial.print("orient: ");
    Serial.print(telemOut.orient[0]);
    Serial.print(" ");
    Serial.print(telemOut.orient[1]);
    Serial.print(" ");
    Serial.println(telemOut.orient[2]);
    Serial.print("flags: ");
    Serial.println(telemOut.stateFlags, HEX);

    /*Expected Output:
    Starting telem test
    KC3UTM>ALL,WIDE1-1:!M:XNe:w7P\ (m!!$dI!8<j1H&<LHZ
    \ (m!!$dI!8<j1H&<LHZL,WIDE1-1:!M:XNe:w7PKC3UTM>AL
    KC3UTM>ALL,WIDE1-1:!M:XNe:w7P\ (m!!$dI!8<j1H&<LHZ
    call: KC3UTM
    tocall: ALL
    path: WIDE1-1
    overlay: M
    symbol: \
    lat: 39.34
    lng: -77.34
    alt: 479.99
    spd: 0.00
    hdg: 31.00
    orient: 1.00 109.99 64.99
    flags: 15ABCDEF
    */

    delay(1000);

    // APRSCmd test
    Serial.println("Starting cmd test");
    uint8_t cmdTest = 0x01;
    uint16_t argsTest = 0x25A5;
    APRSCmd command(config, cmdTest, argsTest);
    m.clear();

    m.encode(&command);
    Serial.println((char *)m.buf);

    APRSCmd commandOut(config);
    m.decode(&commandOut);
    Serial.print("cmd: ");
    Serial.println(commandOut.cmd, HEX);
    Serial.print("args: ");
    Serial.println(commandOut.args, HEX);

    uint16_t len = 19;
    m.shift((uint8_t *)out, len);
    Serial.write(out, len);
    Serial.println("");
    Serial.println(out);
    Serial.println((char *)m.buf);

    /* Expected Output:
    Starting cmd test
    KC3UTM>ALL,WIDE1-1:!""/s
    cmd: 1
    args: 25A5
    KC3UTM>ALL,WIDE1-1:
    KC3UTM>ALL,WIDE1-1:
    !""/s
    */

    delay(1000);

    // APRSText test
    Serial.println("Starting text test");
    char message[67] = "Range test";
    char addr[9] = "KC3UTM";
    APRSConfig configTxt = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};
    APRSText txt(configTxt, message, addr);
    m.clear();

    m.encode(&txt);
    Serial.println((char *)m.buf);

    APRSText txtOut(configTxt);
    m.decode(&txtOut);
    Serial.print("call: ");
    Serial.println(txtOut.config.callsign);
    Serial.print("tocall: ");
    Serial.println(txtOut.config.tocall);
    Serial.print("path: ");
    Serial.println(txtOut.config.path);
    Serial.print("type: ");
    Serial.println(txtOut.config.type);

    Serial.print("msg: ");
    Serial.println(txtOut.msg);
    Serial.print("addr: ");
    Serial.println(txtOut.addressee);

    /* Expected Output:
    Starting text test
    KC3UTM>ALL,WIDE1-1::KC3UTM   :Range test
    call: KC3UTM
    tocall: ALL
    path: WIDE1-1
    type: :
    msg: Range test
    addr: KC3UTM
    */

    delay(1000);

    // multi-message test
    // Note: video not supported, must specify separation character when creating Message
    Serial.println("Starting multi-message test");
    m.clear();
    m.encode(&telem, true)->encode(&command, true)->encode(&txt, true);

    Serial.println(m.size);
    Serial.println((char *)m.buf);

    // deconstruct
    int index = 0;
    while (index < m.size)
    {
        while (index < m.size && m.buf[index] != 3)
        {
            Serial.write((char)m.buf[index++]);
        }
        Serial.write('\n');
        index++;
    }

    /* Expected Output:
    Starting multi-message test
    115
    KC3UTM>ALL,WIDE1-1:!M:XNe:w7P\ (m!!$dI!8<j1H&<LHZ␃KC3UTM>ALL,WIDE1-1:!""/s␃KC3UTM>ALL,WIDE1-1::KC3UTM   :Range test
    KC3UTM>ALL,WIDE1-1:!M:XNe:w7P\ (m!!$dI!8<j1H&<LHZ
    KC3UTM>ALL,WIDE1-1:!""/s
    KC3UTM>ALL,WIDE1-1::KC3UTM   :Range test
*/

    delay(1000);

    // GSData test
    Serial.println("Starting Ground Station data test");
    m.clear();
    m.encode(&telem);
    GSData gs(10, m.buf, m.size);
    m.encode(&gs);

    Serial.write((char *)m.buf, m.size);
    Serial.println("");

    m.clear();
    m.encode(&command);
    gs.fill(m.buf, m.size);
    m.encode(&gs);

    Serial.write((char *)m.buf, m.size);
    Serial.println("");

    m.clear();
    // should be the same as above (but with txt rather than command)
    GSData gsOut;
    m.encode(&txt)->encode(gs.fill(m.buf, m.size))->decode(&gsOut);

    uint8_t type = 0;
    uint16_t size = 0;
    uint32_t header = 0;
    header += m.buf[0] << 8;
    header += m.buf[1];
    GSData::decodeHeader(header, type, size);
    Serial.println(type);
    Serial.println(size);
    Serial.write((char *)gsOut.buf, gsOut.size);
    Serial.write('\n');

    /*Expected Output:
    Starting Ground Station data test
    �1KC3UTM>ALL,WIDE1-1:!M:XNe:w7P\ (m!!$dI!8<j1H&<LHZ
    �␘KC3UTM>ALL,WIDE1-1:!""/s
    10
    40
    KC3UTM>ALL,WIDE1-1::KC3UTM   :Range test
    */

    delay(1000);
}

void loop()
{
}