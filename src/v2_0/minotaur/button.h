#ifndef __BUTTON_H__
#define __BUTTON_H__

// External libraries
#include <Arduino.h>

#define DEBOUNCE_DELAY 50 // debounce period [ms]

class Button
{
private:
  uint8_t pin;
  unsigned int debounce_delay = DEBOUNCE_DELAY;
  volatile bool state = false;
  bool last_state = false;
  unsigned long last_debounce_time = 0;

public:
  // Constructor
  Button(uint8_t pin);

  // Public methods
  void debounce();
  bool isPressed();
  void setDebounceDelay(unsigned int milliseconds);
};

#endif //__BUTTON_H__
