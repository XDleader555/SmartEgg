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
