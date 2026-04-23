#ifndef PIN_DEFS_H
#define PIN_DEFS_H

// User vars

#define PWR_CH1_DEFAULT LOW
#define PWR_CH2_DEFAULT LOW
#define PWR_CH3_DEFAULT LOW
#define PWR_CH4_DEFAULT LOW
#define PWR_CH5_DEFAULT LOW
#define PWR_CH6_DEFAULT LOW

// system definitions

#define PWR_CH1 1 // PA0
#define PWR_CH2 2 // PA1
#define PWR_CH3 3 // PA2
#define PWR_CH4 4 // PA3
#define PWR_CH5 5 // PA4
#define PWR_CH6 6 // PA5

#define VOLT0 7 // PA7
#define VOLT1 6 // PA6
#define VOLT2 8 // PA8

// TODO: tune to be exact
#define VOLT0_MAX 8.4  // Bat voltage
#define VOLT1_MAX 5    // 5V rail
#define VOLT2_MAX 14.5 // Charge voltage

#define BAT_VOLT VOLT0
#define RAIL_VOLT VOLT1
#define CHARGE_VOLT VOLT2

#define BAT_VOLT_MAX VOLT0_MAX
#define RAIL_VOLT_MAX VOLT1_MAX
#define CHARGE_VOLT_MAX VOLT2_MAX

#define STAT 15 // PC15

#endif