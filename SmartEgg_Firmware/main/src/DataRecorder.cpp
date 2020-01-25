#include "DataRecorder.h"

DataRecorder::DataRecorder(ADXL377* accel) {
  /* Setup local variables */
  m_recFlag = false;
  m_writeMutex = false;
  m_recNumSamples = 0;
  m_accel = accel;
  m_request = REQUEST_EMPTY;
  m_requestStatus = 0;
  m_chunkedIter = 0;
  
  /* load recording data */
  m_pref = new Preferences();
  m_pref->begin("DataRec", false);
  m_recordings = m_pref->getString("RecordingNames", "");
  m_pref->end();
  Serial.printf("Loaded %d Recordings into ram:\n%s\n", getNumRecordings(), m_recordings.c_str());

  /* Setup the sample rate */
  setSampleRate(REC_HZ);

  /* Setup time to log */
  setRecordTime(RECORD_TIME);

  /* Setup the EEPROM */
  Serial.print("Mounting EEPROM...");
  if(!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("EEPROM Mount Failed! Did you flash the correct partition table?");
    return;
  } else {    
    Serial.printf("Done\n%luKB/%dKB of EEPROM left for data recording\n",
                    getSpaceLeft()/1024, EEPROM_SIZE/1024);
  }

  /* Setup circular write buffer */
  m_writeBuffer = NULL;
  m_freeAddress = 0;
  m_writeAddress = 0;
}

/* Check how much EEPROM is left */
unsigned long DataRecorder::getSpaceLeft() {
  unsigned long freeEEPROMSize = EEPROM_SIZE;
  
  if(getNumRecordings() > 0) {
    String lastRec = "DR_" + getRecordingName(getNumRecordings() - 1);

    m_pref->begin(lastRec.c_str(), false);
    unsigned long lastRecStartAddr = m_pref->getULong("startAddr", 0);
    unsigned long lastRecNumSamples = m_pref->getULong("numSamples", 0);
    m_pref->end();

    freeEEPROMSize -= (lastRecStartAddr + (lastRecNumSamples * 6));
  }

  return freeEEPROMSize ;
}

void DataRecorder::setAPName(String name) {
  if(name.charAt(0) == '"')
    name = name.substring(1);
  if(name.charAt(name.length() - 1) == '"')
    name = name.substring(0, name.length() - 1);
  
  m_pref->begin("SmartEgg", false);
  m_pref->putString("APName", name);
  m_pref->end();

  Serial.printf("Set AP Name to %s. Please reboot for changes to take effect.\n", name.c_str());
}

String DataRecorder::getAPName() {
  String APName;
  m_pref->begin("SmartEgg", false);
  APName = m_pref->getString("APName", "Nameless SmartEgg");
  m_pref->end();

  return APName;
}

void DataRecorder::setupWifi() {
  /* Make sure everything is disabled */
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  /* Start the WiFi Access Point */
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.softAPsetHostname(getAPName().c_str());
  //WiFi.setTxPower(WIFI_POWER_7dBm);
  WiFi.softAP(getAPName().c_str());
  
  Serial.print("Wireless AP Started: ");
  Serial.println(getAPName().c_str());
  Serial.print("Local IP address: ");
  Serial.println(WiFi.softAPIP());
}

void DataRecorder::disableWifi() {
  if(esp_wifi_stop() != ESP_OK) {
    Serial.println("Error disabling wifi");
  } else {
    Serial.println("Disconnected AP");
  }
}

int DataRecorder::getNumRecordings() {
  int numRec = 0;
  for(int i = 0; i < m_recordings.length(); i ++)
    if(m_recordings[i] == '\n')
      numRec++;

  return numRec;
}

String DataRecorder::getRecordingsList() {
  return m_recordings;
}

void DataRecorder::deleteAllRecordings() {
  for(int i = 0; i < getNumRecordings(); i++) {
    m_pref->begin(("DR_" + getRecordingName(i)).c_str(), false);
    m_pref->clear();
    m_pref->end();
  }

  m_recordings = "";
  m_pref->begin("DataRec", false);
  m_pref->putString("RecordingNames", m_recordings);
  m_pref->end();
}

