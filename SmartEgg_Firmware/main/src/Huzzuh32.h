/*
  Huzzuh32.h - Adafruit Huzzuh32 specific functions
  
  Copyright (c) 2019 Andrew Miyaguchi. All rights reserved.

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

#ifndef HUZZAH32_H
#define HUZZAH32_H

#define V_REF 1100
#define V_REG 3300
#define ADC_BATT ADC1_CHANNEL_7

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#include <stdlib.h>
#include <math.h>
#include <esp_adc_cal.h>
#include <mapf.h>


class _Huzzah32 {
public:
    void begin() {
        adc1_config_width(ADC_WIDTH_12Bit);
        if(adc1_config_channel_atten(ADC_BATT, ADC_ATTEN_11db) != ESP_OK)
            printf("Error configuring battery level input");
    }

    /**
     * Reads the battery level through gpio 35, or ADC1_CHANNEL_7
     * The voltage divider divides the battery voltage in half.
     * Lower cutoff: 3.2v
     * Upper cutoff: 4.2v
     * Nominal: 3.7v
     * 
     * @param s String to check
     * @return analog value between 
     */
    float readBattery() {
        uint32_t rawRead;
        float batt_percent;
        esp_adc_cal_characteristics_t m_characteristics;

        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, V_REF, &m_characteristics);
        esp_adc_cal_get_voltage((adc_channel_t) ADC_BATT, &m_characteristics, &rawRead);
        
        rawRead += 500; // offset value
        batt_percent = mapf((float) rawRead, ((3200/2.0)/((float) V_REG))*4096.0, ((4200/2.0)/((float) V_REG))*4096.0, 0, 100);
        
        return MIN(MAX(batt_percent, 0.0), 100.0);
    };
};

extern _Huzzah32 Huzzah32;

#endif /* HUZZAH32_H */