#ifndef _ARDUINO_FFCONF_CUSTOM_H
#define _ARDUINO_FFCONF_CUSTOM_H

#if defined(FF_DEFINED) && !defined(_FATFS)
  #define _FATFS FF_DEFINED
#endif

#if _FATFS == 80286
  #define _USE_WRITE 1
  #define _USE_IOCTL 1
  #include "ffconf_default_80286.h"
  #undef FF_USE_MKFS
  #define FF_USE_MKFS 1
#elif _FATFS == 68300
  #include "ffconf_default_68300.h"
  #undef _USE_MKFS
  #define _USE_MKFS 1
#else
  #include "ffconf_default_32020.h"
  #undef FF_USE_MKFS
  #define FF_USE_MKFS 1
#endif

#endif /* _ARDUINO_FFCONF_CUSTOM_H */
