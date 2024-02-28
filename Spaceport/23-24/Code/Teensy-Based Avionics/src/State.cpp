#include "State.h"
State::State()
{
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
    stateString = nullptr;
    dataString = nullptr;
    numSensors = 0;
}
State::~State()
{
    delete[] csvHeader;
    delete[] stateString;
}
bool State::init()
{
    int good = 0, tryNumSensors = 0;
    if (baro)
    {
        tryNumSensors++;
        if (baro->initialize()){
            good++;
            recordLogData(INFO, "Barometer initialized");
        }
        else{
            baro = nullptr;
            recordLogData(ERROR, "Barometer failed to initialize");
        }
    }
    if (gps)
    {
        tryNumSensors++;
        if (gps->initialize()){
            good++;
            recordLogData(INFO, "GPS initialized");
        }
        else{
            gps = nullptr;
            recordLogData(ERROR, "GPS failed to initialize");
        }
    }
    if (imu)
    {
        tryNumSensors++;
        if (imu->initialize()){
            good++;
            recordLogData(INFO, "IMU initialized");
        }
        else{
            imu = nullptr;
            recordLogData(ERROR, "IMU failed to initialize");
        }
    }
    if (lisens)
    {
        tryNumSensors++;
        if (lisens->calibrate()){//should be initialize...
            good++;
            recordLogData(INFO, "Light Sensor initialized");
        }
        else{
            lisens = nullptr;
            recordLogData(ERROR, "Light Sensor failed to initialize");
        }
    }
    if (rtc)
    {
        tryNumSensors++;
        if (rtc->initialize()){
            good++;
            recordLogData(INFO, "RTC initialized");
        }
        else{
            rtc = nullptr;
            recordLogData(ERROR, "RTC failed to initialize");
        }
    }
    numSensors = good;
    setcsvHeader();
    return good == tryNumSensors;
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
    if(apogee > zPosition + 10){
        recordLogData(INFO, "Apogee detected");
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
void State::updateSensors()
{
    if (gps && millis() - lastGPSUpdate > 1500)
    {
        gps->read_gps();
        lastGPSUpdate = millis();
    }
    if (baro)
        baro->get_rel_alt_m();
}
void State::updateState()
{
    if(stageNumber > 4 && landingCounter > 50)//if landed and waited 5 seconds, don't update sensors.
        return;
    updateSensors();
    if (gps)
    {
        position = imu::Vector<3>(gps->get_pos().x(), gps->get_pos().y(), gps->get_alt());
        velocity = gps->get_velocity();
    }
    if (baro)
    {
        velocity.z() = (*(double*)baro->get_data() - position.z()) / (millis() - timeAbsolute);//how ugly. get_data() prevents resending commands to the sensor.
        position.z() = *(double *)baro->get_data();
    }
    if (imu)
    {
        acceleration = imu->get_acceleration();
        orientation = imu->get_orientation();
    }
    settimeAbsolute();

    if (stageNumber == 0 && acceleration.z() > 0 && position.z() > 1)
    {
        updateStage(1);
        timeLaunch = timeAbsolute;
        timePreviousStage = timeAbsolute;
        recordLogData(INFO, "Launch detected.");
        recordLogData(INFO, "Printing static data.");
        if(gps){
            recordLogData(INFO, "GPS:");
            recordLogData(INFO, gps->getStaticDataString());
        }
        if(baro){
            recordLogData(INFO, "Barometer:");
            recordLogData(INFO, baro->getStaticDataString());
        }
        if(imu){
            recordLogData(INFO, "IMU:");
            recordLogData(INFO, imu->getStaticDataString());
        }
        if(lisens){
            recordLogData(INFO, "Light Sensor:");
            recordLogData(INFO, lisens->getStaticDataString());
        }
        if(rtc){
            recordLogData(INFO, "RTC:");
            recordLogData(INFO, rtc->getStaticDataString());
        }
    }
    else if (stageNumber == 1 && acceleration.z() < 5)
    {
        updateStage(2);
        recordLogData(INFO, "Coasting detected.");
    }
    else if (stageNumber == 2 && velocity.z() < 1)
    {
        updateStage(3);
        recordLogData(INFO, "Drogue conditions detected.");
    }
    else if (stageNumber == 3 && position.z() < 750 && millis() - timeSinceLaunch > 1200)//This should be lowered
    {
        updateStage(4);
        recordLogData(INFO, "Main parachute conditions detected.");
    }
    else if (stageNumber == 4 && velocity.z() > -0.5 && accelerationMagnitude < 5 && *(double *)baro->get_data() < 200)
    {
        stageNumber = 5;
        recordLogData(INFO, "Landing detected. Waiting for 5 seconds to dump data.");
    }
    else if(stageNumber == 5){
        if (landingCounter++ >= 50)
        { // roughly 5 seconds of data after landing
            updateStage(5);
            recordLogData(INFO, "Dumped data after landing.");
        }
    }
    determineapogee(position.z());
    // backup case to dump data (25 minutes)
    determinetimeSinceLaunch();
    if (stageNumber > 0 && timeSinceLaunch > 1500000 && stageNumber < 5)
    {
        updateStage(5);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        recordLogData(WARNING, "Dumping data after 25 minutes.");
    }
    setdataString();
}

void State::setcsvHeader()
{
    int numCategories = numSensors + 1;
    char csvHeaderStart[] = "Time,Stage,PX,PY,PZ,VX,VY,VZ,AX,AY,AZ,";
    const char **headers = new const char *[numCategories];
    headers[0] = csvHeaderStart;
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
    for (int i = 0; i < numCategories; i++)
    {
        for (int k = 0; headers[i][k] != '\0'; j++, k++) {// append all the header strings onto the main string
            csvHeader[j] = headers[i][k];
        }
    }
    delete[] headers;
    csvHeader[j - 1] = '\0'; // all strings have ',' at end so this gets rid of that and terminates it a character early.
}

// This doesn't really follow DRY, but I couldn't be bothered to make it more generic because I don't think I'll ever have to write it again.
// famous last words.
void State::setdataString()
{
    // Assuming 12 char/float (2 dec precision, leaving min value of -9,999,999.99), 30 char/string, 10 char/int
    // string * 1, float * 9, int * 0, 11 commas
    // 30 + 108 + 11 = 149
    int numCategories = numSensors + 1;
    const int dataStartSize = 30 * 1 + 12 * 9 + 11;
    char csvDataStart[dataStartSize];
    snprintf(
        csvDataStart, dataStartSize,
        "%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", // trailing comma very important
        timeAbsolute / 1000.0, STAGES[stageNumber],
        position.x(), position.y(), position.z(),
        velocity.x(), velocity.y(), velocity.z(),
        acceleration.x(), acceleration.y(), acceleration.z());
    const char **data = new const char *[numCategories];
    data[0] = csvDataStart;
    int cursor = 1;
    delete[] dataString; // This probably definitely exists already.

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
    for (int i = 0; i < numCategories; i++)
    {
        for (int k = 0; data[i][k]; j++, k++) // append all the data strings onto the main string
            dataString[j] = data[i][k];
        if (i >= 1)
            delete[] data[i]; // delete all the heap arrays.
    }
    delete[] data;
    dataString[j - 1] = '\0'; // all strings have ',' at end so this gets rid of that and terminates it a character early.
}

char *State::getStateString()
{
    delete[] stateString;
    stateString = new char[500]; // way oversized for right now.
    snprintf(stateString, 500, "%.2f,%.2f,%s,%.2f|%.2f,%.2f,%.2f|%.2f,%.2f,%.2f|%.7f,%.7f,%.2f|%.2f,%.2f,%.2f,%.2f|%.2f",
             timeAbsolute / 1000.0, timeSinceLaunch / 1000.0, STAGES[stageNumber], timeSincePreviousStage / 1000.0,
             acceleration.x(), acceleration.y(), acceleration.z(),
             velocity.x(), velocity.y(), velocity.z(),
             position.x(), position.y(), position.z(),
             orientation.x(), orientation.y(), orientation.z(), orientation.w(),
             apogee);
    return stateString;
}

char *State::getdataString() { return dataString; }
char *State::getcsvHeader() { return csvHeader; }
int State::getStageNum() { return stageNumber; }
void State::updateStage(int num) { stageNumber = num; dataStageUpdate(num); }
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