void DataRecorder::deleteRecording(String recordingName) {
  if(m_recordings.indexOf(recordingName) == -1) {
    Serial.println("Could not delete recording: Not found");
    return;
  }

  /* Delete recording from preferences */
  m_pref->begin(("DR_" + recordingName).c_str(), false);
  m_pref->clear();
  m_pref->end();

  /* Remove recording from list */
  String temp = "";
  for(int i = 0; i < getNumRecordings(); i++){
    String tempRecName = getRecordingName(i);
    if(tempRecName != recordingName)
      temp += tempRecName + "\n";
  }
  m_recordings = temp;
  
  m_pref->begin("DataRec", false);
  m_pref->putString("RecordingNames", m_recordings);
  m_pref->end();
  
  defragment();

  Serial.printf("Deleted \"%s\"\n", recordingName.c_str());
  Serial.print(m_recordings.c_str());
  Serial.printf("%luKB/%dKB of EEPROM left for data recording\n",
              getSpaceLeft()/1024,
              EEPROM_SIZE/1024);
}

/*
 * Make the recordings continuous instead of dynamically allocated
 * in EEPROM memory because I'm feeling lazy
 */
void DataRecorder::defragment() {
  for(int i = 0; i < getNumRecordings(); i++) {
    m_pref->begin(("DR_" + getRecordingName(i)).c_str(), false);
    unsigned long currStartAddr = m_pref->getULong("startAddr", 0);
    unsigned long currSize = EEPROM.align(m_pref->getULong("numSamples", 0) * 6);
    m_pref->end();
    
    unsigned long calcStartAddr = calcNextStartAddress(i - 1);

    if(currStartAddr != calcStartAddr) {
      /* Copy to the defrag area, then to the new start Addr */
      EEPROM.copy(currStartAddr, calcStartAddr, currSize);
      
      m_pref->begin(("DR_" + getRecordingName(i)).c_str(), false);
      m_pref->putULong("startAddr", calcStartAddr);
      m_pref->end();
    }
    
  }
}

String DataRecorder::getRecordingName(int index) {
  String recName = "" + m_recordings;

  /* Remove leading names */
  for(int i = 0; i < index; i ++) {
    if(recName.indexOf('\n') == -1)
      return ""; /* Return empty if index is out of range */
      
    recName = recName.substring(recName.indexOf('\n') + 1);
  }

  /* Remove trailing names */
  if(recName.indexOf('\n') != -1) {
    recName = recName.substring(0, recName.indexOf('\n'));
  }

  //Serial.printf("Found \"%s\" at index %d\n", recName.c_str(), index);
  return recName;
}

bool DataRecorder::recordingExists(String recName) {
  for(int i = 0; i < getNumRecordings(); i ++)
    if(recName == getRecordingName(i))
      return true;
      
  return false;
}

/* Send a start record request tot he main core */
int DataRecorder::recordStart(String recordingName) {
  if(recordingExists(recordingName)) {
    Serial.println("Name for recording already exists!");
    return -1;
  }

  m_currRecording = recordingName;
  
  return request(REQUEST_START);
}

unsigned long DataRecorder::calcNextStartAddress(int fromRecIndex) {
  unsigned long nextStartAddr = 0;
  int maxIndex = getNumRecordings() - 1;
  
  if(fromRecIndex > maxIndex) {
    fromRecIndex = maxIndex;
  }
  
  if(fromRecIndex >= 0 ) {
    String recName = getRecordingName(fromRecIndex);

    /* Read the properties of the last recording */
    m_pref->begin(("DR_" + recName).c_str(), false);
    unsigned long recStartAddr = m_pref->getULong("startAddr", ULONG_MAX);
    unsigned long recSize = m_pref->getULong("numSamples", ULONG_MAX);
    m_pref->end();

    if(recStartAddr == ULONG_MAX) {
      Serial.printf("[WARNING] Invalid start address for %s!\n", recName.c_str());
    }

    recSize = recSize * 6;

    nextStartAddr = EEPROM.align(recStartAddr + recSize);
  }

  return nextStartAddr;
}

/* Send an end record request to the main core */
int DataRecorder::recordStop() {
  return request(REQUEST_END);
}

