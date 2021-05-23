/* 
 * Title: Minotaur v1.0 firmware
 * Author: Joao Vieira
 * Date: november 28th, 2020
 * 
 * Description: This is the firmware for the Minotaur board for
 * controlling swing automatic gates. The Minotaur board does not
 * include any RFID decoder. That is an extra piece of hardware 
 * that should be connected to the board acting like a button
 * (closed when activated and open when deactivated).
 */

// libraries
#include <Arduino_FreeRTOS.h>
#include <EEPROM.h>

// inputs and outputs
#define VCC_ARM_SW    2  // OUTPUT switch arm voltage
#define LEFT_ARM_PIN  3  // OUTPUT activate left arm
#define RIGHT_ARM_PIN 4  // OUTPUT activate right arm
#define LAMP_PIN      5  // OUTPUT activate lamp
#define BTN_ATN       6  // INPUT  antena activated
#define BUZ_PIN       7  // OUTPUT activate buzzer
#define BTN_CLOSE     8  // INPUT  button close selected
#define BTN_PROGRAM   9  // INPUT  button program selected
#define BTN_SELECT    10 // INPUT  button select selected
#define BTN_SPARE     11 // INPUT  button spare selected
#define BTN_OPEN      12 // INPUT  button open selected

// macros for logical values
#define ARM_OFF   HIGH
#define ARM_ON    LOW
#define ARM_CLOSE LOW
#define ARM_OPEN  HIGH
#define LAMP_OFF  HIGH
#define LAMP_ON   LOW
#define BTN_ON    HIGH
#define BTN_OFF   LOW

// other macros
#define BUZZER_FREQ  1000 // buzzer frequency [Hz]
#define BUZZER_BIP_T 1000 // buzzer bip period [ms]
#define LAMP_BLINK_T 1000 // lamp blink period [ms]
#define OPEN_CLOSE_DELAY LAMP_BLINK_T * 2 // time between lamp starts blinking and gate starts moving
#define NUMBER_BIPS_BEFORE_PROG 3 // number of bips before entering programming mode
#define NUMBER_BIPS_AFTER_PROG 2 // number of bips after exiting programming mode
#define PROG_BTN_DELAY 500 // delay after pressing button in programming mode [ms]

// buttons
#define N_DIGITAL_PINS 14 // size of the debounce array
#define DEBOUNCE_DELAY 50 // miliseconds for debounce delay

volatile bool btnState[N_DIGITAL_PINS];          // state of buttons is read from here
bool lastBtnState[N_DIGITAL_PINS];               // auxiliary debounce variable
unsigned long lastDebounceTime[N_DIGITAL_PINS]; // auxiliary debounce variable

// state variables
volatile bool lampOn;
bool gateOpen;
unsigned long leftArmOpenTime, leftArmCloseTime;
unsigned long rightArmOpenTime, rightArmCloseTime;
unsigned long armsOpenDelta, armsCloseDelta;

// eeprom addresses
#define EEPROM_ULONG_SIZE                4 // size of unsigned long
#define EEPROM_LEFT_ARM_OPEN_TIME_ADDR   0
#define EEPROM_LEFT_ARM_CLOSE_TIME_ADDR  EEPROM_LEFT_ARM_OPEN_TIME_ADDR   + EEPROM_ULONG_SIZE
#define EEPROM_RIGHT_ARM_OPEN_TIME_ADDR  EEPROM_LEFT_ARM_CLOSE_TIME_ADDR  + EEPROM_ULONG_SIZE
#define EEPROM_RIGHT_ARM_CLOSE_TIME_ADDR EEPROM_RIGHT_ARM_OPEN_TIME_ADDR  + EEPROM_ULONG_SIZE
#define EEPROM_ARMS_OPEN_DELTA_ADDR      EEPROM_RIGHT_ARM_CLOSE_TIME_ADDR + EEPROM_ULONG_SIZE
#define EEPROM_ARMS_CLOSE_DELTA_ADDR     EEPROM_ARMS_OPEN_DELTA_ADDR      + EEPROM_ULONG_SIZE

