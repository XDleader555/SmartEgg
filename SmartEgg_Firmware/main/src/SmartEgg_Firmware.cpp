/*
  SmartEgg_Firmware - Data collection firmware for an ESP32 based egg drop
  Copyright (c) 2019 Andrew Miyaguchi. All rights reserved.

  This program is free software: you can redistribute it a nd/or modify
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


#include "Commands.h"
#include "SmartEgg.h"
#include "ADXL377.h"
#include "SPIFFS.h"
#include "Huzzuh32.h"
#include <driver/adc.h>
#include <stdlib.h>
#include <DNSServer.h>
#include <MiyaSh.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define RELEASE_VER "2.1.3"
#define DNS_PORT 53
#define LED_PIN 13
#define BTN_PIN 0

DNSServer dnsServer;
AsyncWebServer m_server(80);
AsyncWebSocket* m_ws;
AsyncEventSource* m_events;

void webServerTask(void *pvParameters);
void dnsServerTask(void *pvParameter);
void miyaShTask(void *pvParameters);
void rollingAvgTask(void *pvParameter);

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void setupWebServer();

void setup() {
  /* Initialize serial communication at 115200 baud */
  Serial.begin(115200);
  Serial.println("\nSerial Communications Begin...");

  Huzzah32.begin();

  //Serial.println("Reducing clock speed to 80Mhz");
  //setCpuFrequencyMhz(80);
  
  /* Print current temperature */
  Serial.printf("Current Temperature: %.1f°F\n", temperatureRead());
  
  /* Setup the global object */
  SMARTEGG.begin();
  SMARTEGG.dataRec->m_server = &m_server;
  setupWebServer();

  SMARTEGG.dataRec->setupWifi();
  
  /* Setup DNS Server to redirect user to main page */
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  /* Start CPU0 tasks, higher number means higher priority */
  xTaskCreatePinnedToCore(dnsServerTask, "dnsServerTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(rollingAvgTask, "rollingAvgTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(miyaShTask, "miyaguchiShell", 4096, NULL, 1, NULL, 1);

  /* Set the LED HIGH */
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
}

// Shell Task
void miyaShTask(void *pvParameter) {
  MiyaSh sh;
  
  Serial.println("\nBootup Complete. Initializing shell..."); 
  
  sh.registerCmd("record", "SmartEgg", &recordCmd);
  sh.registerCmd("setAPName", "SmartEgg", &setAPNameCmd);
  sh.registerCmd("getAvgStr", "SmartEgg", &getAvgStrCmd);
  sh.registerCmd("calibrate", "SmartEgg", &calibrateCmd);
  sh.registerCmd("routeVRef", "SmartEgg", &routeVRefCmd);
  sh.registerCmd("setVRef", "SmartEgg", &setVRefCmd);
  sh.registerCmd("setVReg", "SmartEgg", &setVRegCmd);
  sh.registerCmd("stopWifi", "SmartEgg", &stopWifiCmd);
  sh.registerCmd("startWifi", "SmartEgg", &startWifiCmd);
  sh.registerCmd("getbattlevel", "SmartEgg", &getBattLevelCmd);
  
  sh.begin();
  
  // Loop Task 
  for(;;) {
    sh.run();
    vTaskDelay(10);
  }
}

void rollingAvgTask(void *pvParameter) {
  /* Loop Task */
  for(;;) {
    SMARTEGG.accel->updateAvg();
    vTaskDelay(100);
  }
}

void dnsServerTask(void *pvParameter) {
  /* Loop Task */
  for(;;) {
    dnsServer.processNextRequest();
    vTaskDelay(100);
  }
}

/* Application CPU (CPU1) */
void loop() {
  SMARTEGG.dataRec->run();
}

