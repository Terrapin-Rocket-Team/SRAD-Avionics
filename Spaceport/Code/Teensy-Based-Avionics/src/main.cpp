#include <Arduino.h>
#include <MMFS.h>
#include "AvionicsState.h"
#include "AvionicsKF.h"
#include "AviEventListener.h"
#include "VoltageSensor.h"
#include "ADXL375.h"
#include "Mahony.h"
#include "LKF.h"
#include "Utils/CircBuffer.h"

void FreeMem();

using namespace mmfs;

MAX_M10S m;

DPS368 d;
BMI088Gyro g;
BMI088Accel a;
mmfs::LIS3MDL l;
VoltageSensor vsfc(A0, 330, 220, "Flight Computer Voltage");
ADXL375 accel;

Sensor *s[] = {&m, &d, &g, &a, &l, &vsfc, &accel};

AvionicsKF fk;
AvionicsState t(s, sizeof(s) / 4, &fk);

MMFSConfig c = MMFSConfig()
                   .withBBAsync(true, 50)
                   .withBBPin(LED_BUILTIN)
                   .withBBPin(32)
                   //    .withBuzzerPin(33)
                   .withUsingSensorBiasCorrection(true)
                   .withUpdateRate(50)
                   .withLoggingRate(10)
                   .withState(&t);
MMFSSystem sys(&c);

AviEventLister listener;

MahonyAHRS ahrs(1.5, 0.002);
unsigned long lastMicros = 0;

LKF lkf(
    Matrix(3, 6, new double[18]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}),
    Matrix::ident(3) * 0.1,
    0.1 // m/s² accel noise
);

void setup()
{
    Serial.begin(115200);
    sys.init();
    bb.aonoff(32, *(new BBPattern(200, 1)), true); // blink a status LED (until GPS fix)
    getLogger().recordLogData(INFO_, "Initialization Complete");
    lastMicros = micros();

    // Example usage:
    // MahonyAHRS ahrs;
    // // During pad static phase:
    for (int i = 0; i < 200; i++)
    {
        a.update();
        g.update();
        ahrs.calibrate(a.getAccel(), g.getAngVel());
        delay(20);
    }
    ahrs.initialize();
}
void loop()
{

    if (sys.update())
    {
        unsigned long now = micros();
        double dt = (now - lastMicros) * 1e-6;
        lastMicros = now;

        Vector<3> acc = a.getAccel();
        Vector<3> gyro = g.getAngVel();
        Vector<3> mag = l.getMag();
        ahrs.update(acc, gyro, dt);
        Quaternion q = ahrs.getQuaternion();
        Vector<3> accelNed = ahrs.toEarthFrame(acc);
        accelNed.z() -= 9.81; // remove gravity

        CircBuffer<Vector<3>> buf(UPDATE_RATE * 2);
        buf.push(accelNed);
        Vector<3> tot;
        int ct = buf.getCount() / 2 - 1;
        for(int i = 0; i < ct; i++)
        {
            tot += buf[i];
        }
        tot.x() /= ct;
        tot.y() /= ct;
        tot.z() /= ct;
        accelNed = accelNed - tot; // remove bias
        
        lkf.update(accelNed, Vector<3>(0, 0, d.getAGLAltM()), dt);
        Vector<6> state = lkf.state();
        Serial.printf("E,%f\n", state[2]);
        auto st = lkf.state();
        Serial.printf("bias_z = %f  err = %f\n",
                      accelNed.z(), // bias on z axis
                      d.getAGLAltM());

        Serial.printf("A,%f,%f,%f\n", accelNed.x(), accelNed.y(), accelNed.z());
        Serial.printf("Q,%f,%f,%f,%f\n", q.w(), q.x(), q.y(), q.z());
        Serial.printf("B,%f\n", d.getAGLAltM());
    }
}

void FreeMem()
{
    void *heapTop = malloc(500);
    Serial.print((long)heapTop);
    Serial.print("\n");
    free(heapTop);
}