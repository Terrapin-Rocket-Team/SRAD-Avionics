#include "ADXL375.h"

using namespace mmfs;

bool Adafruit_ADXL375Wrap::init(){ //used to initialize
    this->initialized = false;
    Wire2.begin();
    bool status = this->accel.begin(0x1D); //begin accelerometer
     accel.setTrimOffsets(-3, 
                       -3, 
                       -1);  // Z should be '20' at 1g (49mg per bit)
    if(status){ 
        this->initialized = true;
    } else {
        getLogger().recordLogData(WARNING_, "failed to initalize");
    }
    //setup complimentary filters and automatically scale 
    sensors_event_t event;
    this->accel.getEvent(&event);
    accel.setDataRate(ADXL343_DATARATE_100_HZ);
    this->measuredAcc = mmfs::Vector<3>((double) event.acceleration.x, (double) event.acceleration.y, 
        (double) event.acceleration.z); //using event to update readings and updating at that 
                                // reference
    //copied over from the BMI088 cpp implementation
    // quaternionBasedComplimentaryFilterSetup();
    // setAccelBestFilteringAtStatic(.5);
    return this->initialized; //return initialized true if eveyrhting works 
}

void Adafruit_ADXL375Wrap::read(){ //implementing read function 
    sensors_event_t event; // creating an event object to get the
                            // accelerations in x, y, and z directions
    this->accel.getEvent(&event);
                        
     this->measuredAcc = mmfs::Vector<3>((double) event.acceleration.x, (double) event.acceleration.y, 
        (double) event.acceleration.z); //using event to update readings and updating at that 
                                // reference

}



