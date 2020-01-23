#include "Arduino.h"
#include "MiyaSh.h"
#include "containsint.h"
#include "SmartEgg.h"

inline void startWifiCmd(MiyaSh* shell, String args[], int arglen) {
  SMARTEGG.dataRec->setupWifi();
}

inline void stopWifiCmd(MiyaSh* shell, String args[], int arglen) {
  SMARTEGG.dataRec->disableWifi();
}

inline void printplldataCmd(MiyaSh* shell, String args[], int arglen) {
  SMARTEGG.dataRec->printplldata();
}
inline void recordCmd(MiyaSh* shell, String args[], int arglen) {
  if(args[0] == "stop") {
    SMARTEGG.recordStop();
  } else if(args[0] == "list") {
    Serial.printf("%luKB/%dKB of EEPROM left for data recording\n%s",
                SMARTEGG.dataRec->getSpaceLeft()/1024,
                EEPROM_SIZE/1024,
                SMARTEGG.dataRec->getRecordingsList().c_str());
  } else if(args[0] == "setTime") {
    if(arglen > 1) {
      if(containsInt(args[1]))
        SMARTEGG.dataRec->setRecordTime(args[1].toInt());
    } else {
      Serial.println(F("Please enter the max recording time in seconds!"));
    }
  } else if(args[0] == "start") {
    if(arglen > 1)
      SMARTEGG.recordStart(args[1]);
    else
      Serial.println(F("Please enter a name for your recording!"));
  } else if(args[0] == "delete"){
    if(arglen > 1)
      if(args[1] == "--all")
        SMARTEGG.dataRec->deleteAllRecordings();
      else
        SMARTEGG.dataRec->deleteRecording(args[1]);
    else
      Serial.println(F("Please enter a recording to delete!"));
  } else if(args[0] == "dump") {
    if(arglen > 1)
      SMARTEGG.dataRec->printAllValues(args[1]);
    else
      Serial.println(F("Please specify a recording"));
  } else if(args[0] == "info") {
    if(arglen > 1)
      SMARTEGG.dataRec->recordInfo(args[1]);
    else
      Serial.println(F("Please specify a recording"));
  } else {
    // Help Page
    Serial.println(F("Usage: record {start/stop/list/dump/delete/setTime}"));
    Serial.println(F("start: Begins a recording with specified name"));
    Serial.println(F("stop: Ends a currently running recording"));
    Serial.println(F("list: Displays the names of currently saved recordings"));
    Serial.println(F("dump: Prints the saved data to the console"));
    Serial.println(F("delete: Deletes a recording (--all)"));
    Serial.println(F("setTime: Set the max recording time"));
    Serial.println(F("\nExample: record start myRecordingName"));
  }
}

inline void setAPNameCmd(MiyaSh* shell, String args[], int arglen) {
  if(arglen > 0) {
    /* Rebuild the name string */
    String apName = "";
    for(int i = 0; i < arglen; i++)
      apName += String(args[i]) + " ";
    apName.trim();
    SMARTEGG.dataRec->setAPName(apName);
  } else {
    Serial.println(F("Please specify a WiFi access point name!"));
  }
}

inline void getAvgStrCmd(MiyaSh* shell, String args[], int arglen) {
  Serial.println(SMARTEGG.dataRec->getAvgStr());
}

inline void calibrateCmd(MiyaSh* shell, String args[], int arglen) {
  SMARTEGG.accel->calibrate();
}

inline void routeVRefCmd(MiyaSh* shell, String args[], int arglen) {
  if (adc2_vref_to_gpio(GPIO_NUM_27) == ESP_OK){
      SMARTEGG.accel->disableRolling();
      Serial.println(F("v_ref routed to GPIO 27 and rolling avg disabled. Reboot to remove route."));
  } else {
      Serial.println(F("Failed to route v_ref\n"));
  }
}

inline void setVRefCmd(MiyaSh* shell, String args[], int arglen) {
  if(arglen > 0)
    SMARTEGG.accel->setVRef(args[0].toInt());
  else
    Serial.println(F("Voltage (mv) parameter required!"));
}

inline void setVRegCmd(MiyaSh* shell, String args[], int arglen) {
  if(arglen > 0)
    SMARTEGG.accel->setVReg(args[0].toInt());
  else
    Serial.println(F("Voltage (mv) parameter required!"));
}