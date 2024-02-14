/*
 * State.cpp - Holds all the information about the current position/state of the rocket in 3d space.
 *
 * Fetches data from various sensors to update its fields and stores a string with all values in a csv format that can be fetched for outputting.
 * Includes checks to ensure that various sensors are enabled before attempting to use them
 *
 * TODO: Integrate Kalman filter (Issue #40 - https://github.com/Terrapin-Rocket-Team/SRAD_Avionics/issues/40)
 *
 * Created By: Varun Unnithan
 * Last Updated By: Drew Brandt
 */

#include "State.h"

State::State()
{
    stage = "Pre Launch";
    recordDataStage = "PreFlight";
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
    apogee = position.z();
    stageNumber = 0;
    lastGPSUpdate = millis();

    barometerFlag = gpsFlag = imuFlag = lightSensorFlag = rtcFlag = false;
}

void State::addBarometer(Barometer *Barometer)
{
    stateBarometer = Barometer;
    barometerFlag = true;
}

void State::addGPS(GPS *gps)
{
    stateGPS = gps;
    gpsFlag = true;
}

void State::addIMU(IMU *imu)
{
    stateIMU = imu;
    imuFlag = true;
}

void State::addLightSensor(LightSensor *LightSensor)
{
    stateLightSensor = LightSensor;
    lightSensorFlag = true;
}

void State::addRTC(RTC *rtc)
{
    stateRTC = rtc;
    rtcFlag = true;
}

void State::settimeAbsolute()
{
    timeAbsolute = millis();
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

void State::updateSensors()
{
    if (barometerFlag)
    {
        stateBarometer->get_data();
    }
    if (imuFlag)
    {
        stateIMU->get_data();
        stateIMU->get_orientation();
        stateIMU->get_orientation_euler();
    }
    if (gpsFlag && millis() - lastGPSUpdate > 1500)
    {
        stateGPS->read_gps();
    }
}

void State::updateState()
{
    if (gpsFlag)
    {
        position = imu::Vector<3>(stateGPS->get_pos().x(), stateGPS->get_pos().y(), stateGPS->get_alt());
        velocity = stateGPS->get_velocity();
    }
    if (barometerFlag)
    {
        velocity.z() = (stateBarometer->get_rel_alt_ft() - position.z()) / (millis() - timeAbsolute);
        position.z() = stateBarometer->get_rel_alt_ft();
    }
    if (imuFlag)
    {
        acceleration = stateIMU->get_acceleration();
        orientation = stateIMU->get_orientation();
    }
    settimeAbsolute();

    if (stageNumber == 0 && acceleration.z() > 25 && position.z() > 75)
    {
        stageNumber = 1;
        timeLaunch = timeAbsolute;
        timePreviousStage = timeAbsolute;
        stage = "Ascent";
        recordDataStage = "Flight";
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
    }
    else if (stageNumber == 1 && acceleration.z() < 5)
    {
        stageNumber = 2;
        stage = "Coasting";
    }
    else if (stageNumber == 2 && velocity.z() < 0)
    {
        stageNumber = 3;
        stage = "Drogue Descent";
    }
    else if (stageNumber == 3 && position.z() < 750 && millis() - timeSinceLaunch > 120000)
    {
        stageNumber = 4;
        stage = "Main Descent";
    }
    else if (stageNumber == 4 && velocity.z() > -0.5 && accelerationMagnitude < 5 && stateBarometer->get_rel_alt_ft() < 200)
    {
        stageNumber = 5;
        stage = "Landed";
        recordDataStage = "PostFlight";
    }

    // backup case to dump data (25 minutes)
    determinetimeSinceLaunch();
    if (stageNumber > 0 && timeSinceLaunch > 1500000 && stageNumber < 5)
    {
        stageNumber = 5;
        stage = "Landed";
        recordDataStage = "PostFlight";
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
    }
}

//using vector as its much less prone to error and much easier to update than manually typing out "csvHeader += 'string'; csvHeader += ','..."
void State::setcsvHeader()
{

    std::vector<String> colTitles{
        "Time",
        "Stage",
        "PosX", "PosY", "PosZ",
        "VeloX", "VeloY", "VeloZ",
        "AccelX", "AccelY", "AccelZ"};

    if (barometerFlag)
        colTitles.insert(colTitles.end(), stateBarometer->getcsvHeader().begin(), stateBarometer->getcsvHeader().end());
    if (gpsFlag)
        colTitles.insert(colTitles.end(), stateGPS->getcsvHeader().begin(), stateGPS->getcsvHeader().end());
    if (imuFlag)
        colTitles.insert(colTitles.end(), stateIMU->getcsvHeader().begin(), stateIMU->getcsvHeader().end());
    if (lightSensorFlag)
        colTitles.insert(colTitles.end(), stateLightSensor->getcsvHeader().begin(), stateLightSensor->getcsvHeader().end());
    if (rtcFlag)
        colTitles.insert(colTitles.end(), stateRTC->getcsvHeader().begin(), stateRTC->getcsvHeader().end());
    csvHeader = "";
    for (String s : colTitles)
        csvHeader += s + ",";
    csvHeader.remove(csvHeader.length() - 1);//remove trailing comma
}

void State::setdataString()//not modifying this one to use vectors in part because it seems like a lot of work and in part because I'm not sure it would actually make anythign easier to maintain. - Drew
{
    dataString = "";
    dataString += String(timeAbsolute / 1000);
    dataString += ",";
    dataString += stage;
    dataString += ",";
    dataString += String(position.x());
    dataString += ",";
    dataString += String(position.y());
    dataString += ",";
    dataString += String(position.z());
    dataString += ",";
    dataString += String(velocity.x());
    dataString += ",";
    dataString += String(velocity.y());
    dataString += ",";
    dataString += String(velocity.z());
    dataString += ",";
    dataString += String(acceleration.x());
    dataString += ",";
    dataString += String(acceleration.y());
    dataString += ",";
    dataString += String(acceleration.z());
    dataString += ",";
    if (barometerFlag)
    {
        dataString += stateBarometer->getdataString();
    }
    if (gpsFlag)
    {
        dataString += stateGPS->getdataString();
    }
    if (imuFlag)
    {
        dataString += stateIMU->getdataString();
    }
    if (lightSensorFlag)
    {
        dataString += stateLightSensor->getdataString();
    }
    if (rtcFlag)
    {
        dataString += stateRTC->getdataString();
    }
    dataString.remove(dataString.length() - 1);//remove trailing comma
}

String State::getdataString()
{
    return dataString;
}

String State::getrecordDataState()
{
    return recordDataStage;
}