/*
 * ===================================================================
 * ==================== ARDUINO BULT-IN FUNCTIONS ====================
 * ===================================================================
 */

void setup() {
  // setup inputs and outputs
  pinMode(VCC_ARM_SW,    OUTPUT);
  pinMode(LEFT_ARM_PIN,  OUTPUT);
  pinMode(RIGHT_ARM_PIN, OUTPUT);
  pinMode(LAMP_PIN,      OUTPUT);
  pinMode(BTN_ATN,       INPUT);
  pinMode(BUZ_PIN,       OUTPUT);
  pinMode(BTN_CLOSE,     INPUT);
  pinMode(BTN_PROGRAM,   INPUT);
  pinMode(BTN_SELECT,    INPUT);
  pinMode(BTN_SPARE,     INPUT);
  pinMode(BTN_OPEN,      INPUT);

  // activate buzzer until setup completes
  tone(BUZ_PIN, BUZZER_FREQ);

  // open serial console
  Serial.begin(9600);
  while (!Serial); // wait for serial to connect

  // initialize outputs
  digitalWrite(LEFT_ARM_PIN, ARM_OFF);
  digitalWrite(RIGHT_ARM_PIN, ARM_OFF);
  digitalWrite(LAMP_PIN, LAMP_OFF);

  // initialize global variables
  for (int i = 0; i < N_DIGITAL_PINS; i++) {
    btnState[i] = BTN_OFF;
    lastBtnState[i] = BTN_OFF;
    lastDebounceTime[i] = 0;
  }

  lampOn = false;

  // load open and close timings from persistent memory
  EEPROM.get(EEPROM_LEFT_ARM_OPEN_TIME_ADDR,   leftArmOpenTime);
  EEPROM.get(EEPROM_LEFT_ARM_CLOSE_TIME_ADDR,  leftArmCloseTime);
  EEPROM.get(EEPROM_RIGHT_ARM_OPEN_TIME_ADDR,  rightArmOpenTime);
  EEPROM.get(EEPROM_RIGHT_ARM_CLOSE_TIME_ADDR, rightArmCloseTime);
  EEPROM.get(EEPROM_ARMS_OPEN_DELTA_ADDR,      armsOpenDelta);
  EEPROM.get(EEPROM_ARMS_CLOSE_DELTA_ADDR,     armsCloseDelta);

  // launch threads
  xTaskCreate(taskDebounceBtns, "Debounce buttons", 128, NULL, 3, NULL);
  xTaskCreate(taskBlinkLamp,    "Blink lamp",       128, NULL, 3, NULL);
  xTaskCreate(taskMain,         "Main routine",     128, NULL, 3, NULL);

  // deactivate buzzer when setup completes
  noTone(BUZ_PIN);
}

void loop() {
  // workload is dealt with in tasks
}

/*
 * ===================================================================
 * ======================== AUXILIARY ROUTINES =======================
 * ===================================================================
 */

void debounceBtn(int btn) {
  int reading = digitalRead(btn);

  if (reading != lastBtnState[btn]) {
    lastDebounceTime[btn] = millis();
  }

  if ((millis() - lastDebounceTime[btn]) > DEBOUNCE_DELAY) {
    if (reading != btnState[btn]) {
      btnState[btn] = reading;
    }
  }

  lastBtnState[btn] = reading;
}

void bipBuzzer(int bips) {
  for (int i = 0; i < bips; i++) {
    tone(BUZ_PIN, BUZZER_FREQ);
    vTaskDelay(BUZZER_BIP_T / 2 / portTICK_PERIOD_MS);
    noTone(BUZ_PIN);
    vTaskDelay(BUZZER_BIP_T / 2 / portTICK_PERIOD_MS);
  }
}

