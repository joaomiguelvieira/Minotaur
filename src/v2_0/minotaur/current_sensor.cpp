#include "current_sensor.h"

CurrentSensor::CurrentSensor(uint8_t pin) :
  pin(pin)
{}

uint16_t
CurrentSensor::getValue()
{
  return analogRead(pin);
}

void
CurrentSensor::setOffset()
{
  offset = analogRead(pin);
}

float
CurrentSensor::getAmps()
{
  return (1.0 * analogRead(pin) - offset) / 20;
}

uint16_t
CurrentSensor::getOffset()
{
  return offset;
}
