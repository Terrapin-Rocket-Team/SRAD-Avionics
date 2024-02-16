#include "State.h"

State::State()
{
    strcpy(stage, stages[0]);
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
    lastGPSUpdate = millis();
    baro = nullptr;
    gps = nullptr;
    imu = nullptr;
    lisens = nullptr;
    rtc = nullptr;
}
State::~State()
{
    delete[] csvHeader;
}
bool State::init()
{
    int good = 0, numSensors = 0;
    if (baro)
    {
        numSensors++;
        if (baro->initialize())
            good++;
        else
            baro = nullptr;
    }
    if (gps)
    {
        numSensors++;
        if (gps->initialize())
            good++;
        else
            gps = nullptr;
    }
    if (imu)
    {
        numSensors++;
        if (imu->initialize())
            good++;
        else
            imu = nullptr;
    }
    if (lisens)
    {
        numSensors++;
        if (lisens->calibrate()) // This should be changed to init...
            good++;
        else
            lisens = nullptr;
    }
    if (rtc)
    {
        numSensors++;
        if (rtc->initialize())
            good++;
        else
            rtc = nullptr;
    }
    setcsvHeader();
    return good == numSensors;
}
void State::determineaccelerationMagnitude(imu::Vector<3> accel)
{
    accelerationMagnitude = sqrt((accel.x() * accel.x()) + (accel.y() * accel.y()) + (accel.z() * accel.z()));
}

void State::determineapogee(double zPosition)
{
    if (apogee < zPosition)
    {
        apogee = zPosition;
    }
}

void State::determinetimeSincePreviousStage()
{
    timeSincePreviousStage = timeAbsolute - timePreviousStage;
}

void State::determinetimeSinceLaunch()
{
    timeSinceLaunch = timeAbsolute - timeLaunch;
}

