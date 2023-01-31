#include "motor_arm.h"

MotorArm::MotorArm(uint8_t pin_p, uint8_t pin_n) :
  pin_p(pin_p),
  pin_n(pin_n)
{
  pinMode(pin_p, OUTPUT);
  pinMode(pin_n, OUTPUT);
}

void
MotorArm::open()
{
  digitalWrite(pin_p, HIGH);
  digitalWrite(pin_n, LOW);
}

void
MotorArm::close()
{
  digitalWrite(pin_p, LOW);
  digitalWrite(pin_n, HIGH);
}

void
MotorArm::off()
{
  digitalWrite(pin_p, LOW);
  digitalWrite(pin_n, LOW);
}
