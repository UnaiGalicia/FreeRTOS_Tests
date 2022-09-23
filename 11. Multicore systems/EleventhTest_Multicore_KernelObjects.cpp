/*
    As long as we have shared memory, like we do with the ESP32,
    Kernel objects should work the same. That means we shouldn't do anything special
    to run queues, mutexes, semaphores, etc.
    
    We will test that here: a semaphore will be created
    and used from two different tasks that run in two different cores
    
    */

#include <Arduino.h>
#include <stdlib.h>

//core definitions: PRO_CPU -> 0, APP_CPU 1
static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// settings
static const uint32_t task_0_delay = 500; //Time (ms) task 0 blocks itself

static const int pin = 25;

static SemaphoreHandle_t bin_sem;
/*shared resource that will be used in both cores!*/

//**************************************************************
//Tasks

//Task in core 0
void doTask0(void*parameters) { 
  
  pinMode(pin, OUTPUT);
  
  while(1) {

    xSemaphoreGive(bin_sem);

    // yield processor for a while
    vTaskDelay(task_0_delay / portTICK_PERIOD_MS);

  }
}

//Task in core 1
void doTask1(void*parameters) { 
  
  while(1) {
    xSemaphoreTake(bin_sem, portMAX_DELAY);

    digitalWrite(pin, !(digitalRead(pin)));
    
    }
}

void setup(){

  Serial.begin(115200);

  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Multicore Demo---");

  bin_sem = xSemaphoreCreateBinary();

  xTaskCreatePinnedToCore(doTask0, "Task 0", 2048, NULL, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(doTask1, "Task 1", 2048, NULL, 1, NULL, pro_cpu);


  vTaskDelete(NULL);

}

void loop(){

}