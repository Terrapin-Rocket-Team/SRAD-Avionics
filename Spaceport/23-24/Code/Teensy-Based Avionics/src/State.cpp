#include "State.h"
#pragma region Constructor and Destructor
// cppcheck-suppress noCopyConstructor
// cppcheck-suppress noOperatorEq
State::State(bool useKalmanFilter, bool stateRecordsOwnFlightData)
{
    lastTimeAbsolute = 0;
    timeAbsolute = millis();
    timePreviousStage = 0;
    position.x() = 0;
    position.y() = 0;
    position.z() = 0;
    velocity.x() = 0;
    velocity.y() = 0;
    velocity.z() = 0;
    acceleration.x() = 0;
    acceleration.y() = 0;
    acceleration.z() = 0;
    apogee = 0;
    stageNumber = 0;
    baroOldAltitude = 0;
    baroVelocity = 0;

    baro = nullptr;
    gps = nullptr;
    imu = nullptr;
    radio = nullptr;

    stateString = nullptr;
    dataString = nullptr;
    csvHeader = nullptr;

    numSensors = 0;
    recordOwnFlightData = stateRecordsOwnFlightData;


    ///

    recordOwnFlightData = false; // For radio testing. Will not record own data unless told to do so.

    ///
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
        sensors[i] = nullptr;

    landingCounter = 0;
    accelerationMagnitude = 0;
    timeOfLaunch = 0;
    timeSinceLaunch = 0;
    timeSincePreviousStage = 0;
    headingAngle = 0;

    useKF = useKalmanFilter;
    // time pos x y z vel x y z acc x y z
    predictions = new double[10]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // gps x y z barometer z
    measurements = new double[4]{1, 1, 1, 1};
    // imu x y z
    inputs = new double[3]{1, 1, 1};
    strcpy(launchTimeOfDay, "00:00:00");
    useKF = useKalmanFilter;
    kfilter = nullptr;
}

State::~State()
{
    delete[] csvHeader;
    delete[] stateString;
    delete[] predictions;
    delete[] measurements;
    delete[] inputs;
    delete kfilter;
}

#pragma endregion

bool State::init()
{
    char *logData = new char[100];
    int good = 0, tryNumSensors = 0;
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if (sensors[i])
        {
            tryNumSensors++;
            if (sensors[i]->initialize())
            {
                good++;
                strcpy(logData, sensors[i]->getTypeString()); // This is a lot for just some logging...
                strcat(logData, " [");
                strcat(logData, sensors[i]->getName());
                strcat(logData, "] initialized.");
                recordLogData(INFO, logData);
            }
            else
            {
                strcpy(logData, sensors[i]->getTypeString());
                strcat(logData, " [");
                strcat(logData, sensors[i]->getName());
                strcat(logData, "] failed to initialize.");
                recordLogData(ERROR, logData);
            }
        }
        else
        {
            strcpy(logData, "Sensor [");
            strcat(logData, SENSOR_NAMES[i]);
            strcat(logData, "] was not added via addSensor().");
            recordLogData(ERROR, logData);
        }
    }
    if (radio)
    {
        if (!radio->init())
            radio = nullptr;
    }
    setCsvHeader();
    if (useKF)
        kfilter = initializeFilter();
    numSensors = good;

    return good == tryNumSensors;
}

void State::updateSensors()
{
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if (sensorOK(sensors[i]))
        { // not nullptr and initialized
            sensors[i]->update();
            Wire.beginTransmission(0x42); // random address for testing the i2c bus
            byte b = Wire.endTransmission();
            if (b != 0x00)
            {
                Wire.end();
                Wire.begin();
                recordLogData(ERROR, "I2C Error");
                sensors[i]->update();
                delay(10);
                sensors[i]->update();
            }
        }
    }
}

