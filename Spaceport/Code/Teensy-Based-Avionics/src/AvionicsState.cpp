
#include "AvionicsState.h"

using namespace mmfs;

AvionicsState::AvionicsState(Sensor **sensors, int numSensors, LinearKalmanFilter *kfilter) : State(sensors, numSensors, kfilter),
                                                                                              ahrs(1.5, 0.002),
                                                                                              lkf(
                                                                                                  Matrix(3, 6, new double[18]{1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}),
                                                                                                  Matrix::ident(3) * 0.01,
                                                                                                  0.2 // m/s² accel noise
                                                                                                  ),
                                                                                              buf(UPDATE_RATE * 2)
{
    stage = 0;
    timeOfLaunch = 0;
    timeOfLastStage = 0;
    addColumn(DOUBLE, &orientationFromInitial.w(), "Initial W (quat)");
    addColumn(DOUBLE, &orientationFromInitial.x(), "Initial X (quat)");
    addColumn(DOUBLE, &orientationFromInitial.y(), "Initial Y (quat)");
    addColumn(DOUBLE, &orientationFromInitial.z(), "Initial Z (quat)");
    addColumn(DOUBLE, &orientation.w(), "Global W (quat)");
    addColumn(DOUBLE, &orientation.x(), "Global X (quat)");
    addColumn(DOUBLE, &orientation.y(), "Global Y (quat)");
    addColumn(DOUBLE, &orientation.z(), "Global Z (quat)");
}

bool AvionicsState::init()
{
    bool b = State::init(); // Call the base class init to initialize sensors and columns
    Accel *a1 = reinterpret_cast<Accel *>(getSensor("Accelerometer"_i, 1));
    Gyro *g = reinterpret_cast<Gyro *>(getSensor("Gyroscope"_i));
    if (a1 && g)
    {
        for (int i = 0; i < 200; i++)
        {
            a1->update();
            g->update();
            ahrs.calibrate(a1->getAccel(), g->getAngVel());
            delay(20);
        }
    }
    ahrs.initialize();
    getLogger().recordLogData(LOG_, 100, "Mahony initialized with WXYZ orientation %f, %f, %f, %f",
                              ahrs.getAbsoluteQuaternion().w(),
                              ahrs.getAbsoluteQuaternion().x(),
                              ahrs.getAbsoluteQuaternion().y(),
                              ahrs.getAbsoluteQuaternion().z());
    buf = CircBuffer<Vector<4>>(UPDATE_RATE * 2);
    return b;
}

void AvionicsState::updateVariables()
{
    Accel *a1 = reinterpret_cast<Accel *>(getSensor("Accelerometer"_i, 1));
    Accel *a2 = reinterpret_cast<Accel *>(getSensor("Accelerometer"_i, 2));
    Gyro *g = reinterpret_cast<Gyro *>(getSensor("Gyroscope"_i));
    Barometer *b = reinterpret_cast<Barometer *>(getSensor("Barometer"_i));
    GPS *gps = reinterpret_cast<GPS *>(getSensor("GPS"_i));
    unsigned long now = micros();
    double dt = (now - lastMicros) * 1e-6;
    lastMicros = now;

    // 1) Read both raw accels
    Vector<3> acc;
    if (a1 && *a1 && a2 && *a2) // Check if both accelerometers are available and initialized
    {

        Vector<3> zL = a1->getAccel();
        Vector<3> zH = a2->getAccel();

        double sigmaL = 0.2; // m/s² noise for low‐noise accel
        double sigmaH = 2.0; // m/s² noise for high‐noise
        // 2) Compute fusion weight (example: inverse‐variance)
        double invVarL = 1.0 / (sigmaL * sigmaL);
        double invVarH = 1.0 / (sigmaH * sigmaH);
        double wH = invVarH / (invVarH + invVarL);
        double wL = 1 - wH;
        double clipThreshold = 0.9 * 24 * 9.81; // e.g. 0.9 * 24g
        if (zL.magnitude() > clipThreshold)
        {
            wL = 0;
            wH = 1;
        }

        // 3) Build your single, fused control vector
        acc = zL * wL + zH * wH;
    }
    else if (a1 && *a1) // Only low-noise accel available
    {
        acc = a1->getAccel();
    }
    else if (a2 && *a2) // Only high-noise accel available
    {
        acc = a2->getAccel();
    }
    else
    {
        acc = Vector<3>(0, 0, 9.81); // No accelerometers available
        return;
    }
    Vector<3> gyro = g->getAngVel();
    ahrs.update(acc, gyro, dt);
    Quaternion q = ahrs.getQuaternion();
    Vector<3> accelNed = ahrs.toEarthFrame(acc);
    accelNed.z() -= 9.81;                         // remove gravity
    double alt = (b && *b) ? b->getAGLAltM() : 0; // Use AGL altitude if barometer is available

    if (stage == 0)
    {
        buf.push(Vector<4>(accelNed.x(), accelNed.y(), accelNed.z(), alt));
    }
    Vector<4> tot(0, 0, 0, 0);
    int ct = buf.getCount() / 2 - 1;
    for (int i = 0; i < ct; i++)
    {
        tot += buf[i];
    }
    if (ct > 1)
        tot = tot / (double)(ct * 1.0);

    accelNed.x() -= tot.x();
    accelNed.y() -= tot.y();
    accelNed.z() -= tot.z();
    alt -= tot[3];
    lkf.update(accelNed, Vector<3>(gps->getDisplacement().x(), gps->getDisplacement().y(), alt), dt);
    Vector<6> state = lkf.state();
    // Serial.printf("E,%f\n", state[2]);
    // Serial.printf("Q,%f,%f,%f,%f\n", q.w(), q.x(), q.y(), q.z());
    // Serial.printf("A,%f,%f,%f\n", accelNed.x(), accelNed.y(), accelNed.z());
    // Serial.printf("B,%f\n", alt);

    position = Vector<3>(state[0], state[1], state[2]); // x, y, z in m
    velocity = Vector<3>(state[3], state[4], state[5]);
    acceleration = accelNed;                    // in m/s²
    orientation = ahrs.getAbsoluteQuaternion(); // in quaternion
    orientationFromInitial = q;                 // save the initial orientation for stage detection
}

