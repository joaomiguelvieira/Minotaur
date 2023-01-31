#ifndef __PERSISTENT_VAR_H__
#define __PERSISTENT_VAR_H__

// External libraries
#include <Arduino.h>
#include <EEPROM.h>

typedef unsigned long DATA_TYPE;

class PersistentVar
{
private:
  static unsigned int eof;
  unsigned int eeprom_ptr;
  DATA_TYPE var;
  
public:
  // Constructor
  PersistentVar();

  // Public methods
  DATA_TYPE get();
  void set(DATA_TYPE value);
  unsigned int getEepromPtr();
};

#endif //__PERSISTENT_VAR_H__
