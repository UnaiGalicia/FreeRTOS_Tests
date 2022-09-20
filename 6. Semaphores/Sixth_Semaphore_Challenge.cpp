/*Program that creates 5 producer tasks that add values (their task number, 3 times) to a shared circular buffer
and 2 consumer tasks that read the circular buffer

The circular buffer has to be protected using semaphores and mutexes
two separate counting semaphores are needed in addition to the mutex
    - One that counts the number of filled slots
    - Other to count the number of empty slots
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
static const uint8_t num_prod_tasks = 5; //
static const uint8_t num_cons_tasks = 2;
static const uint8_t num_writes = 3;     //How many times will producers write

//Globals
static uint8_t buf[BUF_SIZE];           //Shared buffer
static uint8_t head = 0;                //Writing index for buf
static uint8_t tail = 0;                //Reading index for buf
static SemaphoreHandle_t bin_sem;       //Semaphore to pass params

static SemaphoreHandle_t prod_sem;
static SemaphoreHandle_t cons_sem;

static SemaphoreHandle_t headMutex;
static SemaphoreHandle_t tailMutex;
static SemaphoreHandle_t serialMutex;

//Task that writes shared buf
void producer(void *parameters){
    
    //Copy the received param (task num) into a local variable
    uint8_t num = *(uint8_t*)parameters;    

    //Increment the semaphore to indicate that the parameter was taken
    xSemaphoreGive(bin_sem);
    
    for(uint8_t i=0; i<num_writes; i++){
        
        xSemaphoreTake(prod_sem, portMAX_DELAY);
        
        xSemaphoreTake(headMutex, portMAX_DELAY);
        buf[head] = num;
        head = (head + 1) % BUF_SIZE;
        xSemaphoreGive(headMutex);

        xSemaphoreGive(cons_sem);

    }

    vTaskDelete(NULL);

}

//Task that writes shared buf
void consumer(void *parameters){
    
    //Copy the received param (task num) into a local variable
    uint8_t val;

    
    while(1){

        xSemaphoreTake(cons_sem, portMAX_DELAY);
        
        xSemaphoreTake(tailMutex, portMAX_DELAY);
        val = buf[tail];        
        tail = (tail + 1) % BUF_SIZE;
        xSemaphoreGive(tailMutex);

        xSemaphoreTake(serialMutex, portMAX_DELAY);
        Serial.println(val);
        xSemaphoreGive(serialMutex);
        
        xSemaphoreGive(prod_sem);
    }

}

void setup(){
    
    Serial.begin(115200);

    uint8_t task_num=0;
    char task_name[7];

    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Semaphore Challenge 1---");
    
    bin_sem = xSemaphoreCreateBinary();
    

    //create a counting sem with a maximum of num_tasks, with an initial value of 0
    //Note that a semaphore is initialized to 0, so we dont have to take it before creating our task
    prod_sem = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);           //Occupied slots
    cons_sem = xSemaphoreCreateCounting(BUF_SIZE, 0);    //Free slots
    headMutex = xSemaphoreCreateMutex();
    tailMutex = xSemaphoreCreateMutex();
    serialMutex = xSemaphoreCreateMutex();

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