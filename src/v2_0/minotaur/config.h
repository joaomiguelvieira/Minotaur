#ifndef __CONFIG_H__
#define __CONFIG_H__

// Digital PINs
#define BUZZER_PIN       2                // OUTPUT activate buzzer
#define BTN_PROGRAM      3                // INPUT btn program selected
#define BTN_SELECT       4                // INPUT btn select selected
#define BTN_OPEN         5                // INPUT btn open selected
#define BTN_FN           6                // INPUT btn function selected
#define BTN_CLOSE        7                // INPUT btn close selected
#define LEFT_ARM_P_PIN   8                // OUTPUT switch left arm positive to +12 V
#define LEFT_ARM_N_PIN   9                // OUTPUT switch left arm negative to +12 V
#define RIGHT_ARM_P_PIN  10               // OUTPUT switch right arm positive to +12 V
#define RIGHT_ARM_N_PIN  11               // OUTPUT switch right arm negative to +12 V
#define LAMP_PIN         12               // OUTPIT activate lamp

// Analog PINs
#define RIGHT_ARM_SENSOR A1               // INPUT right arm current sensor
#define LEFT_ARM_SENSOR  A2               // INPUT left arm current sensor

// General configurations
#define BUZZER_BIP_T 1000                 // Buzzer bip period [ms]
#define LAMP_BLINK_T 1000                 // Lamp blink period [ms]
#define OPEN_CLOSE_DELAY LAMP_BLINK_T * 2 // Time between lamp starts blinking and gate starts moving
#define NUMBER_BIPS_BEFORE_PROG 3         // Number of bips before entering programming mode
#define NUMBER_BIPS_AFTER_PROG 2          // Number of bips after exiting programming mode
#define PROG_BTN_DELAY 500                // Delay after pressing button in programming mode [ms]

#define MAX_CURRENT_VALUE_OVER_OFFSET 100 // Check current_sensor.h for explanation

#endif //__CONFIG_H__
