/*Program that creates 5 producer tasks that add values (their task number, 3 times) to a shared circular buffer
and 2 consumer tasks that read the circular buffer

The circular buffer has to be protected using semaphores and mutexes
two separate counting semaphores are needed in addition to the mutex
    - One that counts the number of filled slots
    - Other to count the number of empty slots

THIS TIME WE WILL IMPLEMENT THIS WITH A QUEUE! ITS MUCH EASIER.
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

/* ENUM: Acts like a #define, but it's not a preprocessor directive
 *
 * enum week{Mon, Tue, Wed, Thu, Fri, Sat, Sun};
 * ...
 * enum week day;
 * day = Wed;
 * printf("%d", day);   //OUTPUT: 2;
 * day = Mon;
 * printf("%d", day);   //OUTPUT: 0;
 * 
 */ 

//Settings
enum {BUF_SIZE = 10};                    //Size of circular buf
static const uint8_t num_prod_tasks = 5; 
static const uint8_t num_cons_tasks = 2;
static const uint8_t num_writes = 3;     //How many times will producers write

//Globals
static SemaphoreHandle_t bin_sem;       //Semaphore to pass params
static SemaphoreHandle_t serialMutex;   //Mutex to control Serial access

static QueueHandle_t msg_queue;         //Queue to pass values from producers to consumers
static const uint8_t msg_queue_len = 10;

//Task that writes shared buf
void producer(void *parameters){
    
    //Copy the received param (task num) into a local variable
    uint8_t num = *(uint8_t*)parameters;    

    //Increment the semaphore to indicate that the parameter was taken
    xSemaphoreGive(bin_sem);

    //The only thing we have to do is to fill the queue    
    for(uint8_t i=0; i<num_writes; i++)
        xQueueSend(msg_queue, (void*)&num, portMAX_DELAY);  //no mutex needed as this op is atomic!!!


    vTaskDelete(NULL);

}

//Task that reads queue
void consumer(void *parameters){
    
    //Copy the received param (task num) into a local variable
    uint8_t val;

    
    while(1){

        //If queue is not empty, we print the value
        if(xQueueReceive(msg_queue, (void*)&val, portMAX_DELAY)){

            xSemaphoreTake(serialMutex, portMAX_DELAY);
            Serial.println(val);
            xSemaphoreGive(serialMutex);    

        }
    }

}

void setup(){
    
    Serial.begin(115200);

    uint8_t task_num=0;
    char task_name[7];

    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Semaphore challenge 2---");
    
    bin_sem = xSemaphoreCreateBinary();
    serialMutex = xSemaphoreCreateMutex();

    msg_queue = xQueueCreate(msg_queue_len, sizeof(uint8_t));

    for(uint8_t i=0; i<num_prod_tasks; i++){

        sprintf(task_name, "Prod %d", i);
        xTaskCreatePinnedToCore(producer, task_name, 1024, (void *)&i, 1, NULL, app_cpu); 
        xSemaphoreTake(bin_sem, portMAX_DELAY);
    }

    for(uint8_t i=0; i<num_cons_tasks; i++){
        sprintf(task_name, "Cons %i", i);
        xTaskCreatePinnedToCore(consumer, task_name, 1024, NULL, 1, NULL, app_cpu); 
    }

    xSemaphoreTake(serialMutex, portMAX_DELAY);
    Serial.println("done.");
    xSemaphoreGive(serialMutex);

    vTaskDelete(NULL);
}

void loop(){
    //Will never reach here
    vTaskDelay(1000/portTICK_PERIOD_MS);

}