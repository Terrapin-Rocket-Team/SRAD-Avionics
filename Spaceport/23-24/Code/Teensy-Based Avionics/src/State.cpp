#include "State.h"

State::State(){
    stage = "Pre Launch";
    recordDataStage = "PreFlight";
    timeAbsolute = millis();
    timePreviousStage = 0;
    position.x() = 0; position.y() = 0; position.z() = 0;
    velocity.x() = 0; velocity.y() = 0; velocity.z() = 0;
    acceleration.x() = 0; acceleration.y() = 0; acceleration.z() = 0;
    apogee = position.z();
    stageNumber = 0;
    lastGPSUpdate = millis();

    barometerFlag = false; gpsFlag = false; imuFlag = false; lightSensorFlag = false;
}

void State::addBarometer(Barometer* Barometer){
    stateBarometer = Barometer;
    barometerFlag = true;
}

void State::addGPS(GPS* gps){
    stateGPS = gps;
    gpsFlag = true;
}

void State::addIMU(IMU* imu){
    stateIMU = imu;
    imuFlag = true;
}

void State::addLightSensor(LightSensor* LightSensor){
    stateLightSensor = LightSensor;
    lightSensorFlag = true;
}

void State::addRTC(RTC* rtc){
    stateRTC = rtc;
    rtcFlag = true;
}

void State::settimeAbsolute(){
    timeAbsolute = millis();
}

void State::determineaccelerationMagnitude(imu::Vector<3> accel){
    accelerationMagnitude = sqrt((accel.x()*accel.x()) + (accel.y()*accel.y()) + (accel.z()*accel.z()));
}

void State::determineapogee(double zPosition){
    if(apogee < zPosition){
        apogee = zPosition;
    }
}

void State::determinetimeSincePreviousStage(){
    timeSincePreviousStage = timeAbsolute - timePreviousStage;
}

void State::determinetimeSinceLaunch(){
    timeSinceLaunch = timeAbsolute - timeLaunch;
}

void State::updateSensors() {
    if (barometerFlag) {
        stateBarometer->get_data();
    }
    if (imuFlag) {
        stateIMU->get_data();
        stateIMU->get_orientation();
        stateIMU->get_orientation_euler();
    }
    if (gpsFlag && millis() - lastGPSUpdate > 1500){
        stateGPS->read_gps();
    }
}

void State::updateState() {
    if(gpsFlag) {
        position = imu::Vector<3>(stateGPS->get_pos().x(), stateGPS->get_pos().y(), stateGPS->get_alt());
        velocity = stateGPS->get_velocity();
    }
    if (barometerFlag) {
        velocity.z() = (stateBarometer->get_rel_alt_ft() - position.z()) / (millis() - timeAbsolute);
        position.z() = stateBarometer->get_rel_alt_ft();
    }
    if (imuFlag)
    {
        acceleration = stateIMU->get_acceleration();
        orientation = stateIMU->get_orientation();
    }
    settimeAbsolute();

    if (stageNumber == 0 && acceleration.z() > 20 && position.z() > 75) {
        stageNumber = 1;
        timeLaunch = timeAbsolute;
        timePreviousStage = timeAbsolute;
        stage = "Ascent";
        recordDataStage = "Flight";
    }
    else if (stageNumber == 1 && acceleration.z() < 5) {
        stageNumber = 2;
        stage = "Coasting";
    }
    else if (stageNumber == 2 && velocity.z() < 0){
        stageNumber = 3;
        stage = "Drogue Descent";
    }
    else if (stageNumber == 3 && position.z() < 750) {
        stageNumber = 4;
        stage = "Main Descent";
    }
    else if (stageNumber == 4 && velocity.z() > -0.5 && accelerationMagnitude < 5 && stateBarometer->get_rel_alt_ft() < 200) {
        stageNumber = 5;
        stage = "Landed";
        recordDataStage = "PostFlight";
    }

    // backup case to dump data (25 minutes)
    determinetimeSinceLaunch();
    if (timeSinceLaunch > 1500) {
        stageNumber = 5;
        stage = "Landed";
    }

    
    

}

void State::setcsvHeader(){
    csvHeader = "";
    csvHeader += "Time"; csvHeader += ",";
    csvHeader += "Stage"; csvHeader += ",";
    csvHeader += "PosX"; csvHeader += ","; csvHeader += "PosY"; csvHeader += ","; csvHeader += "PosZ"; csvHeader += ",";
    csvHeader += "VeloX"; csvHeader += ","; csvHeader += "VeloY"; csvHeader += ","; csvHeader += "VeloZ"; csvHeader += ",";
    csvHeader += "AccelX"; csvHeader += ","; csvHeader += "AccelY"; csvHeader += ","; csvHeader += "AccelZ"; csvHeader += ",";
    if(barometerFlag){
        csvHeader += stateBarometer -> getcsvHeader();
    }
    if(gpsFlag){
        csvHeader += stateGPS -> getcsvHeader();
    }
    if(imuFlag){
        csvHeader += stateIMU -> getcsvHeader();
    }
    if(lightSensorFlag){
        csvHeader += stateLightSensor -> getcsvHeader();
    }
    if (rtcFlag){
        csvHeader += stateRTC -> getcsvHeader();
    }
}

void State::setdataString(){
    dataString = "";
    dataString += String(timeAbsolute); dataString += ",";
    dataString += stage; dataString += ",";
    dataString += String(position.x()); dataString += ",";
    dataString += String(position.y()); dataString += ",";
    dataString += String(position.z()); dataString += ",";
    dataString += String(velocity.x()); dataString += ",";
    dataString += String(velocity.x()); dataString += ",";
    dataString += String(velocity.x()); dataString += ",";
    dataString += String(acceleration.x()); dataString += ",";
    dataString += String(acceleration.y()); dataString += ",";
    dataString += String(acceleration.z()); dataString += ",";
    if(barometerFlag){
        dataString += stateBarometer -> getdataString();
    }
    if(gpsFlag){
        dataString += stateGPS -> getdataString();
    }
    if(imuFlag){
        dataString += stateIMU -> getdataString();
    }
    if(lightSensorFlag){
        dataString += stateLightSensor -> getdataString();
    }
    if (rtcFlag){
        dataString += stateRTC -> getdataString();
    }
}

String State::getdataString(){
    return dataString;
}

String State::getrecordDataState(){
    return recordDataStage;
}