#ifndef __CURRENT_SENSOR_H__
#define __CURRENT_SENSOR_H__

// External libraries
#include <Arduino.h>

class CurrentSensor
{
private:
  uint8_t pin;
  uint16_t offset = 512;

public:
  // Constructor
  CurrentSensor(uint8_t pin);

  // Public methods
  uint16_t getValue();
  void setOffset();
  uint16_t getOffset();

  /*
  * ACS712 Datasheet: https://www.farnell.com/datasheets/1759100.pdf (Ip=20A)
  * In theory, a variation of 1V in the output represents a variation of 10A.
  * However, experimental results with a Minotaur board showed a variation of
  * approximately 75mV/A for the left sensor and 100mV/A for the right sensor.
  * Since the purpose of these sensors is just to avoid the overload of the PSU
  * (and we are working with confortable margins), an error of 25% can just be
  * ignored, and we can work with the theoretical variation of 100mV/A or 20
  * steps of the Arduino ADC per Amp: A = (Reading - Offset) / 20.
  * The offset depends on Vcc. Therefore, it is obtained for I=0A at boot time.
  */
  float getAmps();
};

#endif //__CURRENT_SENSOR_H__
