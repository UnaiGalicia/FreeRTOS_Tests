/*
 * creates two rtos tasks, each toggles a led
 * and then goes to a blocked state, to sleep
*/

#include <Arduino.h>
#include <iostream>

#if CONFIG_FREERTOS_UNICORE           //configure to use one core
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const int led_pin = 2;       //esp32 led pins
static const int led_pin2 = 25;


void toggleLED(void *parameter){
  while(1){
    digitalWrite(led_pin, HIGH);
    digitalWrite(led_pin2, LOW);
    std::cout << "Hi from "               
       << pcTaskGetName(NULL) << "\n";      //print task name
    vTaskDelay(1000/portTICK_PERIOD_MS);    //block FreeRTOS task
  }
}

void printing(void *parameter){
  while(1){
    digitalWrite(led_pin, LOW);
    digitalWrite(led_pin2, HIGH);
    std::cout << "Hi from " 
        << pcTaskGetName(NULL) << "\n";   //print task name
    vTaskDelay(1000/portTICK_PERIOD_MS);  //block FreeRTOS task
  }
}

void setup() {                //acts like other task with priority 1
  pinMode(led_pin, OUTPUT);
  pinMode(led_pin2, OUTPUT);

  xTaskCreatePinnedToCore(  //create a Task
            toggleLED,        //Function to be called
            "Toggle LED",     //Name of the task
            1024,             //stack size (bytes in ESP32)
            NULL,             //Parameter to pass to function
            1,                //Task priority (0 LOWEST, 25 (configMAX_PRIORITIES - 1) HIGHEST)
            NULL,             //Task handle (?)
            app_cpu);         //Run on one core for demo
  
  xTaskCreatePinnedToCore(  //create a Task
            printing,         //Function to be called
            "Print shit",     //Name of the task
            1024,             //stack size (bytes in ESP32)
            NULL,             //Parameter to pass to function
            1,                //Task priority (0 LOWEST, 25 (configMAX_PRIORITIES - 1) HIGHEST)
            NULL,             //Task handle (?)
            app_cpu);         //Run on one core for demo

}

void loop() {
  
}