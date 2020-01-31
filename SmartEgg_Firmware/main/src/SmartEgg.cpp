#include "SmartEgg.h"

SmartEgg::SmartEgg() {
}

bool SmartEgg::begin() {
  accel = new ADXL377(ADC1_CHANNEL_0, ADC1_CHANNEL_3, ADC1_CHANNEL_6, 25, 4, 5);
  dataRec = new DataRecorder(accel);

  m_pref = new Preferences();
  m_pref->begin("SmartEgg", false);
  bootcount = m_pref->getULong("bootcount", 0);
  bootcount ++;
  m_pref->putULong("bootcount", bootcount);
  m_pref->end();
  Serial.printf("Bootcount: %lu\r\n", bootcount);
  return true;
}



int SmartEgg::recordStart(String recName) {
  int ret = dataRec->recordStart(recName);
  if(ret < 0)
    return ret;

  /* Start the task */
  Serial.println("Started writeTask");
  xTaskCreatePinnedToCore(SMARTEGG.writeTask, "writeTask", 2048, NULL, 4, &writeTaskHandle, 0);

  return ret;
}

int SmartEgg::recordStop() {
  int ret = dataRec->recordStop();

  Serial.println("Removed writeTask");
  vTaskDelete(writeTaskHandle);
  writeTaskHandle = NULL;

  return ret;  
}



void SmartEgg::writeTask(void *pvParameter) {
  /* Loop Task */
  for(;;) {
    SMARTEGG.dataRec->writeTask();
    vTaskDelay(100);
  }
}

SmartEgg SMARTEGG;
