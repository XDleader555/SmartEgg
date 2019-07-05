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
