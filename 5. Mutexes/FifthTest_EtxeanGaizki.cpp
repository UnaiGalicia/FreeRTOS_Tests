/*
 * When a task creates another task and passes a LOCAL parameter to it,
 * If the first task ends just before creating the new task,
 * as the LOCAL parameter passed to the new task is in the stack of the old task,
 * the parameter might be deleted and in that case the new task will receive the default value (0)
 * 
 * In this program, we avoid this using a mutex. This is not the best way to do it, but a test.
 * 
 * The FreeRTOS documentation strongly discourages using stack memory to pass arguments to the task creation process, 
 * which is exactly what we’re doing here (even if it does work). 
 * See the note on pvParameters in the API reference for xTaskCreate for more information.
 * https://www.freertos.org/a00113.html
 * 
 */

#include <Arduino.h>
#include <cstdlib>
#include <stdlib.h>
#include <getit.h>
//#include semphr.h Needs to be used in vanilla freertos

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Globals
static SemaphoreHandle_t mutex; //Declare the mutex as global so that all tasks can use it
static const int pin = 25;

//Task: wait for item in the queue and print it
void blink(void *parameters){
    
    //Copy the parameter into a local variable
    int num = *(int*)parameters;

    xSemaphoreGive(mutex);

    Serial.print("Received: ");
    Serial.println(num);
    
    while(1){

        digitalWrite(pin, HIGH);
        vTaskDelay(num/portTICK_PERIOD_MS);
        digitalWrite(pin, LOW);
        vTaskDelay(num/portTICK_PERIOD_MS);
        
    }

}

void setup(){
    
    Serial.begin(115200);
    pinMode(pin, OUTPUT);

    int delay_arg;

    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Mutex demo---");
    
    Serial.print("Enter a delay: ");

    while(Serial.available() <= 0);

    delay_arg = Serial.parseInt();
    
    Serial.print("Sending ");
    Serial.println(delay_arg);

    //Create mutex before starting tasks
    mutex = xSemaphoreCreateMutex();

    //Take the mutex
    //xSemaphoreTake(mutex,portMAX_DELAY);

    xTaskCreatePinnedToCore(blink, "Blinking task", 3000, (void *)&delay_arg, 1, NULL, app_cpu); 

    // Do nothing until mutex has been returned (maximum delay)
    xSemaphoreTake(mutex, portMAX_DELAY);

    Serial.println("done.");

}

void loop(){

    vTaskDelay(1000/portTICK_PERIOD_MS);

}



