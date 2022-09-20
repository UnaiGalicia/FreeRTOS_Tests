/* Timers are very useful: to delay the execution of a function, execute a functino periodically...
 * 
 * Let's say that we want a task that periodically interrupts every 220 ms to do something (e.g. take the value of a sensor)
 *  1. Create a separate task and use vTaskDelay()
 *      - creating a task to sample something requires a lot of overhead (each task requires around 1kB)
 *  2. Create a task with xTaskGetTickCount(): This function gives the number of ticks that have elapsed since the scheduler started
 *      - Remember that the tick timer is set to 1 ms, if we need more precision we need HW timer
 *  3. Using HW timers is an option 
 *      - but microcontrollers have few HW timers
 *      - Code wouldn't be very portable
 *  4. SW timers: available to us at the OS level. Similar to SW interrupts but work at task level
 *      - Timers can call arbitrary functions when they expire
 *      - Dependent on tick timer, so precision is limited to tick time (1 ms by default)
 * 
 * When the FreeRTOS timer library is included, a Timer Service task that runs in the background is created when the scheduler starts
 * The task is known as Timer Service Task or Timer Daemon
 * It is responsible for maintaining a number of timers 
 *                       and calling the associated callback function when one of them expires
 * The task is not running all the time: It enters in block state until one of the timers expire. At this point the task will call the associated function or callback
 * These functions are known as callback functions, as theyre passed to the timer as an argument and invoked inside the timer service task
 * Because of this, the callback function runs at the same priority as the timer service task
 * 
 * We want to treat the timer callbacks like interrupts, so we want them to perform their actions quickly and never blocked
 * As a result, things like delay functions or allowforblocking should not be used when using queues, mutexes or semaphores inside these callbacks 
 *
 * We dont want to control this timer service task directly, instead, FreeRTOS gives us a queue and some functions to access that queue
 * We send commands to the timer service task by calling those functions, which place commands on the queue
 * The TST will read commands from the queue and perform the steps to create, stop, start... timers
 * 
 * Even though this design adds the overhead of having a new task, it allows us to have one task that controls many different timers.
 * 
 * TST API Functions' reference: freertos.org/FreeRTOS-Software-Timer-API-Functions.html
 *  
 * 
 */

//#include "timers.h"   //comes included with arduino framework

#include <Arduino.h>
#include <stdlib.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Globals
static TimerHandle_t one_shot_timer = NULL;
//A one shot timer will call an arbitrary function after some time period but it will only call it once
static TimerHandle_t auto_reload_timer = NULL;
//Auto reload, will call the callback recursively

//Callback functions
void myTimerCallback(TimerHandle_t xTimer){
    //Check the passed handle's ID to identify different timers
    if((uint32_t)pvTimerGetTimerID(xTimer) == 0)
        Serial.println("One-shot timer expired.");
    else if((uint32_t)pvTimerGetTimerID(xTimer) == 1)
        Serial.println("Auto-reload timer expired.");
}

void setup(){
    Serial.begin(115200);

    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Timer demo---");

    one_shot_timer = xTimerCreate(
                        "one-shot timer",           //Name of timer
                        2000 / portTICK_PERIOD_MS,  //Period of timer (2000 ms) (in ticks), (milliseconds/porttick_period_ms => in ticks)
                        pdFALSE,                    // Auto-reload: Allows the timer to continuaally expire and execute the callback. We set it as pdFALSE as it is a one-shot timer
                        (void*)0,                   // Timer ID: pointer to something, if we want to create an unique ID. Has to be casted to void*
                        myTimerCallback);           //Callback function
    
    auto_reload_timer = xTimerCreate(
                        "Auto-reload timer",           //Name of timer
                        1000 / portTICK_PERIOD_MS,  //Period of timer (2000 ms) (in ticks), (milliseconds/porttick_period_ms => in ticks)
                        pdTRUE,                     // Auto-reload: Allows the timer to continuaally expire and execute the callback. We set it as pdTRUE
                        (void*)1,                   // Timer ID: pointer to something, if we want to create an unique ID. Has to be casted to void*
                        myTimerCallback);           //Callback function, leave the same

    if(one_shot_timer == NULL || auto_reload_timer == NULL)
        Serial.println("Could not create one of the timers...");
    else{
        //Wait and then start timer
        vTaskDelay(1000/portTICK_PERIOD_MS);
        Serial.println("Starting timers...");

        // Start timer (max block time if command queue is full, REMEMBER THAT TIMERS USE TST, THAT IS CONTROLLED BY A QUEUE!!)
        xTimerStart(one_shot_timer, portMAX_DELAY);
        xTimerStart(auto_reload_timer, portMAX_DELAY);
    }
    vTaskDelete(NULL);

}

void loop(){

}