/* Initializes a request and waits for a response */
int DataRecorder::request(int requestType) {
  unsigned long timeoutTimerInit;
  if(m_request != REQUEST_EMPTY) {
    Serial.println("[WARNING] Ignoring duplicate request");
    return -4;
  }
  
  m_request = requestType;

  /* Wait a max of 5 seconds before timing out */
  timeoutTimerInit = millis();
  while(m_request != REQUEST_EMPTY) {
    /* Wait until our request is fulfilled */
    if(millis() - timeoutTimerInit > 5000) {
      Serial.println("[WARNING] Request timed out");
      return -2; /* Return timeout error */
    }
  }

  /* return the status */
  return m_requestStatus;
}

/*
 * Setup the data recording
 */
void DataRecorder::recordStartHelper() {
  Serial.println("\nRecord request Acknowledged");
  
  /* If someone pressed the record button while we're already recording */
  if(m_recFlag) {
    Serial.println("\nRecording in progress. Record request ignored");
    m_requestStatus = -1;
    return;
  }
  
  /* Check the buffer, this case should never happen */
  if(m_writeBuffer != NULL) {
    Serial.println("[FATAL] Write buffer is still in use!");
    m_requestStatus = -1;
    return;
  }

  /* Allocate recording space */
  unsigned long nextStartAddr = calcNextStartAddress(getNumRecordings() - 1);
  if(getSpaceLeft() < m_dataSize) {
    Serial.printf("Not enough space left! Need at least %luKB free (%luKB/%luKB Free)\n",
                      m_dataSize/1024, getSpaceLeft()/1024, EEPROM_SIZE/1024l);
    m_requestStatus = -1;
    return;
  }

  m_freeAddress = nextStartAddr;
  m_writeAddress = nextStartAddr;

  /* Setup the new recording address */
  int tries = 1;
  long runTimer = millis();
  while(true) {
    if(millis() - runTimer > 500) {
      if(tries == 3) {
        Serial.printf("[FATAL] Failed to open NVS for writing for \"%s\"!\n", ("DR_" + m_currRecording).c_str());
        m_requestStatus = -3;
        return;
      }

      if (m_pref->begin(("DR_" + m_currRecording).c_str(), false)) {
        Serial.println("Successfully opened NVS for writing");
        break;
      }

      Serial.printf("[WARNING] Failed to open NVS, retrying (%d/%d)...\n", tries, 3);

      runTimer = millis();
      tries++;
    }
  }

  m_pref->putULong("startAddr", nextStartAddr);

  /* Readback just in case */
  nextStartAddr = m_pref->getULong("startAddr", ULONG_MAX);
  m_pref->end();

  if(nextStartAddr == ULONG_MAX) {
    Serial.printf("[FATAL] Failed to write startAddress for %s\n", m_currRecording.c_str());
    m_requestStatus = -3;
    return;
  }

  Serial.printf("Recording startAddr: %lu\n", m_writeAddress);
  
  /* Allocate write buffer */
  m_writeBuffer = (byte*) malloc(BUFFER_SIZE * sizeof(byte));
  Serial.printf("Allocated %dKB of Ram for cached writes\n", BUFFER_SIZE/1024);

  /* Allocate EEPROM */
  Serial.print("Erasing EEPROM...");
  EEPROM.erase(m_writeAddress, m_dataSize);
  Serial.println("Done");
  

  /* Disable the rolling average */
  m_accel->disableRolling();

  m_recTimerInit = esp_timer_get_time() + 1*(1000*1000); // delay start to improve data capture

  /* Setup recording */
  m_recNumSamples = 0;
  m_recFlag = true;

  /* End Request */
  m_requestStatus = 0;

  /* Successfully started recording */
  Serial.println("Recording started");

  
  return;
}