void State::updateState(double newTimeAbsolute)
{
    if (stageNumber > 4 && landingCounter > 300) // if landed and waited 5 seconds, don't update sensors.
        return;

    lastTimeAbsolute = timeAbsolute;
    if (newTimeAbsolute != -1)
        timeAbsolute = newTimeAbsolute;
    else
        timeAbsolute = millis() / 1000.0;

    updateSensors();
    if (kfilter && sensorOK(gps) && gps->getHasFirstFix() && stageNumber > 0)
    {
        measurements = new double[3]{0, 0, 0};
        inputs = new double[3]{0, 0, 0};
        // gps x y z barometer z
        measurements[0] = gps->getDisplace().x();
        measurements[1] = gps->getDisplace().y();
        measurements[2] = sensorOK(baro) ? baro->getRelAltM() : 0;
        // imu x y z
        if (sensorOK(imu))
        {
            inputs[0] = imu->getAcceleration().x();
            inputs[1] = imu->getAcceleration().y();
            inputs[2] = imu->getAcceleration().z();
        }
        else // If this is false, the filter is basically useless as far as I understand.
        {
            inputs[0] = 0;
            inputs[1] = 0;
            inputs[2] = 0;
        }
        delete[] predictions;
        predictions = iterateFilter(kfilter, timeAbsolute - lastTimeAbsolute, inputs, measurements);
        // pos x, y, z, vel x, y, z
        position.x() = predictions[0];
        position.y() = predictions[1];
        position.z() = predictions[2];
        velocity.x() = predictions[3];
        velocity.y() = predictions[4];
        velocity.z() = predictions[5];
        acceleration.x() = inputs[0];
        acceleration.y() = inputs[1];
        acceleration.z() = inputs[2];

        orientation = sensorOK(imu) ? imu->getOrientation() : imu::Quaternion(0, 0, 0, 1);

        if (sensorOK(baro))
        {
            baroVelocity = (baro->getRelAltM() - baroOldAltitude) / (millis() / 1000.0 - timeAbsolute);
            baroOldAltitude = baro->getRelAltM();
        }
    }
    else
    {
        if (sensorOK(gps))
        {
            position = imu::Vector<3>(gps->getDisplace().x(), gps->getDisplace().y(), gps->getAlt());
            velocity = gps->getVelocity();
            headingAngle = gps->getHeading();
        }
        if (sensorOK(baro))
        {
            velocity.z() = (baro->getRelAltM() - baroOldAltitude) / (millis() / 1000.0 - lastTimeAbsolute);
            position.z() = baro->getRelAltM();
            baroVelocity = (baro->getRelAltM() - baroOldAltitude) / (millis() / 1000.0 - lastTimeAbsolute);
            baroOldAltitude = baro->getRelAltM();
        }
        if (sensorOK(imu))
        {
            acceleration = imu->getAcceleration();
            orientation = imu->getOrientation();
        }
    }

    if (stageNumber == 0)
        timeSinceLaunch = 0;
    else
        timeSinceLaunch = timeAbsolute - timeOfLaunch;

    determineAccelerationMagnitude();
    determineStage();
    if (stageNumber > 0)
        timeSincePreviousStage = timeAbsolute - timePreviousStage;
    if (stageNumber < 3)
        apogee = position.z();

    // backup case to dump data (25 minutes)
    if (stageNumber > 0 && timeSinceLaunch > 1500 && stageNumber < 5)
    {
        stageNumber = 5;
        setRecordMode(GROUND);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        recordLogData(WARNING, "Dumping data after 25 minutes.");
    }
    setDataString();
    if (recordOwnFlightData)
        recordFlightData(dataString);
}

void State::setCsvHeader()
{
    char csvHeaderStart[] = "Time,Stage,PX,PY,PZ,VX,VY,VZ,AX,AY,AZ,";
    setCsvString(csvHeader, csvHeaderStart, sizeof(csvHeaderStart), true);
}

void State::setDataString()
{
    // Assuming 12 char/float (2 dec precision, leaving min value of -9,999,999.99), 30 char/string, 10 char/int
    // string * 1, float * 9, int * 0, 11 commas
    // 30 + 108 + 11 = 149
    const int dataStartSize = 30 * 1 + 12 * 9 + 12;
    char csvDataStart[dataStartSize];
    snprintf(
        csvDataStart, dataStartSize,
        "%.3f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", // trailing comma very important
        timeAbsolute, STAGES[stageNumber],
        position.x(), position.y(), position.z(),
        velocity.x(), velocity.y(), velocity.z(),
        acceleration.x(), acceleration.y(), acceleration.z());
    setCsvString(dataString, csvDataStart, dataStartSize, false);
}

