/*
  EEPROM32.h - EEPROM Library with extended addressing support
  Copyright (c) 2017 Andrew Miyaguchi. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef EEPROM32_H
#define EEPROM32_H

#ifndef EEPROM_FLASH_PARTITION_NAME
 #define EEPROM_FLASH_PARTITION_NAME "eeprom"
#endif

#define EEPROM_ALIGN 4096

#include "Arduino.h"
#include <esp_partition.h>

class EEPROM32 {
  public:
    EEPROM32();
    bool begin(unsigned long size);
    uint8_t read(unsigned long address);
    unsigned long align(unsigned long address);
    void write(unsigned long address, uint8_t val);
    void copy(unsigned long sourceAddr, unsigned long destAddr, unsigned long recSize);
    void erase(unsigned long address, unsigned long recSize);
    void erase();
    
  private:
    unsigned long m_size;
    const esp_partition_t* m_part;
};

extern EEPROM32 EEPROM;

#endif /* EEPROM32_H */