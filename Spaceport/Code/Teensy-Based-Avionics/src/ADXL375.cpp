#include "ADXL375.h"

using namespace mmfs;

bool Adafruit_ADXL375Wrap::init(){ //used to initialize
    this->initialized = false;
    bool status = this->accel.begin(); //begin accelerometer
    if(status){ 
        this->initialized = true;
    } else {
        getLogger().recordLogData(WARNING_, "failed to initalize");
    }
    //setup complimentary filters and automatically scale 
    sensors_event_t event;
    this->accel.getEvent(&event);
    this->measuredAcc = mmfs::Vector<3>(event.acceleration.x, event.acceleration.y, 
        event.acceleration.z); //using event to update readings and updating at that 
                                // reference
    //copied over from the BMI088 cpp implementation
    quaternionBasedComplimentaryFilterSetup();
    setAccelBestFilteringAtStatic(.5);
    return this->initialized; //return initialized true if eveyrhting works 
}

void Adafruit_ADXL375Wrap::read(){ //implementing read function 
    sensors_event_t event; // creating an event object to get the
                            // accelerations in x, y, and z directions
                        
     this->measuredAcc = mmfs::Vector<3>(event.acceleration.x, event.acceleration.y, 
        event.acceleration.z); //using event to update readings and updating at that 
                                // reference
        quaternionBasedComplimentaryFilter(UPDATE_INTERVAL / 1000.0);

}



