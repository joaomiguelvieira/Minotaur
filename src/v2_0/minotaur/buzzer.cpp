#include "buzzer.h"

Buzzer::Buzzer(uint8_t pin) :
  pin(pin)
{
  pinMode(pin, OUTPUT);
}

void
Buzzer::on()
{
  digitalWrite(pin, HIGH);
}

void
Buzzer::off()
{
  digitalWrite(pin, LOW);
}