/* Always called on CPU 1, if called anywhere else then you're doing it wrong */
void DataRecorder::recordStopHelper() {
  unsigned long checkMutexTimer;
  unsigned long flushInit;
  

  if(m_requestStatus == -3) {
    return;
  }

  /* Set the request status prior to other status */
  m_requestStatus = 0;

  if(m_currRecording == "") {
    return;
  }
  
  /* Stop recording */
  m_recFlag = false;
  Serial.printf("Finished recording %llu samples\n", m_recNumSamples);
  

  /* Flush the write buffer */
  flushInit = millis();
  Serial.printf("Flushing %lu Writes...", m_freeAddress - m_writeAddress);

  if(m_writeMutex) {
    Serial.println("Waiting for write task to be unlocked...");
    checkMutexTimer = millis();

    /* 
     *  The variable must be checked by an if statement and not by
     * a while loop, otherwise the variable can never be written to
     */
    while(true) {
      if(millis() - checkMutexTimer > 100) {
        if(!m_writeMutex)
          break;
          
        checkMutexTimer = millis();
      }
    }
    
    Serial.println("Done");
  } else {
    writeTask();
  }

  
  /* Sanity check */
  if(m_writeAddress < m_freeAddress) {
    Serial.println("Unable to flush writes!");
    Serial.printf("writeAddr: %lu freeAddr: %lu\n", m_writeAddress, m_freeAddress);
  }
  
  Serial.printf("Done. Operation took %lu ms\n", millis() - flushInit);
  
  /* Free up some ram */
  Serial.printf("Freeing %dKB write buffer\n", BUFFER_SIZE/1024);
  free(m_writeBuffer);
  m_writeBuffer = NULL;
  
  /* Turn LED on */
  vTaskDelete(ledTaskHandle);
  digitalWrite(13, HIGH);
  
  /* Enable AP Beacon */
  //setApBeaconInterval(100);
  setupWifi();

  /* Re-Enable rolling average */
  m_accel->enableRolling();
  
  /* Write Size and save to memory */
  m_pref->begin(("DR_" + m_currRecording).c_str(), false);
  m_pref->putULong("numSamples", (unsigned long) m_recNumSamples);

  /* Check that the data wrote */
  int64_t recNumSamplesBackup = m_recNumSamples;
  m_recNumSamples = (int64_t) m_pref->getULong("numSamples", ULONG_MAX);
  m_pref->end();

  if(m_recNumSamples != recNumSamplesBackup) {
    Serial.printf("[FATAL] Failed to write numSamples for %s\n", m_currRecording.c_str());
    m_requestStatus = -3;
  }
  
  m_recordings += m_currRecording + '\n';
  m_pref->begin("DataRec", false);
  m_pref->putString("RecordingNames", m_recordings);
  m_pref->end();

  Serial.printf("%luKB/%dKB of EEPROM left for data recording\n",
              getSpaceLeft()/1024,
              EEPROM_SIZE/1024);
  
  /* End Request */
  m_currRecording = "";
  
  return;
}

/* 
 *  Recording loop (recFlag == true)
 */
void DataRecorder::run() {
  /* Handle requests */
  if(m_request != REQUEST_EMPTY) {
    if(m_request == REQUEST_START) {
      recordStartHelper();
    } else if (m_request == REQUEST_END) {
      recordStopHelper();
    }

    /* Reset the requests */
    m_request = REQUEST_EMPTY;
    return;
  }

  /* Handle Recording */
  if(m_recFlag == true) {
    // Wait for the start time to occur
    if(m_recTimerInit > esp_timer_get_time()) {
      return;
    }

    m_recTimerDelta = (esp_timer_get_time() - m_recTimerInit) - (m_sampleRateMicros * m_recNumSamples);
    if(m_recTimerDelta > 0) {
      if(m_recNumSamples == 0) {
        /* Turn the LED on */
        xTaskCreatePinnedToCore(ledTask, "ledTask", 2048, NULL, 3, &ledTaskHandle, 1);
      }
      
      if(m_recTimerDelta > m_sampleRateMicros && m_recNumSamples > 0) {
        /* Uh oh, looks like our code is too slow to sample this fast. Better yell at your programmers */
        Serial.printf("[WARNING] Missed Sample %llu! Code lagging by: %llums\n", m_recNumSamples, m_recTimerDelta/1000);
        Serial.printf("[DEBUG] currTime: %llums m_sampleRateMicros: %llu\n", (esp_timer_get_time() - m_recTimerInit)/1000, m_sampleRateMicros);
        m_recNumSamples += 4; /* Attempt to catch up */
        m_writeAddress += 4;
        m_freeAddress += 4;
      }

      // Check for write buffer overflow
      if(m_freeAddress - m_writeAddress > BUFFER_SIZE) {
        Serial.println("FATAL: Write cache overflow!\nStopping Recording...");
        recordStopHelper();
        return;
      }

      if(m_recNumSamples >= m_maxNumSamples){
        Serial.println("Exceeded max number of samples!");
        recordStopHelper(); /* Call the helper since requests are meant for CPU0 */
        return;
      }

      /* Record a single data point */
      int* dPoint = m_accel->readInt();
      saveDataPoint(dPoint);
      free(dPoint);
      
      /* Increase the current sample Iterator */
      m_recNumSamples++;

      /* Debug info */
      if(m_recNumSamples % 1000 == 0) {
        float timeRemaining = (m_dataSize / (3 * 2) - m_recNumSamples) * m_sampleRateMicros * pow(10,-6);
        
        Serial.printf("Logged %llu samples(%.3f seconds left) %lu writes cached (%lu bytes free)\n",
                          m_recNumSamples, timeRemaining, m_freeAddress - m_writeAddress,
                          BUFFER_SIZE - (m_freeAddress - m_writeAddress));
      }
    }
  }
}

