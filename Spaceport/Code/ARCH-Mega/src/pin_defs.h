#ifndef PIN_DEFS_H
#define PIN_DEFS_H

// User vars

#define PWR_CH1_DEFAULT HIGH
#define PWR_CH2_DEFAULT HIGH
#define PWR_CH3_DEFAULT HIGH
#define PWR_CH4_DEFAULT HIGH
#define PWR_CH5_DEFAULT HIGH
#define PWR_CH6_DEFAULT HIGH

// system definitions

#define PWR_CH1 pinNametoDigitalPin(PA_0) // PA0
#define PWR_CH2 pinNametoDigitalPin(PA_1) // PA1
#define PWR_CH3 pinNametoDigitalPin(PA_2) // PA2
#define PWR_CH4 pinNametoDigitalPin(PA_3) // PA3
#define PWR_CH5 pinNametoDigitalPin(PA_4) // PA4
#define PWR_CH6 pinNametoDigitalPin(PA_5) // PA5

#define VOLT0 7 // PA7
#define VOLT1 6 // PA6
#define VOLT2 8 // PA8

// TODO: tune to be exact
#define VOLT0_MAX 8.643  // Bat voltage
#define VOLT1_MAX 5.149  // 5V rail
#define VOLT2_MAX 14.528 // Charge voltage

#define BAT_VOLT VOLT0
#define RAIL_VOLT VOLT1
#define CHARGE_VOLT VOLT2

#define BAT_VOLT_MAX VOLT0_MAX
#define RAIL_VOLT_MAX VOLT1_MAX
#define CHARGE_VOLT_MAX VOLT2_MAX

#define STAT pinNametoDigitalPin(PC_15) // PC15

#endif