void State::updateState()
{
    if (gps)
    {
        gps->read_gps();
        position = imu::Vector<3>(gps->get_pos().x(), gps->get_pos().y(), gps->get_alt());
        velocity = gps->get_velocity();
    }
    if (baro)
    {
        velocity.z() = (baro->get_rel_alt_m() - position.z()) / (millis() - timeAbsolute);
        position.z() = baro->get_rel_alt_m();
    }
    if (imu)
    {
        acceleration = imu->get_acceleration();
        orientation = imu->get_orientation();
    }
    settimeAbsolute();

    if (stageNumber == 0 && acceleration.z() > 25 && position.z() > 75)
    {
        stageNumber = 1;
        timeLaunch = timeAbsolute;
        timePreviousStage = timeAbsolute;
        strcpy(stage, stages[1]);
        stage[strlen(stage) - 1] = '1';  // future support for multi-stage rockets
        digitalWrite(LED_BUILTIN, HIGH); // not sure why this is here because it will never be seen, but leaving it for now
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
    }
    else if (stageNumber == 1 && acceleration.z() < 5)
    {
        stageNumber = 2;
        strcpy(stage, stages[2]);
    }
    else if (stageNumber == 2 && velocity.z() < 0)
    {
        stageNumber = 3;
        strcpy(stage, stages[3]);
    }
    else if (stageNumber == 3 && position.z() < 750 && millis() - timeSinceLaunch > 120000)
    {
        stageNumber = 4;
        strcpy(stage, stages[3]); // because I don't know if we need the extra descent stage, I've not included it for now.
    }
    else if (stageNumber == 4 && velocity.z() > -0.5 && accelerationMagnitude < 5 && baro->get_rel_alt_ft() < 200)
    {
        stageNumber = 5;
        strcpy(stage, stages[4]);
    }

    // backup case to dump data (25 minutes)
    determinetimeSinceLaunch();
    if (stageNumber > 0 && timeSinceLaunch > 1500000 && stageNumber < 5)
    {
        stageNumber = 5;
        strcpy(stage, stages[4]);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void State::setcsvHeader()
{
    char csvHeaderStart[] = "Time,Stage,PX,PY,PZ,VX,VY,VZ,AX,AY,AZ,";
    char *headers[] = {csvHeaderStart, nullptr, nullptr, nullptr, nullptr}; // all stack arrays!!!!
    int cursor = 1;
    delete[] csvHeader; // just in case there is already something there. This function should never be called more than once.

    //---Determine required size for header
    int size = sizeof(csvHeaderStart); // includes '\0' at end of string for the end of csvHeader to use
    if (baro)
    {
        headers[cursor] = baro->getcsvHeader();
        size += strlen(headers[cursor++]);
    }
    if (gps)
    {
        headers[cursor] = gps->getcsvHeader();
        size += strlen(headers[cursor++]);
    }
    if (imu)
    {
        headers[cursor] = imu->getcsvHeader();
        size += strlen(headers[cursor++]);
    }
    if (lisens)
    {
        headers[cursor] = lisens->getcsvHeader();
        size += strlen(headers[cursor++]);
    }
    if (rtc)
    {
        headers[cursor] = rtc->getcsvHeader();
        size += strlen(headers[cursor++]);
    }
    csvHeader = new char[size];

    //---Fill header String
    int j = 0;
    for (int i = 0; headers[i]; i++)
    {
        for (int k = 0; headers[i][k]; j++, k++) // append all the header strings onto the main string
            csvHeader[j] = headers[i][k];
    }
    csvHeader[j - 1] = '\0'; // all strings have ',' at end so this gets rid of that and terminates it a character early.
}

// This doesn't really follow DRY, but I couldn't be bothered to make it more generic because I don't think I'll ever have to write it again.
// famous last words.
void State::setdataString()
{
    // Assuming 12 char/float (2 dec precision, leaving min value of -9,999,999.99), 30 char/string, 10 char/int
    // string * 1, float * 9, int * 0, 11 commas
    // 30 + 108 + 11 = 149
    const int dataStartSize = 30 * 1 + 12 * 9 + 11;
    char csvDataStart[dataStartSize];
    int used = snprintf(
        csvDataStart, dataStartSize,
        "%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", // trailing comma very important
        timeAbsolute / 1000, stage,
        position.x(), position.y(), position.z(),
        velocity.x(), velocity.y(), velocity.z(),
        acceleration.x(), acceleration.y(), acceleration.z());
    // Serial.print(used);//Just curious

    char *data[] = {csvDataStart, nullptr, nullptr, nullptr, nullptr};//all stack arrays
    int cursor = 1;
    delete[] dataString; // This probably definitely exists (although maybe shouldn't if RecordData deletes it when it's done.)

    //---Determine required size for string
    int size = sizeof(csvDataStart); // includes '\0' at end of string for the end of dataString to use
    if (baro)
    {
        data[cursor] = baro->getdataString();
        size += strlen(data[cursor++]);
    }
    if (gps)
    {
        data[cursor] = gps->getdataString();
        size += strlen(data[cursor++]);
    }
    if (imu)
    {
        data[cursor] = imu->getdataString();
        size += strlen(data[cursor++]);
    }
    if (lisens)
    {
        data[cursor] = lisens->getdataString();
        size += strlen(data[cursor++]);
    }
    if (rtc)
    {
        data[cursor] = rtc->getdataString();
        size += strlen(data[cursor++]);
    }
    dataString = new char[size];

    //---Fill data String
    int j = 0;
    for (int i = 0; data[i]; i++)
    {
        for (int k = 0; data[i][k]; j++, k++) // append all the data strings onto the main string
            dataString[j] = data[i][k];
    }
    dataString[j - 1] = '\0'; // all strings have ',' at end so this gets rid of that and terminates it a character early.
}

char *State::getdataString() { return dataString; }
char *State::getcsvHeader() { return csvHeader; }

#pragma region Getters and Setters for Sensors
void State::setBaro(Barometer *nbaro) { baro = nbaro; }
void State::setGPS(GPS *ngps) { gps = ngps; }
void State::setIMU(IMU *nimu) { imu = nimu; }
void State::setLS(LightSensor *nlisens) { lisens = nlisens; }
void State::setRTC(RTC *nrtc) { rtc = nrtc; }
void State::settimeAbsolute() { timeAbsolute = millis(); }

Barometer *State::getBaro() { return baro; }
GPS *State::getGPS() { return gps; }
IMU *State::getIMU() { return imu; }
LightSensor *State::getLS() { return lisens; }
RTC *State::getRTC() { return rtc; }
#pragma endregion
