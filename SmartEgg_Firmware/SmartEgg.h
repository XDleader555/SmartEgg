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
