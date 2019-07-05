#include <driver/adc.h>
#include <stdlib.h>
#include "SmartEgg.h"
#include "ADXL377.h"
#include "DataRecorder.h"
#include "Terminal.h"
#include "Webserver.h"

WebServer* webServ;
Terminal* term; 

void webServerTask(void *pvParameters);
void termTask(void *pvParameters);
void rollingAvgTask(void *pvParameter);

void setup() {
  /* Initialize serial communication at 115200 baud */
  Serial.begin(115200);
  Serial.println("\nSerial Communications Begin...");

  /* Print current temperature */
  //Serial.printf("Current Temperature: %.1f°F\n", temperatureRead());


  /* Setup the global object */
  SMARTEGG.begin();
  webServ = new WebServer();
  
  /* Start CPU0 tasks, high number means they wont get interrupted */
  xTaskCreatePinnedToCore(rollingAvgTask, "rollingAvgTask", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(termTask, "terminalEmulator", 2048, NULL, 4, NULL, 0);  
  

  /* Set the LED low */
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
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

/* Application CPU (CPU1) */
void loop() {
  SMARTEGG.dataRec->run();
}