void openCloseCycle() {
  // switch arm voltage depending on gate state
  digitalWrite(VCC_ARM_SW, gateOpen ? ARM_CLOSE : ARM_OPEN);
  
  // start blinking lamp
  lampOn = true;
  vTaskDelay(OPEN_CLOSE_DELAY / portTICK_PERIOD_MS);

  // start first arm
  digitalWrite(gateOpen ? RIGHT_ARM_PIN : LEFT_ARM_PIN, ARM_ON);
  vTaskDelay((gateOpen ? armsCloseDelta : armsOpenDelta) / portTICK_PERIOD_MS);

  // start second arm
  digitalWrite(gateOpen ? LEFT_ARM_PIN : RIGHT_ARM_PIN, ARM_ON);
  vTaskDelay((gateOpen ? (rightArmCloseTime - armsCloseDelta) : (leftArmOpenTime - armsOpenDelta)) / portTICK_PERIOD_MS);

  // stop first arm
  digitalWrite(gateOpen ? RIGHT_ARM_PIN : LEFT_ARM_PIN, ARM_OFF);
  vTaskDelay((gateOpen ? (leftArmCloseTime - rightArmCloseTime + armsCloseDelta) : (rightArmOpenTime - leftArmOpenTime + armsOpenDelta)) / portTICK_PERIOD_MS);

  // stop second arm
  digitalWrite(gateOpen ? LEFT_ARM_PIN : RIGHT_ARM_PIN, ARM_OFF);

  // stop blinking lamp
  lampOn = false;

  // switch gate state
  gateOpen = !gateOpen;
}

void closeWhilePressed() {
  // set voltage to close mode
  digitalWrite(VCC_ARM_SW, ARM_CLOSE);

  // continue closing until button not pressed
  while (btnState[BTN_CLOSE] == BTN_ON) {
    lampOn = true;
    digitalWrite(btnState[BTN_SPARE] == BTN_ON ? RIGHT_ARM_PIN : LEFT_ARM_PIN, ARM_ON);
  }

  // stop both arms from moving
  digitalWrite(LEFT_ARM_PIN, ARM_OFF);
  digitalWrite(RIGHT_ARM_PIN, ARM_OFF);
  lampOn = false;
}

void openWhilePressed() {
  // set voltage to open mode
  digitalWrite(VCC_ARM_SW, ARM_OPEN);

  // continue opening until button not pressed
  while (btnState[BTN_OPEN] == BTN_ON) {
    lampOn = true;
    digitalWrite(btnState[BTN_SPARE] == BTN_ON ? RIGHT_ARM_PIN : LEFT_ARM_PIN, ARM_ON);
  }

  // stop both arms from moving and lamp from blinking
  digitalWrite(LEFT_ARM_PIN, ARM_OFF);
  digitalWrite(RIGHT_ARM_PIN, ARM_OFF);
  lampOn = false;
}