/* Pre-calculates the microseconds so we don't do it on the fly */
void DataRecorder::setSampleRate(int samplesPerSecond) {
  m_sampleRateMicros = (long) (((float) 1/(float) samplesPerSecond) * pow(10,6));
  
  printf("Setting sample rate to %llums (%dHz)\n", m_sampleRateMicros, samplesPerSecond);
}

void DataRecorder::setSampleRateMicros(unsigned long us) {
  m_sampleRateMicros = us;
  
  printf("Setting sample rate to %llu\n", m_sampleRateMicros);
}

void DataRecorder::setRecordTime(long seconds) {
  m_maxNumSamples = seconds / (m_sampleRateMicros * pow(10,-6));
  Serial.printf("Set log time to %lu seconds (%lu samples)\n", seconds, m_maxNumSamples);
  /* 
   *  For info purposes 
   *  There are 3 points per sample, and 2 bytes per point
   */
  m_dataSize = EEPROM.align(6 * m_maxNumSamples);

  Serial.printf("RawData is estimated to be %luKB\n", m_dataSize/1024l);
}

int DataRecorder::getRecordTime() {
  return m_maxNumSamples * (m_sampleRateMicros * pow(10,-6));
}

void DataRecorder::recordInfo(String recName) {
  unsigned long startAddr;
  unsigned long numSamples;
  
  m_pref->begin(("DR_" + recName).c_str(), false);
  startAddr = m_pref->getULong("startAddr", ULONG_MAX);
  numSamples = m_pref->getULong("numSamples", ULONG_MAX);
  m_pref->end();

  if(startAddr == ULONG_MAX) {
    Serial.printf("[WARNING] Invalid start address for %s!\n", recName.c_str());
  }
  
  Serial.printf("recName: %s, startAddr: %lu, numSamples: %lu\n", recName.c_str(), startAddr, numSamples);
}


/* Lets us know if we're recording */
bool DataRecorder::isRecording() {
  if(m_recFlag == true) {
    return true;
  }

  return false;
}

/* Return the sample numbers */
unsigned long DataRecorder::getNumSamples(String recName) {
  unsigned long numSamples;
  
  m_pref->begin(("DR_" + recName).c_str(), false);
  numSamples = m_pref->getULong("numSamples", 0);
  m_pref->end();
  
  return numSamples;  
}

float DataRecorder::rawToForce(int raw) {
    /* Function from sparkfun */
    float mappedValue = mapf(raw, 0, 4095, -200, 200);
    return mappedValue;
}

String DataRecorder::getAvgStr() {
  String instantStr = "";
  float* avg = m_accel->getAvg();

  for (int i = 0; i < 3; i++) {
    instantStr += avg[i];
    instantStr += ',';
  }

  for (int i = 0; i < 3; i++) {
    instantStr += rawToForce(avg[i]);
    instantStr += ',';
  }

  instantStr = instantStr.substring(0,instantStr.length() - 1);
  
  free(avg);
  return instantStr;
}

/*
 * Split the integer into two bytes so we can write them to eeprom
 */
void DataRecorder::saveDataPoint(int* dataPoint) {
  for (int i = 0; i < 3; i ++) {
    /* Split */
    byte LSB_byte = dataPoint[i] & 0x00FF;
    byte MSB_byte = dataPoint[i] >> 8;
    
    /* Write to eeprom */
    bufferPush(LSB_byte);
    bufferPush(MSB_byte);
  }
}