void setupWebServer() {
  /* Setup the Filesystem */
  Serial.print("Mounting filesystem... ");
  if(!SPIFFS.begin(true)){
      Serial.println("SPIFFS Mount Failed!");
      Serial.println("Did you remember to upload the SPIFFS partition?");
      return;
  } else {
    Serial.println("Done");
    Serial.printf("%dKB used out of %dKB\n", SPIFFS.usedBytes()/1024, SPIFFS.totalBytes()/1024);
  }

  /* Setup the WebServer */
  m_server.serveStatic("/", SPIFFS, "/html/").setDefaultFile("index.html");
  
  /* Basic Responses */
  m_server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  m_server.on("/functions/recordStart", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordStart/").length());
    Serial.printf("[WEBSERVER] recorsStart - name: \"%s\"\n", recName.c_str());
    
    int resp = SMARTEGG.recordStart(recName);
    request->send(200, "text/plain", String(resp));
  });

  m_server.on("/functions/disableWifi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200);
    SMARTEGG.dataRec->disableWifi();
  });

  m_server.on("/functions/recordStop", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] recordStop");
    int resp = SMARTEGG.recordStop();
    request->send(200, "text/plain", String(resp));
  });

  m_server.on("/functions/recordDeleteAll", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] recordDeleteAll");
    SMARTEGG.dataRec->deleteAllRecordings();
    request->send(200);
  });

  m_server.on("/functions/recordDelete", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordDelete/").length());
    Serial.printf("[WEBSERVER] recordDelete - name: \"%s\"\n", recName.c_str());
    SMARTEGG.dataRec->deleteRecording(recName);
    request->send(200);
  });

  m_server.on("/functions/recordList", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] recordList");
    request->send(200, "text/plain", SMARTEGG.dataRec->getRecordingsList());
  });

  m_server.on("/functions/recordStatus", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] recordStatus");
    String status;
    if(SMARTEGG.dataRec->isRecording())
      status = "1";
    else
      status = "0";
    request->send(200, "text/plain", status);
  });

  m_server.on("/functions/calibrate", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] calibrate");
    request->send(200, "text/plain", SMARTEGG.accel->calibrate());
  });

  m_server.on("/functions/recordGetMaxTime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(SMARTEGG.dataRec->getRecordTime()));
  });

  m_server.on("/functions/recordSetMaxTime", HTTP_GET, [](AsyncWebServerRequest *request){
    String recTime = request->url().substring(String("/functions/recordGetAxes/").length());
    Serial.printf("Recieved record time: %s\n", recTime.c_str());

    SMARTEGG.dataRec->setRecordTime(recTime.toInt());    
    request->send(200);
  });

  m_server.on("/functions/recordSpaceLeft", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(SMARTEGG.dataRec->getSpaceLeft()/1024) + "/" + String(EEPROM_SIZE/1024));
  });

  m_server.on("/functions/getInstant", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, SMARTEGG.dataRec->getAvgStr());
  });

  m_server.on("/functions/recordGetAxes", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordGetAxes/").length());
    Serial.printf("Recieved read request with name\"%s\"\n", recName.c_str());
    SMARTEGG.dataRec->chunkedReadInit(recName, READ_AXES);

    AsyncWebServerResponse *response = request->beginChunkedResponse("text/csv", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      return SMARTEGG.dataRec->chunkedRead(buffer, maxLen, index);
    });

    request->send(response);
  });

  m_server.on("/functions/recordGetMagnitudes", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordGetMagnitudes/").length());
    Serial.printf("[WEBSERVER] recordGetMagnitudes - name: \"%s\"\n", recName.c_str());
    SMARTEGG.dataRec->chunkedReadInit(recName, READ_MAGNITUDES);

    AsyncWebServerResponse *response = request->beginChunkedResponse("text/csv", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      return SMARTEGG.dataRec->chunkedRead(buffer, maxLen, index);
    });

    request->send(response);
  });

  /*
  m_server.on("/functions/recordGetMaxMagnitude", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordGetMaxMagnitude/").length());
    Serial.printf("Recieved read request with name\"%s\"\n", recName.c_str());
    request->send(200, SMARTEGG.dataRec->getMaxMag(recName));
  });
  */

  m_server.on("/functions/setAPName", HTTP_GET, [](AsyncWebServerRequest *request){
    String apName = request->url().substring(String("/functions/setAPName/").length());
    Serial.printf("Requested to set APName to \"%s\"\n", apName.c_str());
    SMARTEGG.dataRec->setAPName(apName);
    
    request->send(200);
  });

  m_server.on("/functions/getAPName", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", SMARTEGG.dataRec->getAPName());
  });

  m_server.on("/functions/getVersion", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", RELEASE_VER);
  });

  m_server.on("/functions/getBatteryLevel", HTTP_GET, [](AsyncWebServerRequest *request){
    float batteryLevel = Huzzah32.readBattery();
    char strLevel[100];

    printf("Battery Level: ");
    if(batteryLevel == -1) {
      request->send(200, "text/plain", "Charging");
    } else {
      sprintf(strLevel, "%.2f\n", batteryLevel);
      request->send(200, "text/plain", strLevel); 
    }
  });

  m_server.on("/functions/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200);

    Serial.println("Going for a restart...");
    delay(100);
    esp_restart();
  });

  m_server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(204);
  });

  m_server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(204);
  });

  m_server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Microsoft Connect Test");
  });

  m_server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("/master-controller.html");
  });
  
  /* Handle 404 */
  m_server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NotFound: http://%s%s\n", request->host().c_str(), request->url().c_str());
    request->send(404);
  });

  //m_server.begin(); //Moved to startwifi
}
