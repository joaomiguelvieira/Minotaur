#ifndef __MOTOR_ARM_H__
#define __MOTOR_ARM_H__

// External libraries
#include <Arduino.h>

class MotorArm
{
private:
  uint8_t pin_p, pin_n;

public:
  // Constructor
  MotorArm(uint8_t pin_p, uint8_t pin_n);

  void open();
  void close();
  void off();
};

#endif //__MOTOR_ARM_H__
