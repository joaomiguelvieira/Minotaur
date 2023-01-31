#include "persistent_var.h"

unsigned int PersistentVar::eof = 0;

PersistentVar::PersistentVar()
{
  eeprom_ptr = eof;
  eof += sizeof(DATA_TYPE);
  EEPROM.get(eeprom_ptr, var);
}

DATA_TYPE
PersistentVar::get()
{
  return var;
}

void
PersistentVar::set(DATA_TYPE value)
{
  var = value;
  EEPROM.put(eeprom_ptr, value);
}

unsigned int
PersistentVar::getEepromPtr()
{
  return eeprom_ptr;
}
