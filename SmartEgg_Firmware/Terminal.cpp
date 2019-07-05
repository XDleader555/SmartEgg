#include "Terminal.h"

Terminal::Terminal() {
  m_directory = "/";
  
  help();
  printPrompt();
}


/*
* This function handles the serial protocol, allowing for a rich
* user interface when handling the egg through IO
*/
void Terminal::run() {
  if(Serial.available()) {
    /* Recieve the serial input, and trim it of whitespace */
    String command = Serial.readString();
    command.trim();
    Serial.println(command);
  
    /* Get the number of parameters, if any */
    int numParam = 0;
    for(int i = 0; i < command.length(); i++) {
      /* Ignore Whitespace */
      if(command.charAt(i) == ' ' && command.charAt(i+1) != ' ') {
        numParam ++;
      }
    }

    /* Build an array of paramters for commands to use */
    String parameters[numParam];
    int paramIterator = 0;
    for(int i = 0; i < command.length(); i ++) {
      if(command.charAt(i) == ' ' && command.charAt(i+1) != ' ') {
        parameters[paramIterator++] = command.substring(i+1, command.indexOf(' ', i+1));
      }
    }

    /* Trim the command of any paramters */
    if(numParam > 0) {
      command = command.substring(0,command.indexOf(' '));
    }

    
    /* Debug
    Serial.print("command: ");
    Serial.println(command);
    Serial.print("parameters: ");
    for(int i = 0; i < numParam; i ++) {
      Serial.print(parameters[i]);
      if(i < numParam -1) {
        Serial.print(", ");
      }
    }
    Serial.println();
    */
    
    switch(getCommandIndex(command)) {
      case 0: /* help */
        /* Print the help */
        if(numParam > 0) {
          help(parameters[0]);
        } else {
          Serial.println("For help on a specific command, use # help {commandName}");
          help();
        }
        break;
      case 1: /* record */
        if(numParam > 0) {
          if(parameters[0] == "stop") {
            SMARTEGG.recordStop();
          } else if(parameters[0] == "list") {
            Serial.printf("%luKB/%dKB of EEPROM left for data recording\n%s",
                        SMARTEGG.dataRec->getSpaceLeft()/1024,
                        EEPROM_SIZE/1024,
                        SMARTEGG.dataRec->getRecordingsList().c_str());
          } else if(parameters[0] == "setTime") {
            if(numParam > 1)
              SMARTEGG.dataRec->setRecordTime(filterInt(parameters[1]));
            else
              Serial.println("Please enter the max recording time in seconds!");
          } else if(parameters[0] == "start") {
            if(numParam > 1)
              SMARTEGG.recordStart(parameters[1]);
            else
              Serial.println("Please enter a name for your recording!");
          } else if(parameters[0] == "delete"){
            if(numParam > 1)
              if(parameters[1] == "--all")
                SMARTEGG.dataRec->deleteAllRecordings();
              else
                SMARTEGG.dataRec->deleteRecording(parameters[1]);
            else
              Serial.println("Please enter a recording to delete!");
          } else if(parameters[0] == "dump") {
            if(numParam > 1)
              SMARTEGG.dataRec->printAllValues(parameters[1]);
            else
              Serial.println("Please specify a recording");
          } else if(parameters[0] == "info") {
            if(numParam > 1)
              SMARTEGG.dataRec->recordInfo(parameters[1]);
            else
              Serial.println("Please specify a recording");
          } else {
            help("record");
          }
        } else {
          help("record");
        }
        break;
      case 2: /* setAPName */
        if(numParam > 0) {
          /* Rebuild the name string */
          String apName = "";
          for(int n = 0; n < numParam; n++)
            apName += parameters[n] + ' ';
          apName.trim();
          SMARTEGG.setAPName(apName);
        } else {
          Serial.println("Please specify a WiFi access point name!");
        }
        break;
      case 3: /* free */
        Serial.print("Ram Left: ");
        if(numParam > 0 && parameters[0] == "-h") {
          Serial.print(esp_get_free_heap_size()/1024);
          Serial.println(" Kilobytes");
        } else {
          Serial.print(esp_get_free_heap_size());
          Serial.println(" bytes");
        }
        break;
      case 4: /* reboot */
        Serial.println("Going for a restart...");
        delay(100);
        esp_restart();
        break;
      case 5: /* ls */
        if(numParam > 0) {
          ls(parameters[0]);
        } else {
          ls(m_directory);
        }
        break;
      case 6: /* cat */ {
        if(numParam > 0) {
          cat(parameters[0]);
        } else {
          help("cat");
        }
        break;
      }
      case 7: /* getAvgStr */
        Serial.println(SMARTEGG.dataRec->getAvgStr());
        break;
      case 8: /* calibrate */
        SMARTEGG.accel->calibrate();
        break; 
      case 9: /* getTemp */
        //Serial.printf("Current Temperature: %.1f°F\n", temperatureRead());
        Serial.println("Temperature read disabled due to conflict with accelerometer");
        break;
      case 10: /* routeVRef */
        if (adc2_vref_to_gpio(GPIO_NUM_25) == ESP_OK){
            SMARTEGG.accel->disableRolling();
            printf("v_ref routed to GPIO 25 and rolling avg disabled. Reboot to remove route.\n");
        } else {
            printf("Failed to route v_ref\n");
        }
        break;
      case 11: /* setVRef */
        if(numParam > 0)
          SMARTEGG.accel->setVRef(parameters[0].toInt());
        else
          Serial.printf("No parameter for Voltage (mv)!");
        break;
      case 12: /* setVReg */
        if(numParam > 0)
          SMARTEGG.accel->setVReg(parameters[0].toInt());
        else
          Serial.printf("No parameter for Voltage (mv)!");
        break;        
      default:
        Serial.print("Command not found. ");
        help();
    }
    printPrompt();
  }
}

