#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Arduino.h"
#include "SmartEgg.h"
#include "SPIFFS.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class WebServer {
  public:
    WebServer();
    void run();
    
  private:
    AsyncWebServer* m_server;
    AsyncWebSocket* m_ws;
    AsyncEventSource* m_events;
    
    static void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
};
#endif /* WEBSERVER_H */
