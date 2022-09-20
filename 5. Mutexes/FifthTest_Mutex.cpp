/*Mutex
 *
 * Queues can be used as a form of inter task communication to avoid issues with shared resources  
 * But if we want to use a global variable (flag, counter...)
 * We use mutexes
 * 
 * Lets say we have two tasks that want to access a global var
 * A queue is useful to send messages between tasks, but what if we need to keep the global variable around?
 * 
 * Race conditions happen when the systems behaviour depends on the timing of uncontrollable events
 * 
 * 
 * EXAMPLE:
 *  1. Task A gets the value (2) of the global variable from memory
 *  2. Task B gets the value (2) of the global variable from memory
 *  3. Task B increments the value (3) and writes the value to memory
 *  4. Task A increments the value (3) and writes the value to memory
 *  5. The global variable only incremented by one. This is a race condition
 *  
 * How the global var is incremented depends on the exact order of execution of instructions in the tasks,
 * and as can't assume the execution order of the tasks, as they can preempt between them,
 * we might run into problems if we dont use shared resources.
 * 
 * The section of the code where tasks work with shared resources is named CRITICAL SECTION.
 * Critical sections have to entirely be executed by a task before it can be entered by other task
 * 
 * RESOURCES TO SYNCHRONIZE THREADS AND PROTECT SHARED RESOURCES
 * - Queue: Pass data between threads, can be used to signal between threads when it is okay to enter into a critical section
 * - Lock: Allows onl one thread to enter the locked section of code
 * - Mutex: Same as the lock but works for all processes of the system
 * - Semaphore: Like a mutex, but has a counter that allows a limited num of threads to enter to a critical section at a time
 * 
 * The mutex can be represented as a bool. A mutex could be used to solve the beforementioned problem.
 * A task that wants to enter to a critical section cehcks the value of the mutex.
 *  - If its value is 1, the task will set it to 0 and enter the critical section. When finished, it will set the mutex to 1 so that other tasks can enter the critical section
 *  - If its value is 0, the task knows that other task is in the critical section, so it will wait (idle or do other things and try again) until the mutex is set again to 1
 * 
 * FreeRTOS considers mutexes a special type of semaphore
 * 
 * More info on mutexes/semaphores: https://freertos.org/a00113.html
 * 
 */

#include <Arduino.h>
#include <cstdlib>
#include <stdlib.h>
//#include semphr.h Needs to be used in vanilla freertos

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Globals
static int shared_var = 0;
static SemaphoreHandle_t mutex; //Declare the mutex as global so that all tasks can use it

//Task: wait for item in the queue and print it
void incTask(void *parameters){
    
    int local_var;

    while(1){

        /*xSemaphoreTake
            - Mutex we want to take
            - Number of ticks to wait. If we couldn't take the mutex in those ticks, return pdFALSE
             0 is used so that the function immediately returns
        */

        //Take the mutex prior to critical section
        if(xSemaphoreTake(mutex,0) == pdTRUE){

            //bad way to do shared_var++ randomly
            local_var = shared_var;
            local_var++;
            vTaskDelay(random(100,500) / portTICK_PERIOD_MS);
            shared_var = local_var;
            
            //Give mutex after critical section
            xSemaphoreGive(mutex);

            //Print out new shared variable
            Serial.println(shared_var);
        }

        else{

            //Do something while waiting for the mutex!

        }


    }

}

void setup(){
    
    Serial.begin(115200);

    randomSeed(analogRead(0));

    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Mutex demo---");
    
    
    //Create mutex before starting tasks
    mutex = xSemaphoreCreateMutex();



    xTaskCreatePinnedToCore(incTask, "Increment task 1", 1024, NULL, 1, NULL, app_cpu);
    xTaskCreatePinnedToCore(incTask, "Increment task 2", 1024, NULL, 1, NULL, app_cpu);

    vTaskDelete(NULL);

}

void loop(){

}



