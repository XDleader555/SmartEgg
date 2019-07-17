#include "SmartEgg.h"

SmartEgg::SmartEgg() {
}

bool SmartEgg::begin() {
  accel = new ADXL377(ADC1_CHANNEL_7, ADC1_CHANNEL_6, ADC1_CHANNEL_5, 39, 25, 26);
  dataRec = new DataRecorder(accel);

  m_pref = new Preferences();
}

void SmartEgg::setAPName(String name) {
  if(name.charAt(0) == '"')
    name = name.substring(1);
  if(name.charAt(name.length() - 1) == '"')
    name = name.substring(0, name.length() - 1);
  
  m_pref->begin("SmartEgg", false);
  m_pref->putString("APName", name);
  m_pref->end();

  Serial.printf("Set AP Name to %s. Please reboot for changes to take effect.\n", name.c_str());
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

String SmartEgg::getAPName() {
  String APName;
  m_pref->begin("SmartEgg", false);
  APName = m_pref->getString("APName", "Nameless SmartEgg");
  m_pref->end();

  return APName;
}

void SmartEgg::writeTask(void *pvParameter) {
  /* Loop Task */
  for(;;) {
    SMARTEGG.dataRec->writeTask();
    vTaskDelay(100);
  }
}

SmartEgg SMARTEGG;
