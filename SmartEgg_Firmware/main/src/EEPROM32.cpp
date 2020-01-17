#include "EEPROM32.h"

EEPROM32::EEPROM32(void) {
}

bool EEPROM32::begin(unsigned long size) {
  /* Setup the partition */
  m_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_ANY, EEPROM_FLASH_PARTITION_NAME);
  if (m_part == NULL) {
    return false;
  }

  m_size = size;
  return true;
}

uint8_t EEPROM32::read(unsigned long address) {
  uint8_t data[1];
  esp_partition_read(m_part, address, data, 1);
  return data[0];
}

unsigned long EEPROM32::align(unsigned long address) {
  if(address % EEPROM_ALIGN != 0) {
    address += EEPROM_ALIGN - (address % EEPROM_ALIGN);
  }

  return address;
}

void EEPROM32::copy(unsigned long sourceAddr, unsigned long destAddr, unsigned long recSize) {
  Serial.printf("Copying %lu to %lu with size of %lu...", sourceAddr, destAddr, recSize);
  
  for(int i = 0; i < recSize; i++) {
    if(i % EEPROM_ALIGN == 0)
      EEPROM.erase(destAddr+i, EEPROM_ALIGN);
      
    EEPROM.write(destAddr+i, EEPROM.read(sourceAddr+i));
  }
  
  Serial.println("Done");
}

void EEPROM32::write(unsigned long address, uint8_t val) {
  uint8_t data[1];
  data[0] = val;
  
  esp_partition_write(m_part, address, data, 1);
}

void EEPROM32::erase(unsigned long address, unsigned long recSize) {
  esp_partition_erase_range(m_part, address, recSize);
}

void EEPROM32::erase() {
  esp_partition_erase_range(m_part, 0, m_size);
}

EEPROM32 EEPROM;