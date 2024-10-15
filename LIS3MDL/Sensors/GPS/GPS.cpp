#include "GPS.h"

namespace mmfs
{

    GPS::GPS()
    {
        staticData = new char[60 + MAX_DIGITS_LAT_LON * 2 + MAX_DIGITS_FLOAT * 1];               // 60 chars for the string, 15 chars for the 2 floats, 12 chars for the float
        data = new char[MAX_DIGITS_LAT_LON * 2 + MAX_DIGITS_FLOAT * 4 + MAX_DIGITS_INT * 1 + 8]; // 15 chars for the 2 floats, 12 chars for the 4 floats, 10 chars for the time of day string, 10 for the int, 8 for the comma
    }

    /*
    returns the lat and long of the rocket and the altitude
    */
    Vector<3> GPS::getPos() const
    {
        return position;
    }

    double GPS::getHeading() const
    {
        return heading;
    }

    /*
    return the displacement from the origin in meters
    */
    Vector<3> GPS::getDisplacement() const
    {
        return displacement;
    }

    /*
    returns vector of orginal position in lat(deg), lat(deg), and alti(m)
    */
    Vector<3> GPS::getOrigin() const
    {
        return origin;
    }

    bool GPS::getHasFirstFix() const
    {
        return hasFirstFix;
    }

    /*
    return the number of satellites to indicate quality of data
    */
    int GPS::getFixQual() const
    {
        return fixQual;
    }

    const char *GPS::getCsvHeader() const
    {                                                                                                // incl G- for GPS
        return "G-Lat (deg),G-Lon (deg),G-Alt (m),G-DispX (m),G-DispY (m),G-DispZ (m),G-# of Sats,"; // trailing comma
    }

    const char *GPS::getDataString() const
    {
        sprintf(data, "%.10f,%.10f,%.2f,%.2f,%.2f,%.2f,%d,", position.x(), position.y(), position.z(), displacement.x(), displacement.y(), displacement.z(), fixQual); // trailing comma
        return data;
    }

    const char *GPS::getStaticDataString() const
    {
        sprintf(staticData, "Original Latitude (m): %.10f\nOriginal Longitude (m): %.10f\nOriginal Altitude (m): %.2f\n", origin.x(), origin.y(), origin.z());
        return staticData;
    }

    void GPS::update()
    {
        read();

        if (!hasFirstFix && fixQual >= 3)
        {
            logger.recordLogData(INFO_, "GPS has first fix.");

            bb.aonoff(BUZZER_PIN, 1000);
            hasFirstFix = true;
            origin.x() = position.x();
            origin.y() = position.y();
            origin.z() = position.z();

            calcInitialValuesForDistance();
        }
        if (hasFirstFix)
        {
            if(biasCorrectionMode){
                originBuffer.push(position);
                Vector<3> sum = Vector<3>(0, 0, 0);
                int valsToCount = std::min(originBuffer.getCount(), CIRC_BUFFER_LENGTH - CIRC_BUFFER_IGNORE);
                for (int i = 0; i < valsToCount; i++)
                {
                    sum += originBuffer[i];
                }
                origin = sum / valsToCount / 1.0;
            }
            calcDistance();
            displacement.z() = (position.z() - origin.z());
        }
    }

    bool GPS::begin(bool useBiasCorrection)
    {
        biasCorrectionMode = useBiasCorrection;
        position = Vector<3>(0, 0, 0);
        displacement = Vector<3>(0, 0, 0);
        origin = Vector<3>(0, 0, 0);
        originBuffer.clear();
        fixQual = 0;
        hasFirstFix = false;
        heading = 0;
        return init();
    }

    // Taken from this article and repo. As I understand it, it's an accurate approximation of the Vincenty formulae to find the distance between two points on the earth
    //  https://github.com/mapbox/cheap-ruler/blob/main/index.js#L475
    //  https://blog.mapbox.com/fast-geodesic-approximations-with-cheap-ruler-106f229ad016
    void GPS::calcInitialValuesForDistance()
    {
        constexpr auto EARTH_RAD = 6378.137e3; // meters
        constexpr auto RAD = 3.14159265358979323846 / 180.0;

        constexpr auto EARTH_FLAT = 1.0 / 298.257223563; // flattening of the earth. IDK what this means

        constexpr auto ECC_SQRD = EARTH_FLAT * (2.0 - EARTH_FLAT); // eccentricity squared. IDK what this means

        constexpr auto m = RAD * EARTH_RAD;
        const auto coslat = cos(position.x() * RAD);
        const auto w2 = 1.0 / (1.0 - ECC_SQRD * (1.0 - coslat * coslat)); // IDK what this means
        const auto w = sqrt(w2);                                          // IDK what this means

        ky = m * w * coslat;                // IDK what this means
        kx = m * w * w2 * (1.0 - ECC_SQRD); // IDK what this means
    }

    void GPS::calcDistance()
    {
        double dy = wrapLongitude(position.y() - origin.y()) * ky;
        double dx = (position.x() - origin.x()) * kx;
        displacement.x() = dx;
        displacement.y() = dy;
    }

    double GPS::wrapLongitude(double val)
    {
        while (val > 180)
            val -= 360;
        while (val < -180)
            val += 360;
        return val;
    }
}