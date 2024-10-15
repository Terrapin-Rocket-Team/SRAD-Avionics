#ifndef SENSOR_H
#define SENSOR_H

#include "../Constants.h"
#include "../Utils/CircBuffer.h"
#include "../BlinkBuzz/BlinkBuzz.h"
#include "../RecordData/Logger.h"
#include <algorithm>

namespace mmfs
{
    enum SensorType // These have trailing underscores to avoid conflicts with the same names in other libraries *cough* IMU *cough*
    {
        BAROMETER_,
        GPS_,
        IMU_,
        LIGHT_SENSOR_,
        RADIO_,
        RTC_,
        ENCODER_
    };
    class Sensor
    {
    public:
        virtual ~Sensor()
        {
            delete[] data;
            delete[] staticData;
            delete[] name;
        };

        // ------------------------------- SENSOR TYPE IMPLEMMENTATION ---------------------------------------------

        // gives the names of the columns which transient data will be stored under, in a comma separated string
        virtual const char *getCsvHeader() const = 0;
        // gives the data values of the variables said to be stored by the header, in the same order, in a comma separated string
        virtual const char *getDataString() const = 0;
        // gives a string which includes all the sensor's static/initialization data. This will be in the format of dataName:dataValue, with each pair separated by a newline (\n)
        virtual const char *getStaticDataString() const = 0;
        // Updates the sensor's fields by querying the sensor for new data (calls read() internally)
        virtual void update() = 0;
        // Initializes the sensor and sets up any necessary parameters
        virtual bool begin(bool useBiasCorrection = true) = 0;

        virtual SensorType getType() const = 0;        // Returns the type of the sensor
        virtual const char *getTypeString() const = 0; // Returns the type of the sensor as a string

        // ------------------------------- BASE SENSOR CLASS IMPLEMENTATION ----------------------------------------

        virtual const char *getName() const { return name; } // Returns the name of the sensor
        virtual void setName(const char *n)                  // Sets the name of the sensor
        {
            delete[] name;
            int len = strlen(n);
            name = new char[len + 1];
            snprintf(name, len + 1, "%s", n);
        }

        virtual bool isInitialized() const { return initialized; } // Returns whether the sensor has been initialized or not

        virtual explicit operator bool() const { return initialized; } // Returns whether the sensor has been initialized or not

        virtual void markLiftoff() { biasCorrectionMode = false; } // Sets the bias correction mode to false, storing origin positions

        virtual void setBiasCorrectionMode(bool mode) { biasCorrectionMode = mode; } // Sets the bias correction mode

        // ----------------------------------------------------------------------------------------------------------

    protected:
        // --------------------------------- HARDWARE IMPLEMENTATION -----------------------------------------------

        // Sets up the sensor and stores any critical parameters. Needs to reset the sensor if it is already initialized. Called by begin()
        virtual bool init() = 0;
        // Physically reads the outputs from the sensor hardware. Called by update()
        virtual void read() = 0;

        // ----------------------------------------------------------------------------------------------------------
        bool initialized = false;
        bool biasCorrectionMode = true;
        char *name = nullptr;       // Name of the sensor
        char *staticData = nullptr; // Initial "power on" data
        char *data = nullptr;       // Transient data

        const int CIRC_BUFFER_LENGTH = UPDATE_RATE * SENSOR_BIAS_CORRECTION_DATA_LENGTH; // number of entries to give SBCDL length average
        const int CIRC_BUFFER_IGNORE = UPDATE_RATE * SENSOR_BIAS_CORRECTION_DATA_IGNORE; // number of entries to ignore for SBCD
    };
}; // namespace mmfs

#endif // SENSOR_H