#include "State.h"

State::State(bool useKalmanFilter)
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
    useKF = useKalmanFilter;
    // time pos x y z vel x y z acc x y z
    predictions = new double[10]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // gps x y z barometer z
    measurements = new double[4]{1, 1, 1, 1};
    // imu x y z
    inputs = new double[3]{1, 1, 1};
    kfilter = new akf::KFState();
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

bool State::init()
{
    int good = 0, tryNumSensors = 0;
    if (baro)
    {
        tryNumSensors++;
        if (baro->initialize())
            good++;
        else
            baro = nullptr;
    }
    if (gps)
    {
        tryNumSensors++;
        if (gps->initialize())
            good++;
        else
            gps = nullptr;
    }
    if (imu)
    {
        tryNumSensors++;
        if (imu->initialize())
            good++;
        else
            imu = nullptr;
    }
    if (lisens)
    {
        tryNumSensors++;
        if (lisens->calibrate()) // This should be changed to init...
            good++;
        else
            lisens = nullptr;
    }
    if (rtc)
    {
        tryNumSensors++;
        if (rtc->initialize())
            good++;
        else
            rtc = nullptr;
    }
    setcsvHeader();
    numSensors = good;

    if (useKF)
        initKF(baro, gps, imu);

    return good == tryNumSensors;
}
void State::determineaccelerationMagnitude(imu::Vector<3> accel)
{
    accelerationMagnitude = sqrt((accel.x() * accel.x()) + (accel.y() * accel.y()) + (accel.z() * accel.z()));
}

void State::determineapogee(double zPosition)
{
    if (apogee < zPosition)
        apogee = zPosition;
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
    updateSensors();
    if (useKF)
    {
        // gps x y z barometer z
        measurements[0] = gps->get_displace().x();
        measurements[1] = gps->get_displace().y();
        measurements[2] = gps->get_displace().z();
        measurements[4] = baro->get_rel_alt_m();
        // imu x y z
        inputs[0] = imu->get_acceleration().x();
        inputs[1] = imu->get_acceleration().y();
        inputs[2] = imu->get_acceleration().z() + 9.80665;//add G cuz imu removes it.
        akf::updateFilter(kfilter, timeAbsolute, gps ? 1 : 0, baro ? 1 : 0, imu ? 1 : 0, measurements, inputs, &predictions);
        // time, pos x, y, z, vel x, y, z, acc x, y, z
        //ignore time return value.
        position.x() = predictions[1];
        position.y() = predictions[2];
        position.z() = predictions[3];
        velocity.x() = predictions[4];
        velocity.y() = predictions[5];
        velocity.z() = predictions[6];
        acceleration.x() = predictions[7];
        acceleration.y() = predictions[8];
        acceleration.z() = predictions[9];
    }
    else
    {

        if (gps)
        {
            position = imu::Vector<3>(gps->get_pos().x(), gps->get_pos().y(), gps->get_alt());
            velocity = gps->get_velocity();
        }
        if (baro)
        {
            velocity.z() = (*(double *)baro->get_data() - position.z()) / (millis() - timeAbsolute); // how ugly. get_data() prevents resending commands to the sensor.
            position.z() = *(double *)baro->get_data();
        }
        if (imu)
        {
            acceleration = imu->get_acceleration();
            orientation = imu->get_orientation();
        }
    }
    settimeAbsolute();

    if (stageNumber == 0 && acceleration.z() > 25 && position.z() > 75)
    {
        stageNumber = 1;
        timeLaunch = timeAbsolute;
        timePreviousStage = timeAbsolute;
        digitalWrite(LED_BUILTIN, HIGH); // not sure why this is here because it will never be seen, but leaving it for now
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
    }
    else if (stageNumber == 1 && acceleration.z() < 5)
    {
        stageNumber = 2;
    }
    else if (stageNumber == 2 && velocity.z() < 0)
    {
        stageNumber = 3;
    }
    else if (stageNumber == 3 && position.z() < 750 && millis() - timeSinceLaunch > 120000) // This should be lowered
    {
        stageNumber = 4;
    }
    else if (stageNumber == 4 && velocity.z() > -0.5 && accelerationMagnitude < 5 && *(double *)baro->get_data() < 200)
    {
        stageNumber = 5;
    }
    determineapogee(position.z());
    // backup case to dump data (25 minutes)
    determinetimeSinceLaunch();
    if (stageNumber > 0 && timeSinceLaunch > 1500000 && stageNumber < 5)
    {
        stageNumber = 5;
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
    }
    setdataString();
}

void State::setcsvHeader()
{
    int numCategories = numSensors + 1;
    char csvHeaderStart[] = "Time,Stage,PX,PY,PZ,VX,VY,VZ,AX,AY,AZ,";
    const char **headers = new char *[numCategories];
    headers[0] = csvHeaderStart;
    for (int i = 1; i < numCategories; i++)
        headers[i] = nullptr;
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
        for (int k = 0; headers[i][k] != '\0'; j++, k++) // append all the header strings onto the main string
            csvHeader[j] = headers[i][k];
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
    int used = snprintf(
        csvDataStart, dataStartSize,
        "%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", // trailing comma very important
        timeAbsolute / 1000, STAGES[stageNumber],
        position.x(), position.y(), position.z(),
        velocity.x(), velocity.y(), velocity.z(),
        acceleration.x(), acceleration.y(), acceleration.z());
    // Serial.print(used);//Just curious
    const char **data = new char *[numCategories];
    data[0] = csvDataStart;
    for (int i = 1; i < numCategories; i++)
        data[i] = nullptr;
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
    snprintf(stateString, 500, "%.2f,%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
             timeAbsolute, timeSinceLaunch, STAGES[stageNumber], timeSincePreviousStage,
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

#pragma region Private Helper methods

void State::initKF(bool useBaro, bool useGps, bool useImu)
{
    // These are currently not ever deleted. Since this is only called once, it should not be an issue, but proper practice is to delete these when the state is destroyed.

    double *initial_state = new double[6]{0, 0, 0, 0, 0, 0};
    double *initial_input = new double[3]{0, 0, 0};
    double *initial_covariance = new double[36]{1, 0, 0, 1, 0, 0,
                                                0, 1, 0, 0, 1, 0,
                                                0, 0, 1, 0, 0, 1,
                                                1, 0, 0, 1, 0, 0,
                                                0, 1, 0, 0, 1, 0,
                                                0, 0, 1, 0, 0, 1};
    double *measurement_covariance = new double[16]{1, 0, 0, 0,
                                                    0, 1, 0, 0,
                                                    0, 0, 1, 0,
                                                    0, 0, 0, 1};
    double *process_noise_covariance = new double[36]{.03, 0, 0, 0, 0, 0,
                                                      0, .03, 0, 0, 0, 0,
                                                      0, 0, .03, 0, 0, 0,
                                                      0, 0, 0, .03, 0, 0,
                                                      0, 0, 0, 0, .03, 0,
                                                      0, 0, 0, 0, 0, .03};
    akf::init(kfilter, 6, 3, 4, initial_state, initial_input, initial_covariance, measurement_covariance, process_noise_covariance);
}

#pragma endregion