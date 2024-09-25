#include <Adafruit_DPS310.h>

#include <Sensors/Baro/Barometer.h>

namespace mmfs
{
    class DPS310 : public Barometer
    {
    public:
        DPS310(const char *name = "DPS310");
        virtual bool init() override;
        virtual void read() override;

    private:
        Adafruit_DPS310 dps;
    };
}; // namespace mmfs
