#include "ADXL377.h"

ADXL377::ADXL377(adc1_channel_t xPin, adc1_channel_t yPin, adc1_channel_t zPin, int stPin, int vccPin, int gndPin) {
  Serial.println("Initializing Accelerometer...");
  m_rollingAvgIter = 0;
  m_calibrating = false;
  m_allowRolling = true;
  m_manualCal = true;

  m_pins[0] = xPin;
  m_pins[1] = yPin;
  m_pins[2] = zPin;

  m_stPin = stPin;
  m_vccPin = vccPin;
  m_gndPin = gndPin;

  if(m_stPin != -1) {
    pinMode(m_stPin, OUTPUT);
    digitalWrite(m_stPin, LOW);
  }

  if(m_vccPin != -1) {
    pinMode(m_vccPin, OUTPUT);
    digitalWrite(m_vccPin, HIGH);
  }
  
  if(m_gndPin != -1) {
    pinMode(m_gndPin, OUTPUT);
    digitalWrite(m_gndPin, LOW);
  }

  /* Wait for the chip to turn on */
  delay(5);

  /* Grab the calibration data if it exists */
  m_pref = new Preferences();
  m_pref->begin("ADXL377", true);
  m_offsets[0] = m_pref->getFloat("calX", 0);
  m_offsets[1] = m_pref->getFloat("calY", 0);
  m_offsets[2] = m_pref->getFloat("calZ", 0);
  m_vRef = m_pref->getUInt("vRef", 1100);
  m_vReg = m_pref->getUInt("vReg", 3300);
  m_pref->end();

  /* Check if the device is calibrated from the factory */
  bool efuse_tp_present = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP);
  bool efuse_vref_present = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF );

  Serial.printf("Factory Two Point Cal: %s\nFactory vRef: %s\n", efuse_tp_present ? "true" : "false", efuse_vref_present ? "true" : "false");

  if(efuse_tp_present || efuse_vref_present) {
    Serial.println("Factory calibration exists, disabling manual calibration.");

    /* Use nominal values */
    m_vRef = 1100;
    m_vReg = 3300;
    m_manualCal = false;
  }
  
  Serial.printf("Loaded ADXL377 Calibration Data x=%f, y=%f, z=%f, vRef=%d vReg=%d\n",
                          m_offsets[0], m_offsets[1], m_offsets[2], m_vRef, m_vReg);

  /* Setup ADC */
  adc1_config_width(ADC_WIDTH_12Bit);
  for(adc1_channel_t e:m_pins)
    adc1_config_channel_atten(e, ADC_ATTEN_11db);

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, m_vRef, &m_characteristics);
  
  /* Initialize the rolling average buffer */
  for(int i = 0; i < ROLLING_AVG_SIZE; i++)
    m_rollingAvgBuffer[i] = (float*) malloc(3 * sizeof(float));


  /* Self test */
  if(m_stPin != -1) {
    float* st_tare = read();
    digitalWrite(m_stPin, HIGH);
    delay(1);
    float* st = read();
    digitalWrite(m_stPin, LOW);
    
    Serial.printf("Self Test x=%f, y=%f, z=%f\n",
      mapf(st[0] - st_tare[0], 0, 4095, -200, 200),
      mapf(st[1] - st_tare[1], 0, 4095, -200, 200),
      mapf(st[2] - st_tare[2], 0, 4095, -200, 200)
    );
    
    free(st_tare);
    free(st);
  }
}

void ADXL377::setVReg(int vReg) {
  if(!m_manualCal) {
    Serial.println("Unable to set vReg. Factory calibration exists.");
    return;
  }
  
  Serial.printf("Adjusting vReg cal to %dmv", vReg);
  m_pref->begin("ADXL377", false);
  m_pref->putUInt("vReg", vReg);
  m_pref->end();

  m_vReg = vReg;
}

void ADXL377::setVRef(int vRef) {
  if(!m_manualCal) {
    Serial.println("Unable to set vRef. Factory calibration exists.");
    return;
  }
  
  Serial.printf("Adjusting vRef cal to %dmv", vRef);
  m_pref->begin("ADXL377", false);
  m_pref->putUInt("vRef", vRef);
  m_pref->end();

  m_vRef = vRef;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, m_vRef, &m_characteristics);
}