int Terminal::filterInt(String param) {
  for(int i = 0; i < param.length(); i++) {
    if(param.charAt(i) > 57 || param.charAt(i) < 48) {
      if(i < param.length() - 1) {
        param = param.substring(0,i) + param.substring(i+1);
      } else {
        param = param.substring(0,i);
      }
    }
  }
  if(param.length() == 0) {
    return NULL;
  }
  
  return param.toInt();
}

int Terminal::getCommandIndex(String command) {
  int commandIndex = -1;
  for(int i = 0; i < sizeof(availCommands)/sizeof(String); i ++) {
    if(command == availCommands[i]) {
      commandIndex = i;
      break;
    }
  }
  return commandIndex;  
}

/* If you actually want to document stuff lol */
void Terminal::help(String command) {
  if(getCommandIndex(command) != -1) {
    Serial.print("Help page for \"");
    Serial.print(command);
    Serial.println("\"");
    switch(getCommandIndex(command)) {
      case 1:
        Serial.println("Usage: record {start/stop/list/dump/delete/setTime}");
        Serial.println("start: Begins a recording with specified name");
        Serial.println("stop: Ends a currently running recording");
        Serial.println("list: Displays the names of currently saved recordings");
        Serial.println("dump: Prints the saved data to the console");
        Serial.println("delete: Deletes a recording");
        Serial.println("setTime: Set the max recording time");
        Serial.println("\nExample: record start myRecordingName");
        break;
      case 3:
        Serial.println("Usage: getCommandIndex {dataSetIndex}");
        Serial.println("Returns the x, y, z, time for the selected index");
        break;
    }
  } else {
    Serial.print("Can't find help page for \"");
    Serial.print(command);
    Serial.println("\"");
    help();
  }
}

void Terminal::help() {
  Serial.println("List of availiable Commands:");
  for(int i = 0; i < sizeof(availCommands)/sizeof(String); i ++) {
    Serial.print("\t- ");
    Serial.println(availCommands[i]);
  }
}

void Terminal::printPrompt() {
  Serial.print(HOSTNAME);
  Serial.print(m_directory);
  Serial.print("# ");
}

/* Lists files in the specified directory */
void Terminal::ls(String dirName) {
  fs::FS fs = SPIFFS;
  File dir = fs.open(dirName);

  /* Check if the directory exists */
  if(!dir){
      Serial.print("ls: cannot access ");
      Serial.print(dir);
      Serial.println(": No such file or directory");
      return;
  }
  
  /* Print out the file names */
  File file = dir.openNextFile();
  String fileName;
  while(file) {
    Serial.print(file.size());
    Serial.print("\t");
    Serial.println(file.name());
    file = dir.openNextFile();
  }
}

/* Prints out a file from the filesystem */
void Terminal::cat(String path) {
  fs::FS fs = SPIFFS;
  File file = fs.open(path);

  /* Check if the directory exists */
  if(!file){
      Serial.print("cat: ");
      Serial.print(path);
      Serial.println(": No such file or directory");
      return;
  }

  while(file.available()) {
    Serial.print(char(file.read()));
  }

  file.close();
}

