//Expand the before seen example to a counting semaphore, with 5 tasks

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
static const int num_tasks = 5;
static SemaphoreHandle_t sem_params; //Declare the sempahore as global so that all tasks can use it
static SemaphoreHandle_t mutex;
static const int pin = 25;

typedef struct{
  char body[20];
  uint8_t len;
}Message;

//Task: wait for item in the queue and print it
void myTask(void *parameters){
    
    //Copy the received struct to a local variable
    Message msg = *(Message*)parameters;    

    //Increment the semaphore to indicate that the parameter was taken
    xSemaphoreGive(sem_params);
    
    xSemaphoreTake(mutex, portMAX_DELAY);
    //This section should be treated as a critical section, as the serial port is used. Overlapping will happen without mechanisms
    Serial.print("Received: ");
    Serial.print(msg.body);
    Serial.print("\t| len: ");
    Serial.println(msg.len);
    xSemaphoreGive(mutex);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);

}

void setup(){
    
    Serial.begin(115200);
    pinMode(pin, OUTPUT);

    int delay_arg;
    char task_name[12];
    Message msg;
    char text[14] = "All your base";


    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Semaphore demo---");
    
    strcpy(msg.body, text);
    msg.len = strlen(text);

    //create a counting sem with a maximum of num_tasks, with an initial value of 0
    //Note that a semaphore is initialized to 0, so we dont have to take it before creating our task
    sem_params = xSemaphoreCreateCounting(num_tasks, 0);
    mutex = xSemaphoreCreateMutex();

    for(int i=0; i<num_tasks; i++){

      sprintf(task_name, "Task %i", i);
    
      xTaskCreatePinnedToCore(myTask, task_name, 1024, (void *)&msg, 2, NULL, app_cpu); 
    }
    
    //setup takes back all num_tasks counts of the semaphore by
    //using the take function in a for loop, repeating num_tasks times
    for(int i=0; i<num_tasks; i++)
      xSemaphoreTake(sem_params, portMAX_DELAY);
    
    Serial.println("done.");
}

void loop(){

    vTaskDelay(1000/portTICK_PERIOD_MS);

}