void programCycle() {
  // the gate must be fully closed when entering programming mode
  
  // inform user that entered programming mode
  bipBuzzer(NUMBER_BIPS_BEFORE_PROG);
  
  lampOn = true; // turn lamp on

  // set voltage to open
  digitalWrite(VCC_ARM_SW, ARM_OPEN);

  // open left arm
  while ((btnState[BTN_SELECT] != BTN_ON) && (btnState[BTN_ATN] != BTN_ON)); // wait for btn
  digitalWrite(LEFT_ARM_PIN, ARM_ON);
  unsigned long leftArmStartOpen = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // open right arm
  while ((btnState[BTN_SELECT] != BTN_ON) && (btnState[BTN_ATN] != BTN_ON)); // wait for btn
  digitalWrite(RIGHT_ARM_PIN, ARM_ON);
  unsigned long rightArmStartOpen = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // stop left arm
  while ((btnState[BTN_SELECT] != BTN_ON) && (btnState[BTN_ATN] != BTN_ON)); // wait for btn
  digitalWrite(LEFT_ARM_PIN, ARM_OFF);
  unsigned long leftArmStopOpen = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // stop right arm
  while ((btnState[BTN_SELECT] != BTN_ON) && (btnState[BTN_ATN] != BTN_ON)); // wait for btn
  digitalWrite(RIGHT_ARM_PIN, ARM_OFF);
  unsigned long rightArmStopOpen = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // set voltage to close
  digitalWrite(VCC_ARM_SW, ARM_CLOSE);

  // close right arm
  while ((btnState[BTN_SELECT] != BTN_ON) && (btnState[BTN_ATN] != BTN_ON)); // wait for btn
  digitalWrite(RIGHT_ARM_PIN, ARM_ON);
  unsigned long rightArmStartClose = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // close left arm
  while ((btnState[BTN_SELECT] != BTN_ON) && (btnState[BTN_ATN] != BTN_ON)); // wait for btn
  digitalWrite(LEFT_ARM_PIN, ARM_ON);
  unsigned long leftArmStartClose = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // stop right arm
  while ((btnState[BTN_SELECT] != BTN_ON) && (btnState[BTN_ATN] != BTN_ON)); // wait for btn
  digitalWrite(RIGHT_ARM_PIN, ARM_OFF);
  unsigned long rightArmStopClose = millis();
  vTaskDelay(PROG_BTN_DELAY / portTICK_PERIOD_MS);

  // stop left arm
  while ((btnState[BTN_SELECT] != BTN_ON) && (btnState[BTN_ATN] != BTN_ON)); // wait for btn
  digitalWrite(LEFT_ARM_PIN, ARM_OFF);
  unsigned long leftArmStopClose = millis();

  lampOn = false; // turn lamp off

  // recalculate open and close timings
  leftArmOpenTime = leftArmStopOpen - leftArmStartOpen;
  leftArmCloseTime = leftArmStopClose - leftArmStartClose;
  rightArmOpenTime = rightArmStopOpen - rightArmStartOpen;
  rightArmCloseTime = rightArmStopClose - rightArmStartClose;
  armsOpenDelta = rightArmStartOpen - leftArmStartOpen;
  armsCloseDelta = leftArmStartClose - rightArmStartClose;

  // store open and close timings to persistent memory
  EEPROM.put(EEPROM_LEFT_ARM_OPEN_TIME_ADDR,   leftArmOpenTime);
  EEPROM.put(EEPROM_LEFT_ARM_CLOSE_TIME_ADDR,  leftArmCloseTime);
  EEPROM.put(EEPROM_RIGHT_ARM_OPEN_TIME_ADDR,  rightArmOpenTime);
  EEPROM.put(EEPROM_RIGHT_ARM_CLOSE_TIME_ADDR, rightArmCloseTime);
  EEPROM.put(EEPROM_ARMS_OPEN_DELTA_ADDR,      armsOpenDelta);
  EEPROM.put(EEPROM_ARMS_CLOSE_DELTA_ADDR,     armsCloseDelta);

  // inform user that exited programming mode
  bipBuzzer(NUMBER_BIPS_AFTER_PROG);
}

/*
 * ===================================================================
 * =================== ROUTINES EXECUTED BY THREADS ==================
 * ===================================================================
 */

void taskDebounceBtns(void *pvParameters) {
  (void) pvParameters;

  while (true) { // execute thread forever
    debounceBtn(BTN_ATN);
    debounceBtn(BTN_CLOSE);
    debounceBtn(BTN_PROGRAM);
    debounceBtn(BTN_SELECT);
    debounceBtn(BTN_SPARE);
    debounceBtn(BTN_OPEN);
  }
}

void taskBlinkLamp(void *pvParameters) {
  (void) pvParameters;

  while (true) { // execute thread forever    
    if (lampOn) {
      digitalWrite(LAMP_PIN, LAMP_ON);
      vTaskDelay(LAMP_BLINK_T / 2 / portTICK_PERIOD_MS);
      digitalWrite(LAMP_PIN, LAMP_OFF);
      vTaskDelay(LAMP_BLINK_T / 2 / portTICK_PERIOD_MS);
    }
  }
}

void taskMain(void *pvParameters) {
  (void) pvParameters;

  while (true) { // execute thread forever
    // open or close cycle
    if (btnState[BTN_ATN] == BTN_ON || btnState[BTN_SELECT] == BTN_ON) {
      openCloseCycle();
    }
    // close while pressed
    else if (btnState[BTN_CLOSE] == BTN_ON) {
      closeWhilePressed();
    }
    // open while pressed
    else if (btnState[BTN_OPEN] == BTN_ON) {
      openWhilePressed();
    }
    // program cycle
    else if (btnState[BTN_PROGRAM] == BTN_ON) {
      programCycle();
    }
  }
}