void ADXL377::run() {
  if(!m_calibrating && m_allowRolling) {
    updateAvg();
  }
}

uint32_t* ADXL377::readRaw() {
  uint32_t* rawdata = (uint32_t*) malloc(3 * sizeof(uint32_t));
  
  for (int i = 0; i < 3; i++)
    esp_adc_cal_get_voltage((adc_channel_t) m_pins[i], &m_characteristics, &(rawdata[i]));

  return rawdata;
}

float* ADXL377::read() {
    float* data = (float*) malloc(3 * sizeof(float));
    uint32_t* rawdata = readRaw();
    
    for (int i = 0; i < 3; i++) {
      data[i] = mapf(rawdata[i], 0, m_vReg, 0, 4095);
      data[i] = round(data[i] - m_offsets[i]);
    }

    free(rawdata);

    return data;
}

int* ADXL377::readInt() {
  int* data = (int*) malloc(3 * sizeof(int));
  float* dataf = read();

  for(int i =0; i < 3; i ++) {
    data[i] = round(dataf[i]);
  }

  free(dataf);

  return data;
}

void ADXL377::enableRolling() {
  m_allowRolling = true;
}

void ADXL377::disableRolling() {
  m_allowRolling = false;
}

/* Calibration function, updates the offsets to 0G,0G,1G */
String ADXL377::calibrate() {
  Serial.print("Calibrating... DONT MOVE THE DEVICE!");
  m_calibrating = true;

  /* Delete the offsets */
  for(int i = 0; i < 3; i++)
    m_offsets[i] = 0;

  float* calData = getCalData();
  setOffsets(calData);
  
  Serial.printf(" Done! x=%f, y=%f, z=%f\n", calData[0], calData[1], calData[2]);
  String ret = "";
  for(int i = 0; i < 3; i ++)
    ret += String(calData[i]) + ",";

  ret = ret.substring(0, ret.length() - 1);
  free(calData);
  
  m_calibrating = false;

  return ret;
}

float* ADXL377::getCalData() {
  /* Reuse the rolling average since I'm lazy */
  long waitTimer = millis();
  int numPoints = 0;
  while(numPoints < ROLLING_AVG_SIZE) {
    if(millis() - waitTimer > 10) {
      updateAvg();
      waitTimer = millis();
      numPoints++;
    }
  }
  
  float* calData = getAvg();

  /* offset the calData */
  for(int i = 0; i < 3; i++)
    calData[i] = calData[i] - 2048;

  /* Special Y offset */
  calData[1] = calData[1] - (mapf(1, -200, 200, 0, 4095) - 2047.5);

  return calData;
}

/* set the calibration offsets */
void ADXL377::setOffsets(float* calData) {
  memcpy(m_offsets, calData, 3 * sizeof(float));

  m_pref->begin("ADXL377", false);
  m_pref->putFloat("calX", calData[0]);
  m_pref->putFloat("calY", calData[1]);
  m_pref->putFloat("calZ", calData[2]);
  m_pref->end(); 
}

/* Returns the rolling average */
float* ADXL377::getAvg() {
  float* avg = (float*) malloc(3 * sizeof(float));
  float* sum = (float*) malloc(3 * sizeof(float));

  for(int i = 0; i < 3; i ++)
    sum[i] = 0;
  
  /* Add up each measurement */
  for(int r = 0; r < ROLLING_AVG_SIZE; r ++)
    for(int i = 0; i < 3; i ++)
      sum[i] += m_rollingAvgBuffer[r][i];

  /* Divide by the number of measurements */
  for(int i = 0; i < 3; i ++)
    avg[i] = sum[i]/ROLLING_AVG_SIZE;

  free(sum);
  return avg;
}

/* Update the rolling average */
void ADXL377::updateAvg() {
  /* Protect the recording session */
  if (!m_allowRolling)
    return;

  /* Circle back */
  if (m_rollingAvgIter >= ROLLING_AVG_SIZE)
    m_rollingAvgIter = 0;

  /* Grab data */
  float* singleRead = read();
  memcpy(m_rollingAvgBuffer[m_rollingAvgIter], singleRead, 3 * sizeof(float));
  free(singleRead);
  
  m_rollingAvgIter++;
}
