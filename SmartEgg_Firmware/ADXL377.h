/*
  This file is part of SmartEgg_Firmware.
  Copyright (c) 2019 Andrew Miyaguchi. All rights reserved.

  SmartEgg_Firmware is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SmartEgg_Firmware is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with SmartEgg_Firmware.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ADXL377_H
#define ADXL377_H

#include "Arduino.h"
#include <Preferences.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <MiyaFunctions.h>

#define ROLLING_AVG_SIZE 100 /* Size of the rolling average */

class ADXL377 {
  public:
    ADXL377(adc1_channel_t xPin, adc1_channel_t yPin, adc1_channel_t zPin, int stPin, int vccPin, int gndPin);
    void run();
    void updateAvg();
    String calibrate();
    void setOffsets(float* calData);
    void enableRolling();
    void disableRolling();
    void setVRef(int vRef);
    void setVReg(int vReg);
    float* read();
    int* readInt();
    float* getAvg();

  private:
    Preferences* m_pref;
    esp_adc_cal_characteristics_t m_characteristics;
    adc1_channel_t m_pins[3];
    uint32_t m_vRef;                       // Reference Voltage from internal 1.1v regualtor
    uint32_t m_vReg;                       // Reference Voltage from external 3.3v regulator
    int m_stPin;
    int m_vccPin;
    int m_gndPin;
    int m_rollingAvgIter;
    float m_offsets[3];
    float* m_rollingAvgBuffer[ROLLING_AVG_SIZE];            /* Holds values for the rolling avg */
    bool m_calibrating, m_allowRolling, m_manualCal;

    float* getCalData();
};

#endif /* ADXL377_H */
