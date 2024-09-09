#include "Arduino.h"
#include "RadioMessage.h"

void setup()
{
    Serial.begin(9600);
    int packetSize = 64;

    Message m(MSG_VIDEO);
    Packet p(packetSize);

    // video test
    Serial.println("Starting video test");
    uint8_t dataIn[2000] = {0};
    uint8_t dataOut[2000] = {0};
    for (int i = 0; i < 2000; i++)
    {
        dataIn[i] = 49;
    }
    VideoData video(dataIn);

    m.encode(&video);
    for (unsigned int i = 0; i < (2000u / packetSize) + 1; i++)
    {
        m.getPacket(&p, i);
        p.get(dataOut + i * packetSize);
    }
    Serial.write(dataIn, 2000);
    Serial.println("");
    Serial.write(dataOut, 2000);
    Serial.println("");

    delay(1000);

    // APRSTelem test
    Serial.println("Starting telem test");
    APRSConfig config = {"KC3UTM", "ALL", "WIDE1-1", '!', '\\', 'M'};
    double orientTest[3] = {1.0, 110.0, 65.0};
    char out[64];
    APRSTelem telem(config, 39.336896667, -77.337067833, 480.0, 0.0, 31.0, orientTest, (uint32_t)0x15abcdef);
    m.clear();

    Serial.println("encoding");
    m.encode(&telem);
    Serial.println("output");
    Serial.println((char *)m.buf);
    for (int i = 0; i < m.size / packetSize + 1; i++)
    {
        m.getPacket(&p, (uint16_t)i);
        p.get((uint8_t *)out);
        Serial.write(out, p.size);
        Serial.println("");
        Serial.write((char *)p.buf, p.size);
        Serial.println("");
    }

    APRSTelem telemOut(config);
    m.decode(&telemOut);
    Serial.print("call: ");
    Serial.println(telemOut.config.callsign);
    Serial.print("tocall: ");
    Serial.println(telemOut.config.tocall);
    Serial.print("path: ");
    Serial.println(telemOut.config.path);
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

    // m.fill(dataIn, 2000)->decode(&video);
}

void loop()
{
}