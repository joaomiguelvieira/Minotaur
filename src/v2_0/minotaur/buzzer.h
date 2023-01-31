#ifndef __BUZZER_H__
#define __BUZZER_H__

// External libraries
#include <Arduino.h>

class Buzzer
{
private:
  uint8_t pin;

public:
  // Constructor
  Buzzer(uint8_t pin);

  // Public methods
  void on();
  void off();
};

#endif //__BUZZER_H__
