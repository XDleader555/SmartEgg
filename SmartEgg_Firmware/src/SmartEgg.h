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

#ifndef SMARTEGG_H
#define SMARTEGG_H
  
#include "Arduino.h"
#include "ADXL377.h"
#include "DataRecorder.h"
#include <Preferences.h>

class SmartEgg {
  public:
    SmartEgg();
    bool begin();
    void setAPName(String name);
    String getAPName();
    int recordStart(String recName);
    int recordStop();
    static void writeTask(void *pvParameters);
    
    ADXL377* accel;
    DataRecorder* dataRec;

  private:
    Preferences* m_pref;                 /* Preferences variable */
    TaskHandle_t writeTaskHandle = NULL;
};

extern SmartEgg SMARTEGG;

#endif /* SMARTEGG_H */