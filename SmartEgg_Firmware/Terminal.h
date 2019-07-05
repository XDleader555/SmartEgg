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

#ifndef TERMINAL_H
#define TERMINAL_H

#include "Arduino.h"
#include "SmartEgg.h"
#include "ADXL377.h"
#include "DataRecorder.h"
#include "SPIFFS.h"
#include <esp_err.h>

/* Make our console look nice */
#define HOSTNAME "\nadmin@SmartEgg:"

class Terminal {
  public:
    Terminal();
    void run();
    
  private:
    /* varibles */
    String availCommands[13] = {"help",
                               "record",
                               "setAPName",
                               "free",
                               "reboot",
                               "ls",
                               "cat",
                               "getAvgStr",
                               "calibrate",
                               "getTemp",
                               "routeVRef",
                               "setVRef",
                               "setVReg"};  /* Commands for use with the serial console */
    String m_directory;

    /* functions */
    int getCommandIndex(String command);
    int filterInt(String param);
    void help();
    void help(String command);
    void printPrompt();

    /* ported commands */
    void ls(String dir);
    void cat(String path);
};
#endif /* TERMINAL_H */