char *State::getStateString()
{
    delete[] stateString;
    stateString = new char[500]; // way oversized for right now.
    snprintf(stateString, 500, "%.2f,%.2f,%s,%.2f|%.2f,%.2f,%.2f|%.2f,%.2f,%.2f|%.7f,%.7f,%.2f|%.2f,%.2f,%.2f,%.2f|%.2f",
             timeAbsolute, timeSinceLaunch, STAGES[stageNumber], timeSincePreviousStage,
             acceleration.x(), acceleration.y(), acceleration.z(),
             velocity.x(), velocity.y(), velocity.z(),
             position.x(), position.y(), position.z(),
             orientation.x(), orientation.y(), orientation.z(), orientation.w(),
             apogee);
    return stateString;
}

char *State::getDataString() { return dataString; }
char *State::getCsvHeader() { return csvHeader; }
int State::getStageNum() { return stageNumber; }

#pragma region Getters and Setters for Sensors

void State::setRecordOwnFlightData(bool value) { recordOwnFlightData = value; }
bool State::getRecordOwnFlightData() { return recordOwnFlightData; }

void State::setRadio(Radio *r) { radio = r; }
Radio *State::getRadio() { return radio; }
bool State::addSensor(Sensor *sensor, int sensorNum)
{
    int modifiedNum = sensorNum;
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if (SENSOR_ORDER[i] == sensor->getType() && --modifiedNum == 0)
        {
            sensors[i] = sensor;
            return applySensorType(i, sensorNum);
        }
    }
    return false;
}

Sensor *State::getSensor(SensorType type, int sensorNum)
{
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if (SENSOR_ORDER[i] == sensors[i]->getType() && --sensorNum == 0)
        {
            return sensors[i];
        }
    }
    return nullptr;
}

// deprecated
Barometer *State::getBaro() { return baro; }
GPS *State::getGPS() { return gps; }
IMU *State::getIMU() { return imu; }
#pragma endregion

#pragma region Helper Functions

bool State::applySensorType(int i, int sensorNum)
{
    bool good = true;
    switch (sensors[i]->getType())
    {
    case BAROMETER_:

        if (sensorNum == 1)
            baro = reinterpret_cast<Barometer *>(sensors[i]); // normally this would be a dynamic cast, but Arduino doesn't support it.
        // else if (sensorNum == 2)//If you have more than one of the same type, add them like so.
        //    baro2 = reinterpret_cast<Barometer *>(sensors[i]);
        else
            good = false;
        break;

    case GPS_:

        if (sensorNum == 1)
            gps = reinterpret_cast<GPS *>(sensors[i]);
        else
            good = false;
        break;

    case IMU_:

        if (sensorNum == 1)
            imu = reinterpret_cast<IMU *>(sensors[i]);
        else
            good = false;
        break;

    default:
        good = false;
        break;
    }
    return good;
}

void State::determineAccelerationMagnitude()
{
    accelerationMagnitude = sqrt((acceleration.x() * acceleration.x()) + (acceleration.y() * acceleration.y()) + (acceleration.z() * acceleration.z()));
}

