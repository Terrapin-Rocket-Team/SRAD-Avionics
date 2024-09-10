#ifndef RADIO_MESSAGE_H
#define RADIO_MESSAGE_H

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(_WIN32) || defined(_WIN64) || defined(__unix__) || defined(__APPLE__) // Windows, Linux, or OSX
#include <cstdint>
#include <string>
#include <cstring>
#endif

#include "Message.h"

#include "Data.h"
// pull together data subclasses
#include "APRS/APRSData.h"
#include "Video/VideoData.h"
// Pull together APRSData subclasses
#include "APRS/APRSTelem.h"
#include "APRS/APRSCmd.h"
#include "APRS/APRSText.h"

#endif