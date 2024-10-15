#include "MAX_M10S.h"
#include "../../BlinkBuzz/BlinkBuzz.h"

namespace mmfs
{

    MAX_M10S::MAX_M10S(const char *name) : m10s()
    {
        setName(name);
    }

    bool MAX_M10S::init()
    {

        // myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

        int count = 0;
        while (m10s.begin() == false && count < 3) // Connect to the u-blox module using Wire port
        {
            // Serial.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
            // TODO: RECORD FAILURE IN RECORDDATA?
            delay(250);
            count++;
        }
        if (!m10s.begin())
            return initialized = false;

        m10s.setI2COutput(COM_TYPE_UBX);            // Set the I2C port to output UBX only (turn off NMEA noise)
        m10s.setNavigationFrequency(10);            // Set the update rate to 10Hz
        m10s.setDynamicModel(DYN_MODEL_AIRBORNE4g); // Set the dynamic model to airborne with 4g acceleration
        m10s.setAutoPVT(true);                      // Enable automatic PVT reports
        m10s.saveConfiguration();                   // Save the current settings to flash and BBR
        return initialized = true;
    }

    /*
    used to update all instance variables
    */
    void MAX_M10S::read()
    {
        if (!initialized || !m10s.getPVT() || m10s.getInvalidLlh())
            return; // See if new data is available

        position.x() = m10s.getLatitude() / 10000000.0;
        position.y() = m10s.getLongitude() / 10000000.0;
        position.z() = m10s.getAltitude() / 1000.0;
        heading = m10s.getHeading();
        fixQual = m10s.getSIV();
    }
}