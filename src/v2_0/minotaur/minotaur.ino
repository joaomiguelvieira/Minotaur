/* 
 * Title: Minotaur v2.0 firmware
 * Author: Joao Vieira
 * Date: January 31st, 2023
 * 
 * Description: This is the firmware for the Minotaur board for controlling swing
 * automatic gates. The Minotaur board does not include any ID decoder. That is
 * an extra piece of hardware that should be connected to the board acting like a
 * button (closed when activated and open when deactivated).
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
 * =============================================================================
 * =========================== SENSORS AND ACTUATORS ===========================
 * =============================================================================
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
 * =============================================================================
 * ======================= VARIABLES STORED IN THE EEPROM ======================
 * =============================================================================
 */
PersistentVar
  left_arm_open_time,
  left_arm_close_time,
  right_arm_open_time,
  right_arm_close_time,
  arms_open_delta,
  arms_close_delta;

/*
 * =============================================================================
 * ============================== STATE VARIABLES ==============================
 * =============================================================================
 */
bool gate_open = false;
volatile bool 
  blink_lamp = false,
  oc_left_occured = false,
  oc_right_occured = false;

/*
 * =============================================================================
 * ============================ AUXILIARY ROUTINES =============================
 * =============================================================================
 */
void
bip_buzzer(uint8_t n)
{
  for (uint8_t i = 0; i < n; i++)
  {
    buzzer.on();
    vTaskDelay(BUZZER_BIP_T / 2 / portTICK_PERIOD_MS);
    buzzer.off();
    vTaskDelay(BUZZER_BIP_T / 2 / portTICK_PERIOD_MS);
  }
}

void  
open_close_cycle()
{
  // Start blinking lamp
  blink_lamp = true;
  vTaskDelay(OPEN_CLOSE_DELAY / portTICK_PERIOD_MS);

  // Start first arm
  if (gate_open) arm_right.close(); else arm_left.open();
  vTaskDelay(
    (
      gate_open ?
      arms_close_delta.get() :
      arms_open_delta.get()
    ) / portTICK_PERIOD_MS);

  // Start second arm
  if (gate_open) arm_left.close(); else arm_right.open();
  vTaskDelay(
    (
      gate_open ?
      (right_arm_close_time.get() - arms_close_delta.get()) :
      (left_arm_open_time.get() - arms_open_delta.get())
    ) / portTICK_PERIOD_MS);
  
  // Stop first arm
  if (gate_open) arm_right.off(); else arm_left.off();
  vTaskDelay(
    (
      gate_open ?
      (left_arm_close_time.get() - right_arm_close_time.get() + arms_close_delta.get()) :
      (right_arm_open_time.get() - left_arm_open_time.get() + arms_open_delta.get())
    ) / portTICK_PERIOD_MS);

  // Stop second arm
  if (gate_open) arm_left.off(); else arm_right.off();

  // Stop blinking lamp
  blink_lamp = false;

  // Switch gate state
  gate_open = !gate_open;

  // Clear oc variables
  oc_left_occured = oc_right_occured = false;
}

void
close_while_pressed()
{
  // Close right arm
  if (btn_fn.isPressed())
    arm_right.close();
  // Close left arm
  else
    arm_left.close();
  
  // Blink lamp while button pressed
  while(btn_close.isPressed())
    blink_lamp = true;

  // Switch off both arms
  arm_left.off();
  arm_right.off();

  // Stop blinking lamp
  blink_lamp = false;

  // Clear oc variables
  oc_left_occured = oc_right_occured = false;
}

void
open_while_pressed()
{
  // Open right arm
  if (btn_fn.isPressed())
    arm_right.open();
  // Open left arm
  else
    arm_left.open();
  
  // Blink lamp while button pressed
  while(btn_close.isPressed())
    blink_lamp = true;

  // Switch off both arms
  arm_left.off();
  arm_right.off();

  // Stop blinking lamp
  blink_lamp = false;

  // Clear oc variables
  oc_left_occured = oc_right_occured = false;
}

void
program_cycle()
{
  // It is assumed that the gate is closed when entering the programming mode

  // Inform user that entered programming mode
  bip_buzzer(NUMBER_BIPS_BEFORE_PROG);

  // Start blinking lamp
  blink_lamp = true;  

  // Open left arm
  while (!btn_select.isPressed()); // wait for btn
  arm_left.open();
  unsigned long left_arm_start_open = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // Open right arm
  while (!btn_select.isPressed()); // wait for btn
  arm_right.open();
  unsigned long right_arm_start_open = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // Stop left arm
  while (!btn_select.isPressed() && !oc_left_occured); // wait for btn or oc left
  arm_left.off();
  unsigned long left_arm_stop_open = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // Stop right arm
  while (!btn_select.isPressed() && !oc_right_occured); // wait for btn or oc right
  arm_right.off();
  unsigned long right_arm_stop_open = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // Clear oc variables
  oc_left_occured = oc_right_occured = false;

  // Close right arm
  while (!btn_select.isPressed()); // wait for btn
  arm_right.close();
  unsigned long right_arm_start_close = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // Close left arm
  while (!btn_select.isPressed()); // wait for btn
  arm_left.close();
  unsigned long left_arm_start_close = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // Stop right arm
  while (!btn_select.isPressed() && !oc_right_occured); // wait for btn or oc right
  arm_right.off();
  unsigned long right_arm_stop_close = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // Stop left arm
  while (!btn_select.isPressed() && !oc_left_occured); // wait for btn or oc left
  arm_left.off();
  unsigned long left_arm_stop_close = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // Clear oc variables
  oc_left_occured = oc_right_occured = false;

  // Stop blinking lamp
  blink_lamp = false;

  // Recalculate open and close timings
  left_arm_open_time.set(left_arm_stop_open - left_arm_start_open);
  left_arm_close_time.set(left_arm_stop_close - left_arm_start_close);
  right_arm_open_time.set(right_arm_stop_open - right_arm_start_open);
  right_arm_close_time.set(right_arm_stop_close - right_arm_start_close);
  arms_open_delta.set(right_arm_start_open - left_arm_start_open);
  arms_close_delta.set(left_arm_start_close - right_arm_start_close);

  // Inform user that exited programming mode
  bip_buzzer(NUMBER_BIPS_AFTER_PROG);
}

/*
 * =============================================================================
 * =================================== TASKS ===================================
 * =============================================================================
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
    // Open or close cycle
    if (btn_select.isPressed())
      open_close_cycle();
    // Close while pressed
    else if (btn_close.isPressed())
      close_while_pressed();
    // Open while pressed
    else if (btn_open.isPressed())
      open_while_pressed();
    // Program cycle
    else if (btn_program.isPressed())
      program_cycle();      
  }
}

/*
 * =============================================================================
 * ========================= ARDUINO BULT-IN FUNCTIONS =========================
 * =============================================================================
 */
void
setup()
{
  // Set sensors offset on boot
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
