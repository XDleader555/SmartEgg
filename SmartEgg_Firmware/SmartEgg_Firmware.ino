/*
  SmartEgg_Firmware - Data collection firmware for an ESP32 based egg drop
  Copyright (c) 2019 Andrew Miyaguchi. All rights reserved.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <driver/adc.h>
#include <stdlib.h>
#include <DNSServer.h>
#include "SmartEgg.h"
#include "ADXL377.h"
#include "DataRecorder.h"
#include "Terminal.h"
#include "SmartEggWebServer.h"

#define DNS_PORT 53
#define LED_PIN 5
#define BTN_PIN 0

DNSServer dnsServer;
SmartEggWebServer* webServ;
Terminal* term; 

void webServerTask(void *pvParameters);
void dnsServerTask(void *pvParameter);
void termTask(void *pvParameters);
void rollingAvgTask(void *pvParameter);
void btnTask(void *pvParameter);


TaskHandle_t btnTaskHandle;
void IRAM_ATTR btnFallingRoutine();
volatile long btnTimer;

void setup() {
  /* Initialize serial communication at 115200 baud */
  Serial.begin(115200);
  Serial.println("\nSerial Communications Begin...");

  /* Print current temperature */
  Serial.printf("Current Temperature: %.1f°F\n", temperatureRead());
  
  /* Setup the global object */
  SMARTEGG.begin();
  webServ = new SmartEggWebServer();
  
  /* Setup DNS Server to redirect user to main page */
  dnsServer.start(DNS_PORT, "ngcsmart.egg", WiFi.softAPIP());
  
  /* Start CPU0 tasks, higher number means higher priority */
  //xTaskCreatePinnedToCore(dnsServerTask, "dnsServerTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(rollingAvgTask, "rollingAvgTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(termTask, "terminalEmulator", 4096, NULL, 4, NULL, 0);  

  /* Set the LED low */
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  /* Setup the button */
  pinMode(BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), btnFallingRoutine, FALLING);
}

void IRAM_ATTR btnFallingRoutine() {
  /* debounce */
  if(millis() - btnTimer < 50)
    return;
    
  btnTimer = millis();
  digitalWrite(LED_PIN, HIGH);
  xTaskCreatePinnedToCore(btnTask, "btnTask", 4096, NULL, 1, &btnTaskHandle, 0);
}

/* Button Task */
void btnTask(void *pvParameter) {
  boolean settingsBool = false;
  boolean ledBool = false;
  
  for(;;) {
    /* debounce */
    if(millis() - btnTimer < 50)
      continue;

    /* Flash LED so user knows the button was pressed */
    if(!ledBool && millis() - btnTimer > 100) {
      digitalWrite(LED_PIN, LOW);
      ledBool = true;
    }

    if(!settingsBool && millis() - btnTimer > 5000) {
      Serial.println("Settings Erased");
      digitalWrite(LED_PIN, HIGH);
      settingsBool = true;
    }
  
    if(digitalRead(BTN_PIN) == HIGH) {
      Serial.printf("Button held for %lums\n",millis() - btnTimer);

      digitalWrite(LED_PIN, LOW);

      /* Remove the task from the list and stop */
      vTaskDelete(btnTaskHandle);
      vTaskSuspend(NULL);
    }

    vTaskDelay(10);
  }
}

/* Terminal Task */
void termTask(void *pvParameter) {
  /* Setup the terminal */
  Serial.println("\nBootup Complete. Initializing Terminal...");
  term = new Terminal();
  
  /* Loop Task */
  for(;;) {
    term->run();
    vTaskDelay(500);
  }
}

void rollingAvgTask(void *pvParameter) {
  /* Loop Task */
  for(;;) {
    SMARTEGG.accel->updateAvg();
    vTaskDelay(50);
  }
}

void dnsServerTask(void *pvParameter) {
  /* Loop Task */
  for(;;) {
    dnsServer.processNextRequest();
    vTaskDelay(500);
  }
}

/* Application CPU (CPU1) */
void loop() {
  SMARTEGG.dataRec->run();
}