/* Reconstructs the 16bit integer from two bytes from eeprom */
int DataRecorder::returnPackedValue(unsigned long address) {
  int temp1 = (int) EEPROM.read(address);
  int temp2 = (int) EEPROM.read(address+1) * (1 << 8);
  int ret = temp2 + temp1;

  if(ret > 4096) {
    Serial.printf("Bad value found at address %lu, value is %d\n", address, ret);
    return -1;
  }

  return ret;
}

/* Calculates time for an index and prints the appropriate float to a string */
String DataRecorder::getTimeStr(long index) {
  float t = ((float) (index))/REC_HZ + ((float) 1)/REC_HZ;

  /* extended float print */
  size_t timeStrSize = 10 * sizeof(char);
  char* timeStr = (char*) malloc(timeStrSize);
  snprintf(timeStr, timeStrSize, "%.3f", t);

  /* Copy the character array into a String class */
  String retStr(timeStr);
  free(timeStr);

  return retStr;
}

String DataRecorder::getRawStr(String recName, unsigned long index) {
  /* Retrieve the data */
  String rawPointStr = "";

  /* Add time */
  rawPointStr += getTimeStr(index) + ",";

  /* Get the address offset */
  m_pref->begin(("DR_" + recName).c_str(), false);
  unsigned long startAddr = m_pref->getULong("startAddr", 0);
  m_pref->end();

  /* Multiply by 6, since the starting address of each set is every 6th byte */
  for(int i = 0; i < 3; i ++) {
    int packedVal = returnPackedValue(startAddr + (index * 6) + (i * 2));
    if (packedVal == -1)
      return "";
      
    rawPointStr += packedVal;
    
    if(i < 2) {
      rawPointStr += ",";
    }
  }
  return(rawPointStr);
}

String DataRecorder::getAxesStr(String recName, unsigned long index) {
  /* Retrieve the data */
  String rawPointStr = "";

  /* Add time */
  rawPointStr += getTimeStr(index) + ",";

  /* Get the address offset */
  m_pref->begin(("DR_" + recName).c_str(), false);
  unsigned long startAddr = m_pref->getULong("startAddr", 0);
  m_pref->end();

  /* Multiply by 6, since the starting address of each set is every 6th byte */
  for(int i = 0; i < 3; i ++) {
    int packedVal = returnPackedValue(startAddr + (index * 6) + (i * 2));
    if (packedVal == -1)
      return "";
      
    rawPointStr += packedVal;
    if(i < 2) {
      rawPointStr += ",";
    }
  }
  
  return(rawPointStr);
}

String DataRecorder::getMagStr(String recName, unsigned long index) {
  /* Retrieve the data */
  String rawPointStr = "";

  /* Add time */
  rawPointStr += getTimeStr(index) + ",";

  /* Get the address offset */
  m_pref->begin(("DR_" + recName).c_str(), false);
  unsigned long startAddr = m_pref->getULong("startAddr", 0);
  m_pref->end();

  /* Multiply by 6, since the starting address of each set is every 6th byte */
  float mag = 0;
  for(int i = 0; i < 3; i ++) {
    int packedVal = returnPackedValue(startAddr + (index * 6) + (i * 2));
    if (packedVal == -1)
      return "";
      
    mag += pow(rawToForce(packedVal), 2);
  }

  mag = sqrt(mag);
  rawPointStr += mag;
  
  return(rawPointStr);
}

/* This operation takes way too long */
String DataRecorder::getMaxMag(String recName) {
  float maxMag;
  long maxMagIndex;
  unsigned long startAddr;
  unsigned long numSamples;
  String rawPointStr = "";

  /* Get the address offset */
  m_pref->begin(("DR_" + recName).c_str(), false);
  startAddr = m_pref->getULong("startAddr", 0);
  numSamples = m_pref->getULong("numSamples", 0);
  m_pref->end();

  /* Find the max magnitude */
  maxMag = 0;
  maxMagIndex = -1;
  for(unsigned long j = 0; j < numSamples; j++) {
    /* Multiply by 6, since the starting address of each set is every 6th byte */
    float currMag = 0;
    for(int i = 0; i < 3; i ++) {
      int packedVal = returnPackedValue(startAddr + (j * 6) + (i * 2));
      if (packedVal == -1)
        return "";
        
      currMag += pow(rawToForce(packedVal), 2);
    }
    
    currMag = sqrt(currMag);

    if(currMag > maxMag) {
      maxMag = currMag;
      maxMagIndex = j;
    }
  }
  /* Add time */
  rawPointStr += getTimeStr(maxMagIndex) + ",";
  rawPointStr += maxMag;

  Serial.printf("Max magnitude for \"%s\": %f", recName.c_str(), maxMag);
  return(rawPointStr);
}

