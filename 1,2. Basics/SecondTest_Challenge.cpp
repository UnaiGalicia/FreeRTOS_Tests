/**
 * FreeRTOS LED Demo
 * 
 * One task flashes an LED at a rate specified by a value set in another task.
 * 
 * Date: December 4, 2020
 * Author: Shawn Hymel
 * License: 0BSD
 */

// Needed for atoi()
#include <stdlib.h>
#include <Arduino.h>
#include <getit.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Pins
static const int led_pin = 25;

static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;

//*****************************************************************************
// Tasks

// Task: Blink LED at rate set by global variable
void toggleLED(void *parameter) {
  while (1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay( *(uint16_t*)parameter / portTICK_PERIOD_MS);        //VOID POINTER DEREFERENCING REQUIRES *(*TYPE) FORMAT
    digitalWrite(led_pin, LOW);
    vTaskDelay( *(uint16_t*)parameter / portTICK_PERIOD_MS);
  }
}

// Task: Read from serial terminal
// Feel free to use Serial.readString() or Serial.parseInt(). I'm going to show
// it with atoi() in case you're doing this in a non-Arduino environment. You'd
// also need to replace Serial with your own UART code for non-Arduino.
void readSerial(void *parameters) {

  uint16_t val=0, *ptr=&val, p=0;
  //Loop forever
  while (1) {
    Serial.print("Enter the blinking rate in ms: ");
    val = getIntUser();
    xTaskCreatePinnedToCore(toggleLED,
                                "Blink with user rate",
                                1024,
                                (void*)ptr, //THIS IS HOW WE PASS THE PARAMETER
                                1,          //Lower priority
                                &task_1,    //Here we assign the handle
                                app_cpu);
        vTaskDelay(2000/portTICK_PERIOD_MS); //print every 100 ms
        
    do{
        Serial.print("\nIf the user wants a new delay, press 1\nIf the user wants to finish, press 2\nSelect: ");
        p = getIntUser();
    }
    while(p!=1 && p != 2);

    if(task_1 != NULL){
        Serial.println("Killing task 1...");
        vTaskDelete(task_1);    //completely remove the first task
        task_1=NULL;            //IMPORTANT TO DO THIS AND THE IF(TASK_1!=NULL)´
    }

    if(p==2){
        Serial.println("Killing task 2...");
        vTaskDelete(NULL);
    }
    p=0;
    }
}


//*****************************************************************************
// Main

void setup() {

  // Configure pin
  pinMode(led_pin, OUTPUT);

  // Configure serial and wait a second
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("Multi-task LED Demo");
  Serial.println("Enter a number in milliseconds to change the LED delay.");

  // Start blink task
  xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
            readSerial,      // Function to be called
            "Read Serial",   // Name of task
            1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
            NULL,           // Parameter to pass
            1,              // Task priority
            NULL,           // Task handle
            app_cpu);       // Run on one core for demo purposes (ESP32 only)

  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}