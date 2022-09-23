/*
  Critical sections are weird in multicore programming.

  portENTER_CRITICAL/portEXIT_CRITICAL are much more severe than mutex:
    - they prevent the scheduler from running
    - Disable interrupts up to a certain priority level while execution is inside the critical section
  
  Using them is okay, but critical section must be short! What we have here is too long!

  Spinlocks introduce another layer of complexity thanks to the need to work with 2 cores
  When entering a critical section you need to provide a spinlock, which is similar to a mutex,
  as it is a global variable that works as a lock
  
  Unlike a mutex, it forces any core that attempts to grab it when it is already taken
  to wait in a while loop: spinning.

  That is why when task1 makes a delay of 200 ms, the scheduler doesnt give processor to task 0,
  as the scheduler is stopped. NEVER USE A DELAY AS LONG AS THAT IN A CRITICAL SECTION!

  Then task 1 yields processor to task 2

  Core 0 has task 0, which just toggles the LED.
    
*/

#include <Arduino.h>
#include <stdlib.h>

//core definitions: PRO_CPU -> 0, APP_CPU 1
static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// settings
static const TickType_t time_hog = 200; //Time (ms) hogging the CPU in task 1
static const TickType_t task_0_delay = 30; //Time (ms) task 0 blocks itself
static const TickType_t task_1_delay = 100; //Time (ms) task 1 blocks itself

static const int pin_0 = 25;
static const int pin_1 = 26;

static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
/*shared resource that will be used in both cores!*/

//**************************************************************
//Functions

static void hog_delay(uint32_t ms) {
  for(uint32_t i=0; i<ms; i++){
    for(uint32_t j=0; j<40000; j++) 
      asm("nop");    
    }
}

//**************************************************************
//Tasks

//Task in core 0
void doTask0(void*parameters) { 
  
  pinMode(pin_0, OUTPUT);
  
  while(1) {
    portENTER_CRITICAL(&spinlock);
    digitalWrite(pin_0, !(digitalRead(pin_0)));
    portEXIT_CRITICAL(&spinlock);

    // yield processor for a while
    vTaskDelay(task_0_delay / portTICK_PERIOD_MS);

  }
}

//Task in core 1
void doTask1(void*parameters) { 
  
  pinMode(pin_1, OUTPUT);

  while(1) {
    
    portENTER_CRITICAL(&spinlock);
    digitalWrite(pin_1, HIGH);
    hog_delay(time_hog);
    digitalWrite(pin_1, LOW);
    portEXIT_CRITICAL(&spinlock);

    vTaskDelay(task_1_delay / portTICK_PERIOD_MS);
    
    }
}

void setup(){

  Serial.begin(115200);

  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Multicore Demo---");


  xTaskCreatePinnedToCore(doTask0, "Task 0", 2048, NULL, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(doTask1, "Task 1", 2048, NULL, 1, NULL, pro_cpu);


  vTaskDelete(NULL);

}

void loop(){

}