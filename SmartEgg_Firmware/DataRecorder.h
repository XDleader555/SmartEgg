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

#ifndef DATARECORDER_H
#define DATARECORDER_H

#include "Arduino.h"
#include <MiyaFunctions.h>
#include "EEPROM32.h"
#include "ADXL377.h"
#include <math.h>
#include <Preferences.h>

#define RECORD_TIME 60      /* record time */
#define NUM_SAMPLES 1       /* how many samples to take when recording one value, usually 1 is enough */
#define REC_HZ 500          /* Samples per second */
#define EEPROM_SIZE 512000  /* Size of the EEPROM */
#define BUFFER_SIZE 5120    /* Size of the write buffer */

/* 
 * Handle requests for handling tasks on the application CPU
 * status of 0 means no error
 * status of -1 means general error
 * status of -2 means timeout error
 */
#define REQUEST_START 1
#define REQUEST_END -1
#define REQUEST_EMPTY 0

#define READ_AXES 1
#define READ_MAGNITUDES 2

class DataRecorder {
  public:
    DataRecorder(ADXL377* accel);
    void run();
    void printAllValues(String recName);
    void setRecordTime(long seconds);
    void writeTask();
    void deleteRecording(String recordingName);
    void deleteAllRecordings();
    bool isRecording();
    int recordStart(String recordingName);
    int recordStop();
    int getRecordTime();
    unsigned long getNumSamples(String recName);
    void recordInfo(String recName);
    String getAvgStr();
    String getTimeStr(long index);
    String getRawStr(String recName, unsigned long index);
    String getAxesStr(String recName, unsigned long index);
    String getMagStr(String recName, unsigned long index);
    String getRecordingsList();
    unsigned long getSpaceLeft();
    String getMaxMag(String recName);

  private:
    ADXL377* m_accel;                   /* Accelerometer reference */
    byte* m_writeBuffer;                /* Circular Buffer */
    bool m_recFlag;                     /* Currently recording? */
    bool m_writeMutex;                  /* Locks the requests */
    int m_request, m_requestStatus;
    long m_recTimerDelta;               /* Temporary varible, delta between records */
    unsigned long m_recTimerInit;       /* How often we record */
    unsigned long m_recNumSamples;      /* What sample are we on, to ensure accuracy of the timer - has to be long*/
    unsigned long m_sampleRateMicros;   /* samples Per Micros - Has to be long, otherwise math will overflow*/
    unsigned long m_maxNumSamples;      /* Max numbers of samples to take */
    unsigned long m_dataSize;           /* Size of the file in bytes */
    unsigned long m_writeAddress;       /* Current Buffer address we're writing to EERPROM*/
    unsigned long m_freeAddress;        /* Free EEPROM address */
    Preferences* m_pref;                 /* Preferences variable */

    int request(int requestType);
    int returnPackedValue(unsigned long address);
    float rawToForce(int raw);
    void recordStartHelper();
    void recordStopHelper();
    void setSampleRate(int samplesPerSecond);
    void bufferPush(byte val);
    void saveDataPoint(int* dataPoint);

    /* Saved recordings */
    String m_currRecording;
    String m_recordings;
    int getNumRecordings();
    String getRecordingName(int index);
    void defragment();
    bool recordingExists(String recName);
    unsigned long calcNextStartAddress(int fromRecIndex);

    /* Chunked Functions */
    unsigned long m_chunkedIter;
    unsigned long m_chunkedNumSamples;
    unsigned long m_chunkedTimer;
    int m_chunkedReadType;
    String m_chunkedName;
    String m_chunkedBuffer;
    
  public:
    void chunkedReadInit(String name, int readType);
    int chunkedRead(uint8_t *buffer, size_t maxLen, size_t index);
};

#endif /* DATARECORDER_H */
