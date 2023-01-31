#include "button.h"

Button::Button(uint8_t pin) :
  pin(pin)
{
  pinMode(pin, INPUT);
}

void 
Button::debounce()
{
  bool reading = digitalRead(pin);

  if (reading != last_state)
    last_debounce_time = millis();

  if ((millis() - last_debounce_time) > debounce_delay && reading != state)
    state = reading;

  last_state = reading;
}

bool
Button::isPressed()
{
  return state;
}

void
Button::setDebounceDelay(unsigned int milliseconds)
{
  debounce_delay = milliseconds;
}
