#ifndef __LAMP_H__
#define __LAMP_H__

// External libraries
#include <Arduino.h>

class Lamp
{
private:
  uint8_t pin;

public:
  // Constructor
  Lamp(uint8_t pin);

  // Public methods
  void on();
  void off();
};

#endif //__LAMP_H__
