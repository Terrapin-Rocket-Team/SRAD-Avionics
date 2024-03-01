#include "State.h"

#pragma region Constructor and Destructor
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

    baro = nullptr;
    gps = nullptr;
    imu = nullptr;

    stateString = nullptr;
    dataString = nullptr;
    csvHeader = nullptr;

    numSensors = 0;
    recordOwnFlightData = true;
    for(int i = 0; i < NUM_MAX_SENSORS; i++)
        sensors[i] = nullptr;

    landingCounter = 0;
    accelerationMagnitude = 0;
    timeOfLaunch = 0;
    timeSinceLaunch = 0;
    timeSincePreviousStage = 0;
}
State::~State()
{
    delete[] csvHeader;
    delete[] stateString;
}

#pragma endregion

bool State::init(bool stateRecordsOwnFlightData)
{
    recordOwnFlightData = stateRecordsOwnFlightData;
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
                sensors[i] = nullptr;
            }
        }
    }
    if (radio)
    {
        if (!radio->begin())
            radio = nullptr;
    }
    numSensors = good;
    setCsvHeader();
    return good == tryNumSensors;
}

void State::updateSensors()
{
    for(int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if(sensors[i])
            sensors[i]->update();
    }
}

void State::updateState()
{
    if (stageNumber > 4 && landingCounter >= 50) // if landed and waited 5 seconds, don't update sensors.
        return;
    updateSensors();
    if (*gps)
    {
        position = imu::Vector<3>((*gps)->get_pos().x(), (*gps)->get_pos().y(), (*gps)->get_alt());
        velocity = (*gps)->get_velocity();
        heading_angle = (*gps)->get_heading();
    }
    if (*baro)
    {
        velocity.z() = ((*baro)->get_rel_alt_m() - position.z()) / (millis() - timeAbsolute);
        position.z() = (*baro)->get_rel_alt_m();
    }
    if (*imu)
    {
        acceleration = (*imu)->get_acceleration();
        orientation = (*imu)->get_orientation();
    }
    timeAbsolute = millis();
    determineAccelerationMagnitude();
    determineStage();
    if (stageNumber < 3)
        apogee = position.z();
    // backup case to dump data (25 minutes)
    timeSinceLaunch = timeAbsolute - timeOfLaunch;
    if (stageNumber > 0 && timeSinceLaunch > 1500000 && stageNumber < 5)
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
    const int dataStartSize = 30 * 1 + 12 * 9 + 11;
    char csvDataStart[dataStartSize];
    snprintf(
        csvDataStart, dataStartSize,
        "%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", // trailing comma very important
        timeAbsolute / 1000.0, STAGES[stageNumber],
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
             timeAbsolute / 1000.0, timeSinceLaunch / 1000.0, STAGES[stageNumber], timeSincePreviousStage / 1000.0,
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
void State::setRadio(Radio *r) { radio = r; }
Radio *State::getRadio() { return radio; }
bool State::addSensor(Sensor *sensor)
{
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if (sensors[i] == nullptr)
        {
            sensors[i] = sensor;
            return applySensorType(i);
        }
    }
    return false;
}

Sensor *State::getSensor(SensorType type)
{
    for (int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if (sensors[i] && sensors[i]->getType() == type)
            return sensors[i];
    }
    return nullptr;
}


//deprecated
Barometer *State::getBaro() { return *baro; }
GPS *State::getGPS() { return *gps; }
IMU *State::getIMU() { return *imu; }
#pragma endregion

#pragma region Helper Functions

bool State::applySensorType(int i)
{
    bool good = true;
    switch (sensors[i]->getType())
    {
    case BAROMETER_:
        baro = (Barometer **)&sensors[i];
        break;
    case GPS_:
        gps = (GPS **)&sensors[i];
        break;
    case IMU_:
        imu = (IMU **)&sensors[i];
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
    if (stageNumber == 0 && acceleration.z() > 25 && position.z() > 75)
    {
        setRecordMode(FLIGHT);
        stageNumber = 1;
        timeOfLaunch = timeAbsolute;
        timePreviousStage = timeAbsolute;
        recordLogData(INFO, "Launch detected.");
        recordLogData(INFO, "Printing static data.");
        for (int i = 0; i < NUM_MAX_SENSORS; i++)
        {
            if (sensors[i])
            {
                char logData[200];
                snprintf(logData, 200, "%s: %s", sensors[i]->getName(), sensors[i]->getStaticDataString());
                recordLogData(INFO, logData);
            }
        }
    }
    else if (stageNumber == 1 && acceleration.z() < 5)
    {
        stageNumber = 2;
        recordLogData(INFO, "Coasting detected.");
    }
    else if (stageNumber == 2 && velocity.z() <= 0)
    {
        char logData[100];
        snprintf(logData, 100, "Apogee detected at %.2f m.", position.z());
        recordLogData(INFO, logData);
        stageNumber = 3;
        recordLogData(INFO, "Drogue conditions detected.");
    }
    else if (stageNumber == 3 && position.z() < 750 / 3 && millis() - timeSinceLaunch > 1200) // This should be lowered
    {
        stageNumber = 4;
        recordLogData(INFO, "Main parachute conditions detected.");
    }
    else if (stageNumber == 4 && velocity.z() > -0.5 && (*baro)->get_rel_alt_m() < 20)
    {
        stageNumber = 5;
        recordLogData(INFO, "Landing detected. Waiting for 5 seconds to dump data.");
    }
    else if (stageNumber == 5)
    {
        if (landingCounter++ >= 50)
        { // roughly 5 seconds of data after landing
            setRecordMode(GROUND);
            recordLogData(INFO, "Dumped data after landing.");
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
    for(int i = 0; i < NUM_MAX_SENSORS; i++)
    {
        if(sensors[i])
        {
            if(header)
                str[cursor] = sensors[i]->getCsvHeader();
            else
                str[cursor] = sensors[i]->getDataString();
            size += strlen(str[cursor++]);
        }
    }
    dest = new char[size];
    if(header)
        csvHeader = dest;
    else
        dataString = dest;
    //---Fill data String
    int j = 0;
    for (int i = 0; i < numCategories; i++)
    {
        for (int k = 0; str[i][k] != '\0'; j++, k++) {// append all the data strings onto the main string
            dest[j] = str[i][k];
        }
        if (i >= 1 && !header){
            delete[] str[i]; // delete all the heap arrays.
        }
    }
    delete[] str;
    dest[j - 1] = '\0'; // all strings have ',' at end so this gets rid of that and terminates it a character early.

}

#pragma endregion

bool State::transmit()
{
    char data[200];
    snprintf(data, 200, "%f,%f,%i,%i,%i,%c,%i,%s", position(0), position(1), (int)(position(2) * 3.28084), (int)(velocity.magnitude() * 3.2808399), (int)heading_angle, 'H', stageNumber, "12:00:00");
    bool b = radio->send(data, ENCT_TELEMETRY);
    return b;
}