/* Pushes a value to the buffer to write */
void DataRecorder::bufferPush(byte val) {
  //Serial.println(val);
  m_writeBuffer[m_freeAddress % BUFFER_SIZE] = val;
  m_freeAddress++;
}

/* Called from CPU0 to save time */
void DataRecorder::writeTask() {
  if(m_writeMutex) {
    return; /* one of the cores is already writing */
  }

  if(m_writeAddress < m_freeAddress) {
    m_writeMutex = true;
    //Serial.printf("Calling write task on CPU%d\n", xPortGetCoreID());
    if(m_writeBuffer == NULL) {
      Serial.println("[FATAL] Write buffer isn't initialized, resetting iterators");
      m_freeAddress = 0;
      m_writeAddress = 0;
    }
    
    while(m_writeAddress < m_freeAddress) {
      /* Sanity check */
      if(m_freeAddress - m_writeAddress >= BUFFER_SIZE) {
        Serial.println("FATAL: Ran out of buffer");
        m_writeMutex = false;
        recordStop();
        return;
      }

      EEPROM.write(m_writeAddress, m_writeBuffer[m_writeAddress % BUFFER_SIZE]);
      m_writeAddress++;
    }
    m_writeMutex = false;
    //Serial.printf("End write task on CPU%d\n", xPortGetCoreID());
  }
}

void DataRecorder::printAllValues(String recName) {
  Serial.println("t,x,y,z");
  for(unsigned long i = 0; i < getNumSamples(recName); i ++) {
    Serial.println(getAxesStr(recName, i));
  }
  Serial.print("Printed ");
  Serial.print(getNumSamples(recName));
  Serial.print(" values");
}

void DataRecorder::chunkedReadInit(String name, int readType) {
  m_chunkedIter = 0;
  m_chunkedReadType = readType;
  m_chunkedTimer = millis();
  m_chunkedName = name;
  m_chunkedNumSamples = getNumSamples(name);
  Serial.printf("RecName: %s NumSamples: %lu\n", name.c_str(), m_chunkedNumSamples);

  if (m_chunkedReadType == READ_AXES) {
    m_chunkedBuffer = "t,x,y,z\n";
  } else if (m_chunkedReadType == READ_MAGNITUDES) {
    m_chunkedBuffer = "t,g\n";
  } 
}

int DataRecorder::chunkedRead(uint8_t *buffer, size_t maxLen, size_t index) {
  if(m_chunkedIter >= m_chunkedNumSamples) {
    Serial.printf("Took %.2fs to send %luKB\n", ((float) (millis() - m_chunkedTimer))/1000l, index/1024l);
    return 0;
  }

  while(true) {
    if(strlen(m_chunkedBuffer.c_str()) < maxLen) {

      /* End of file, send the rest */
      if(m_chunkedIter >= m_chunkedNumSamples) {
        goto write;
      }
    } else {
write:/* Buffer is full, copy what we need and trim it from the buffer for the next packet */
      String copyBuffer = m_chunkedBuffer.substring(0, maxLen);
      m_chunkedBuffer = m_chunkedBuffer.substring(maxLen);
      Serial.printf("sending %d/%d, sent %dbytes and %lu samples\n",
                          strlen(copyBuffer.c_str()), maxLen, index, m_chunkedIter);
      strcpy((char*) buffer, copyBuffer.c_str());
      
      return strlen(copyBuffer.c_str());
    }

    /* Add a new line to the buffer */
    if(m_chunkedReadType == READ_AXES) {
      String readBuffer = getAxesStr(m_chunkedName, m_chunkedIter++);
      if (readBuffer != "") {
        m_chunkedBuffer += readBuffer + "\n";
      }
    } else if (m_chunkedReadType == READ_MAGNITUDES) {
      String readBuffer = getMagStr(m_chunkedName, m_chunkedIter++);
      if (readBuffer != "") {
        m_chunkedBuffer += readBuffer + "\n";
      }
    }
  }
}