void AvionicsState::determineStage()
{
    double timeSinceLaunch = currentTime - timeOfLaunch;
    // GPS *gps = reinterpret_cast<GPS *>(getSensor(GPS_));
    Barometer *b = reinterpret_cast<Barometer *>(getSensor("Barometer"_i));
    if (stage == 0 && (position.z() > 2.0 || (b->getAGLAltM() > 10 && acceleration.z() > 10)))
    {
        getLogger().setRecordMode(FLIGHT);
        bb.aonoff(BUZZER, 200);
        stage = 1;
        timeOfLaunch = currentTime;
        timeOfLastStage = currentTime;
        getLogger().recordLogData(INFO_, 100, "Launch detected at %.2f seconds.", timeSinceLaunch);
    } // TODO: Add checks for each sensor being ok and decide what to do if they aren't.
    else if (stage == 1 && abs(acceleration.z()) < 10)
    {
        bb.aonoff(BUZZER, 200, 2);
        timeOfLastStage = currentTime;
        stage = 2;
        getLogger().recordLogData(INFO_, 100, "Coasting detected at %.2f seconds.", timeSinceLaunch);

        // if (Serial8.availableForWrite() > 0) {
        //     Serial8.println("0");
        //     getLogger().recordLogData(INFO_, 100, "RotCam rotated to 0 degrees at %.2f seconds.", timeSinceLaunch);
        // }
    }
    else if (stage == 2 && velocity.z() < 0 && currentTime - timeOfLastStage > 5 /*&& imuVelocity > 102 */)
    {
        bb.aonoff(BUZZER, 200, 3);
        getLogger().recordLogData(INFO_, 100, "Apogee detected at %.2f m.", position.z());
        timeOfLastStage = currentTime;
        stage = 3;
        getLogger().recordLogData(INFO_, 100, "Drogue conditions detected %.2f seconds.", timeSinceLaunch);
    }
    else if (stage == 3 && position.z() * 3.28 < 700 && currentTime - timeOfLastStage > 15)
    {
        bb.aonoff(BUZZER, 200, 4);
        stage = 4;
        timeOfLastStage = currentTime;
        getLogger().recordLogData(INFO_, 100, "MODIFIED Main parachute conditions detected at %.2f seconds.", timeSinceLaunch);

        // if (Serial8.availableForWrite() > 0) {
        //     Serial8.println("180");
        //     getLogger().recordLogData(INFO_, "RotCam rotated 180 degrees.");
        // }
    }
    else if (stage == 4 && velocity.z() < 4 && position.z() < 20 && currentTime - timeOfLastStage > 10)
    {
        bb.aonoff(BUZZER, 200, 5);
        timeOfLastStage = currentTime;
        stage = 5;
        getLogger().recordLogData(INFO_, 100, "Landing detected at %.2f seconds. Waiting for 5 seconds to dump data.", timeSinceLaunch);

        // if (Serial8.availableForWrite() > 0) {
        //     Serial8.println("0");
        //     getLogger().recordLogData(INFO_, "RotCam rotated 0 degrees at %.2f seconds.", timeSinceLaunch);
        // }
    }
    else if ((stage == 5 && currentTime - timeOfLastStage > 5) || (stage >= 1 && stage != 6 && timeSinceLaunch > 10 * 60))
    {
        stage = 0;
        getLogger().setRecordMode(GROUND);
        getLogger().recordLogData(INFO_, 100, "Dumped data after landing at %.2f second.", timeSinceLaunch);
    }
}

double AvionicsState::getTimeSinceLastStage()
{
    return max(millis() / 1000.0 - timeOfLastStage, 0);
}