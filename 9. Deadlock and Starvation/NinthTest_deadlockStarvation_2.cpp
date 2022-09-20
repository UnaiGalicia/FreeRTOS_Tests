/*
    Livelock

    Timeouts can prevent total dealock, but don't prevent livelock

    Livelock: There is a chance that all tasks pick a different lock and then leave it,
    repeating this process over and over.Tasks are not blocked, but they still cant perform their actions

    Dijkstra's solution: Assigning a hierarchy to locks
    Each lock gets a number (1,2,4,3,5...)
    Each task picks the available lock with the lowest number


    Now task A takes mutex 1 first and 
    task B gets blocked waiting for that same mutex
    As a result task A can take the second mutex and do its work

    When its done, it returns both mutexes so that task B
    can do its job in the critical section.

    This continues forever with no deadlock


    Another way to avoid livelock: introducing an arbitrator that indicates which task can access the critical section

        The arbitrator can be implemented as a mutex that has to be taken prior to accessing the shared resource
        

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
        xSemaphoreTake(mutex_1, mutex_timeout);
        Serial.println("Task A took mutex 1");
        vTaskDelay(1 / portTICK_PERIOD_MS);     //force deadlock! (without if statements and portMAX_DELAY )

        xSemaphoreTake(mutex_2, mutex_timeout);
        Serial.println("Task A took mutex 2");

        //Critical section protected by 2 mutexes
        Serial.println("Task A doing work");
        vTaskDelay(500/portTICK_PERIOD_MS);         //simulate that critical section takes 500ms
        //Good practice to release mutexes in the reverse order in which you took them
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
        xSemaphoreTake(mutex_1, mutex_timeout);
        Serial.println("Task B took mutex 1");
        vTaskDelay(1 / portTICK_PERIOD_MS);         //force deadlock!
                                                    //IF we dont do this, deadlock may or may not happen
            //Both tasks will have the two different mutexes, so both will try to take the other's one, without success

        xSemaphoreTake(mutex_2, mutex_timeout);
        Serial.println("Task B took mutex 2");
        
                //Critical section protected by 2 mutexes
        Serial.println("Task B doing work");
        vTaskDelay(500/portTICK_PERIOD_MS);         //simulate that critical section takes 500ms
        
        xSemaphoreGive(mutex_2);
        xSemaphoreGive(mutex_1);
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
    mutex_1 = xSemaphoreCreateMutex();
    mutex_2 = xSemaphoreCreateMutex();

    //Start task A (high priority)
    xTaskCreatePinnedToCore(doTaskA, "Task A", 1024, NULL, 2, NULL, app_cpu);

    //Start task B (low priority)
    xTaskCreatePinnedToCore(doTaskB, "Task B", 1024, NULL, 1, NULL, app_cpu);

    vTaskDelete(NULL);
}

void loop(){

}