void State::determineStage()
{
    if (stageNumber == 0 &&
        (sensorOK(imu) || sensorOK(baro)) &&
        (sensorOK(imu) ? abs(imu->getAcceleration().z()) > 25 : true) &&
        (sensorOK(baro) ? baro->getRelAltFt() > 60 : true))
    // if we are in preflight AND
    // we have either the IMU OR the barometer AND
    // imu is ok AND the z acceleration is greater than 29 ft/s^2 OR imu is not ok AND
    // barometer is ok AND the relative altitude is greater than 30 ft OR baro is not ok

    // essentially, if we have either sensor and they meet launch threshold, launch. Otherwise, it will never detect a launch.
    {
        recordLogData(INFO, "Launch detected.");
        launch();
    } // TODO: Add checks for each sensor being ok and decide what to do if they aren't.
    else if (stageNumber == 1 && abs(acceleration.z()) < 10)
    {
        bb.aonoff(BUZZER, 200, 2);
        timePreviousStage = timeAbsolute;
        stageNumber = 2;
        recordLogData(INFO, "Coasting detected.");
    }
    else if (stageNumber == 2 && baroVelocity <= 0 && timeSinceLaunch > 5)
    {
        bb.aonoff(BUZZER, 200, 3);
        char logData[100];
        snprintf(logData, 100, "Apogee detected at %.2f m.", position.z());
        recordLogData(INFO, logData);
        timePreviousStage = timeAbsolute;
        stageNumber = 3;
        recordLogData(INFO, "Drogue conditions detected.");
    }
    else if (stageNumber == 3 && baro->getRelAltFt() < 1000 && timeSinceLaunch > 10)
    {
        bb.aonoff(BUZZER, 200, 4);
        stageNumber = 4;
        timePreviousStage = timeAbsolute;
        recordLogData(INFO, "Main parachute conditions detected.");
    }
    else if (stageNumber == 4 && baroVelocity > -1 && baro->getRelAltFt() < 66 && timeSinceLaunch > 15)
    {
        bb.aonoff(BUZZER, 200, 5);
        timePreviousStage = timeAbsolute;
        stageNumber = 5;
        recordLogData(INFO, "Landing detected. Waiting for 5 seconds to dump data.");
    }
    else if (stageNumber == 5 && timeSinceLaunch > 20)
    {
        if (landingCounter++ >= 300)
        { // roughly 5 seconds of data after landing
            setRecordMode(GROUND);
            recordLogData(INFO, "Dumped data after landing.");
        }
    }
}
void State::launch()
{
    recordOwnFlightData = true; // just in case this wasnt already set
    bb.aonoff(BUZZER, 200);
    setRecordMode(FLIGHT);
    stageNumber = 1;
    timeOfLaunch = timeAbsolute;
    timePreviousStage = timeAbsolute;
    timeSinceLaunch = 0;
    strcpy(launchTimeOfDay, gps->getTimeOfDay());
    recordLogData(INFO, "Printing static data.");
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if (sensorOK(sensors[i]))
        {
            char logData[200];
            snprintf(logData, 200, "%s: %s", sensors[i]->getName(), sensors[i]->getStaticDataString());
            recordLogData(INFO, logData);
            sensors[i]->setBiasCorrectionMode(false);
        }
    }
}
void State::setCsvString(char *dest, const char *start, int startSize, bool header)
{
    int numCategories = numSensors + 1;
    const char **str = new const char *[numCategories];
    str[0] = start;
    int cursor = 1;
    delete[] dest;
    //---Determine required size for string
    int size = startSize + 1; // includes '\0' at end of string for the end of dataString to use
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if (sensorOK(sensors[i]))
        {
            str[cursor] = header ? sensors[i]->getCsvHeader() : sensors[i]->getDataString();
            size += strlen(str[cursor++]);
        }
    }
    dest = new char[size];
    if (header)
        csvHeader = dest;
    else
        dataString = dest;
    //---Fill data String
    int j = 0;
    for (int i = 0; i < numCategories; i++)
    {
        for (int k = 0; str[i][k] != '\0'; j++, k++)
        { // append all the data strings onto the main string

            dest[j] = str[i][k];
        }
        if (i >= 1 && !header)
        {
            delete[] str[i]; // delete all the heap arrays.
        }
    }
    delete[] str;
    dest[j - 1] = '\0'; // all strings have ',' at end so this gets rid of that and terminates it a character early.
}

bool State::sensorOK(const Sensor *sensor)
{
    if (sensor && *sensor) // not nullptr and initialized
        return true;
    return false;
}

#pragma endregion

void State::fillAPRSData(APRSTelemData &data)
{
    data.lat = sensorOK(gps) ? gps->getPos().x() : 0;
    data.lng = sensorOK(gps) ? gps->getPos().y() : 0;
    data.alt = sensorOK(baro) ? baro->getRelAltFt() : 0;
    data.hdg = (int)headingAngle;
    data.spd = abs((int)(baroVelocity * 3.28));
    data.stage = stageNumber;
    data.orientation = imu->getOrientation().toEuler();
    data.orientation.toDegrees();
    for (int i = 0; i < 3; i++)
    {
        if (data.orientation[i] < 0)
            data.orientation[i] += 360;
    }
}

extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;
uint32_t State::FreeMem()
{
    return (char *)&_heap_end - __brkval;
}