/* 
 * Title: Minotaur v2.0 firmware
 * Author: Joao Vieira
 * Date: January 30th, 2023
 * 
 * Description: This is the firmware for the Minotaur board for
 * controlling swing automatic gates. The Minotaur board does not
 * include any ID decoder. That is an extra piece of hardware 
 * that should be connected to the board acting like a button
 * (closed when activated and open when deactivated).
 */

#include <Arduino_FreeRTOS.h>

#include "config.h"
#include "motor_arm.h"
#include "buzzer.h"
#include "current_sensor.h"
#include "lamp.h"
#include "button.h"
#include "persistent_var.h"

/*
 * ===================================================================
 * ====================== SENSORS AND ACTUATORS ======================
 * ===================================================================
 */
Button
  btn_select (BTN_SELECT),
  btn_open   (BTN_OPEN),
  btn_close  (BTN_CLOSE),
  btn_program(BTN_PROGRAM),
  btn_fn     (BTN_FN);

CurrentSensor
  sensor_left (LEFT_ARM_SENSOR),
  sensor_right(RIGHT_ARM_SENSOR);

Lamp lamp(LAMP_PIN);

MotorArm
  arm_left (LEFT_ARM_P_PIN,  LEFT_ARM_N_PIN),
  arm_right(RIGHT_ARM_P_PIN, RIGHT_ARM_N_PIN);

Buzzer buzzer(BUZZER_PIN);

/*
 * ===================================================================
 * ================== VARIABLES STORED IN THE EEPROM =================
 * ===================================================================
 */
PersistentVar
  left_arm_open_time,
  left_arm_close_time,
  right_arm_open_time,
  right_arm_close_time,
  arms_open_delta,
  arms_close_delta;

/*
 * ===================================================================
 * ========================= STATE VARIABLES =========================
 * ===================================================================
 */
bool gate_open = false;
volatile bool 
  blink_lamp = false,
  oc_left_occured = false,
  oc_right_occured = false;

/*
 * ===================================================================
 * ======================= AUXILIARY ROUTINES ========================
 * ===================================================================
 */

/*
 * ===================================================================
 * ============================== TASKS ==============================
 * ===================================================================
 */
void
task_debounce_btns(void *pvParameters)
{
  (void) pvParameters;

  while (true)
  { // Execute task forever
    btn_select.debounce();
    btn_open.debounce();
    btn_close.debounce();
    btn_program.debounce();
    btn_fn.debounce();
  }
}

void
task_blink_lamp(void *pvParameters)
{
  (void) pvParameters;

  while (true)
  { // Execute task forever
    if (blink_lamp) {
      lamp.on();
      vTaskDelay(LAMP_BLINK_T / 2 / portTICK_PERIOD_MS);
      lamp.off();
      vTaskDelay(LAMP_BLINK_T / 2 / portTICK_PERIOD_MS);
    }
  }
}

void
task_monitor_sensors(void *pvParameters)
{
  (void) pvParameters;

  while (true)
  { // Execute task forever
    // Left arm overload
    if (sensor_left.getValue() > MAX_CURRENT_VALUE_OVER_OFFSET) {
      oc_left_occured = true;
      arm_left.off();
    }

    // Right arm overload
    if (sensor_right.getValue() > MAX_CURRENT_VALUE_OVER_OFFSET) {
      oc_right_occured = true;
      arm_right.off();
    }
  }
}

void
task_main(void *pvParameters)
{
  (void) pvParameters;

  while (true)
  { // Execute task forever
    blink_lamp = (btn_select.isPressed()) ? true : false;
  }
}

/*
 * ===================================================================
 * ==================== ARDUINO BULT-IN FUNCTIONS ====================
 * ===================================================================
 */
void
setup()
{
  // Set sensors offset
  sensor_left.setOffset();
  sensor_right.setOffset();
  
  // Create tasks
  xTaskCreate(task_debounce_btns,   "Debounce buttons", 128, NULL, 3, NULL);
  xTaskCreate(task_blink_lamp,      "Blink lamp",       128, NULL, 3, NULL);
  xTaskCreate(task_monitor_sensors, "Monitor sensors",  128, NULL, 3, NULL);
  xTaskCreate(task_main,            "Main task",        128, NULL, 3, NULL);
}

void
loop()
{
  // Nothing to do here
}
