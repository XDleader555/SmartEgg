#include "WebServer.h"

WebServer::WebServer() {
  /* Setup the Filesystem */
  Serial.print("Mounting filesystem... ");
  if(!SPIFFS.begin(true)){
      Serial.println("SPIFFS Mount Failed!");
      return;
  } else {
    Serial.println("Done");
    Serial.printf("%dKB used out of %dKB\n", SPIFFS.usedBytes()/1024, SPIFFS.totalBytes()/1024);
  }

  /* Start the WiFi Access Point */
  WiFi.mode(WIFI_AP);
  WiFi.softAPsetHostname(SMARTEGG.getAPName().c_str());
  WiFi.softAP(SMARTEGG.getAPName().c_str());
  
  Serial.print("Wireless AP Started: ");
  Serial.println(SMARTEGG.getAPName().c_str());
  Serial.print("Local IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the WebServer */
  m_server = new AsyncWebServer(80);
  m_server->serveStatic("/", SPIFFS, "/html/").setDefaultFile("index.html");;

  /* Basic Responses */
  m_server->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  m_server->on("/functions/recordStart", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordStart/").length());
    Serial.printf("[WEBSERVER] recorsStart - name: \"%s\"\n", recName.c_str());
    
    int resp = SMARTEGG.recordStart(recName);
    request->send(200, "text/plain", String(resp));
  });

  m_server->on("/functions/recordStop", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] recordStop");
    int resp = SMARTEGG.recordStop();
    request->send(200, "text/plain", String(resp));
  });

  m_server->on("/functions/recordDeleteAll", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] recordDeleteAll");
    SMARTEGG.dataRec->deleteAllRecordings();
    request->send(200);
  });

  m_server->on("/functions/recordDelete", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordDelete/").length());
    Serial.printf("[WEBSERVER] recordDelete - name: \"%s\"\n", recName.c_str());
    SMARTEGG.dataRec->deleteRecording(recName);
    request->send(200);
  });

  m_server->on("/functions/recordList", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] recordList");
    request->send(200, "text/plain", SMARTEGG.dataRec->getRecordingsList());
  });

  m_server->on("/functions/recordStatus", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] recordStatus");
    String status;
    if(SMARTEGG.dataRec->isRecording())
      status = "1";
    else
      status = "0";
    request->send(200, "text/plain", status);
  });

  m_server->on("/functions/calibrate", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBSERVER] calibrate");
    request->send(200, "text/plain", SMARTEGG.accel->calibrate());
  });

  m_server->on("/functions/recordGetMaxTime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(SMARTEGG.dataRec->getRecordTime()));
  });

  m_server->on("/functions/recordSetMaxTime", HTTP_GET, [](AsyncWebServerRequest *request){
    String recTime = request->url().substring(String("/functions/recordGetAxes/").length());
    Serial.printf("Recieved record time: %s\n", recTime.c_str());

    SMARTEGG.dataRec->setRecordTime(recTime.toInt());    
    request->send(200);
  });

  m_server->on("/functions/recordSpaceLeft", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(SMARTEGG.dataRec->getSpaceLeft()/1024) + "/" + String(EEPROM_SIZE/1024));
  });


  m_server->on("/functions/getInstant", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, SMARTEGG.dataRec->getAvgStr());
  });

  m_server->on("/functions/recordGetAxes", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordGetAxes/").length());
    Serial.printf("Recieved read request with name\"%s\"\n", recName.c_str());
    SMARTEGG.dataRec->chunkedReadInit(recName, READ_AXES);

    AsyncWebServerResponse *response = request->beginChunkedResponse("text/csv", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      return SMARTEGG.dataRec->chunkedRead(buffer, maxLen, index);
    });

    request->send(response);
  });

  m_server->on("/functions/recordGetMagnitudes", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordGetMagnitudes/").length());
    Serial.printf("[WEBSERVER] recordGetMagnitudes - name: \"%s\"\n", recName.c_str());
    SMARTEGG.dataRec->chunkedReadInit(recName, READ_MAGNITUDES);

    AsyncWebServerResponse *response = request->beginChunkedResponse("text/csv", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      return SMARTEGG.dataRec->chunkedRead(buffer, maxLen, index);
    });

    request->send(response);
  });

  /*
  m_server->on("/functions/recordGetMaxMagnitude", HTTP_GET, [](AsyncWebServerRequest *request){
    String recName = request->url().substring(String("/functions/recordGetMaxMagnitude/").length());
    Serial.printf("Recieved read request with name\"%s\"\n", recName.c_str());
    request->send(200, SMARTEGG.dataRec->getMaxMag(recName));
  });
  */

  m_server->on("/functions/setAPName", HTTP_GET, [](AsyncWebServerRequest *request){
    String apName = request->url().substring(String("/functions/setAPName/").length());
    Serial.printf("Requested to set APName to \"%s\"\n", apName.c_str());
    SMARTEGG.setAPName(apName);
    
    request->send(200);
  });

  m_server->on("/functions/getAPName", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, SMARTEGG.getAPName());
  });

  m_server->on("/functions/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200);

    Serial.println("Going for a restart...");
    delay(100);
    esp_restart();
  });

  /* Handle 404 */
  m_server->onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NotFound: http://%s%s\n", request->host().c_str(), request->url().c_str());
    request->send(404);
  });

  m_server->begin();
}
