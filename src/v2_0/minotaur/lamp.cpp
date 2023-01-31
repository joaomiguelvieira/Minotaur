#include "lamp.h"

Lamp::Lamp(uint8_t pin) :
  pin(pin)
{
  pinMode(pin, OUTPUT);
}

void
Lamp::on()
{
  digitalWrite(pin, HIGH);
}

void
Lamp::off()
{
  digitalWrite(pin, LOW);
}
