/*
    Deadlock and starvation

    Starvation: Some tasks never get to run or have access to a shared resource because
    other, higher priority tasks are hoggin it. 
    They are a few ways to prevent starvation: 
        
        -If running on a core, a simple solution is to make sure that high priority tasks have some kind of delay,
        or yield to the scheduler from time to time to let other tasks run.

        -If running on a multicore system, you might have the option to run high priority tasks on other core

        -Or you can have your high priority task only run when it receives some kind of notification from an interrupt or event (like the binary semaphore we saw at the HW INterrupts example)
        This allows lower priority tasks run all other time

        -Another way is to use the aging technique: Lets say we have task A running at a higher priority than task B
        Task A is hogging all the processor time. The scheduler or another even higher priority task, can periodically 
        check the length of time that other tasks have been in the ready state but have not run.
        As the time increases (seconds, minutes, hours...), the priority of these tasks waiting is gradually increased
        Eventually the waiting tasks' priority will equal to the other task, and the scheduler will be forced to give to it running time
        Once task B has had enough running time, its priority can be reset to its default value

    Deadlock: Imagine that 4 tasks use in total 4 mutexes. A task that wants to access the shared resource must take 2 mutexes: They take one, wait until they take another one, perform actions with the shared resource and leave the mutexes.
    If all tasks take 1 mutex, no mutexes are left, so all tasks wait in an infinte loop for a second mutex that never becomes free.
    They are all freezed, its a system-wide starvation.

    */

//Deadlock example

#include <Arduino.h>
#include <stdlib.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Settings
TickType_t mutex_timeout = 1000 / portTICK_PERIOD_MS;
//Timeout for any task that tries to take a mutex!

//Globals
static SemaphoreHandle_t mutex_1;
static SemaphoreHandle_t mutex_2;

//**********************************************************
//Tasks

//Task A (High priority)
void doTaskA(void*parameters){

    while(1){

        //Take mutex 1
        if( xSemaphoreTake(mutex_1, mutex_timeout) == pdTRUE){
            Serial.println("Task A took mutex 1");
            vTaskDelay(1 / portTICK_PERIOD_MS);     //force deadlock! (without if statements and portMAX_DELAY )

            if(xSemaphoreTake(mutex_2, mutex_timeout) == pdTRUE){
                // NOT WAITING MUTEX_TIMEOUT  WOULD CAUSE DEADLOCK!
                Serial.println("Task A took mutex 2");

                //Critical section protected by 2 mutexes
                Serial.println("Task A doing work");
                vTaskDelay(500/portTICK_PERIOD_MS);         //simulate that critical section takes 500ms
            } else {
                Serial.println("Task A timed out waiting for mutex 2. Trying again...");
                }
        } else {
            Serial.println("Task A timed out waiting for mutex 1. Trying again...");
        //If the task times out getting the mutex, it prints an error msg,
        //releases any mutexes (if taken) and waits for 500 ms before trying again
        }

        //Return mutexes
        xSemaphoreGive(mutex_2);
        xSemaphoreGive(mutex_1);

        Serial.println("Task A going to sleep");
        vTaskDelay(500/portTICK_PERIOD_MS);
        //Wait to let other task execute
    }
}

//Task B (low priority)
void doTaskB(void * parameters){

    while(1){

        //Take mutex 2 and wait to force deadlock
        if(xSemaphoreTake(mutex_2, mutex_timeout)==pdTRUE){
            Serial.println("Task B took mutex 2");
            vTaskDelay(1 / portTICK_PERIOD_MS);         //force deadlock!
                                                    //IF we dont do this, deadlock may or may not happen
            //Both tasks will have the two different mutexes, so both will try to take the other's one, without success

            if(xSemaphoreTake(mutex_1, mutex_timeout) == pdTRUE){
                Serial.println("Task B took mutex 1");
        

                //Critical section protected by 2 mutexes
                Serial.println("Task A doing work");
                vTaskDelay(500/portTICK_PERIOD_MS);         //simulate that critical section takes 500ms
            } else {
                Serial.println("Task B timed out waiting for mutex 1");
                }
        } else {
            Serial.println("Task B timed out waiting for mutex 2");
            }

        //Return mutexes
        xSemaphoreGive(mutex_1);
        xSemaphoreGive(mutex_2);
        //Task B gets and gives mutexes in reverse order as task A, but
        //it is not important, as deadlock can still occur as one tasks waits for the other to release

        Serial.println("Task B going to sleep");
        vTaskDelay(500/portTICK_PERIOD_MS);
        //Wait to let other task execute


    }

}

void setup(){
    Serial.begin(115200);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Deadlock Demo---");

    //create mmutexes
    /*Wont't work with mutexes! Don't really know why
    mutex_1 = xSemaphoreCreateMutex();
    mutex_2 = xSemaphoreCreateMutex();
    */

    mutex_1 = xSemaphoreCreateBinary();
    mutex_2 = xSemaphoreCreateBinary();

    //Start task A (high priority)
    xTaskCreatePinnedToCore(doTaskA, "Task A", 1500, NULL, 2, NULL, app_cpu);

    //Start task B (low priority)
    xTaskCreatePinnedToCore(doTaskB, "Task B", 1500, NULL, 1, NULL, app_cpu);

    vTaskDelete(NULL);
}

void loop(){

}

/*The use of multiple mutexes can cause deadlock!!!!

    To prevent deadlocks, the most important thing is NOT TO HAVE A TASK WAITING FOREVER FOR A QUEUE, MUTEX OR SEMAPHORE!

    In our case, PORTMAXDELAY SHOULD NEVER BE USED!
    Some kind of timeout (1 sec e.g.) should always be used
    FOr example, in our program mutex_timeout

    Any time we block a task to wait for a kernel object, we should have some kind od timeout

*/

