#include "BTSens.h"

BTGPS::BTGPS(const char *name, ESP32BluetoothRadio *radio) {
    setName(name);
    this->radio = radio;
    addColumn(DOUBLE, &angle, "Flap Angle (deg)");
}

bool BTGPS::init(){
    return initialized = radio->begin();
}

void BTGPS::read() {
    radio->rx();
    if(radio->getReceiveSize() <= 0)
    return;

    char str[513];
    memcpy(str, radio->getReceiveBuffer(), radio->getReceiveSize());
    str[radio->getReceiveSize()] = '\0';
    sscanf(str, "Ang %lf", &angle);
}