/*
 *  This program creates two tasks:
 *      - listen: Listens serial to receive a string from the user
 *      getStringUser returns a string, which has been dynamically allocated.
 *      Then it notifies the other task
 *      -PrintMessage: Prints the dynamic string.
 *      
 *  NOTE THAT READY, THE BOOLEAN USED TO NOTIFY BETWEEN TASKS IS VOLATILE! 
 *  IF ELSE, THE COMPILER WILL OPTIMIZE IT AS THE FIRST TASK DOESNT USE IT
 * 
 * EDUCBA.COM ON VOLATILE VARS: The main reason behind using volatile is that 
 *  it can change value any time a user wants it to be changed or when 
 *  another thread is running but using the same variable.
 */
#include <Arduino.h>
#include <iostream>
#include <getit.h>

//configure to use one core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const uint8_t len = 100;
static volatile bool ready = true;

char *ptr;
uint8_t tam = 0;

void listen(void *parameter){
  int8_t tam=0;
  while(1){
    Serial.print("Enter a string: ");

    ptr = getStringUser(len, &tam); //ptr will be the users string, with dynamic memory
                        //we will also get the strings length in tam. if its -1, it means that strlcpy went wrong or that there is not enough memory in heap
    if(tam==-1)
        Serial.println("Not enough memory.");
    else if(tam==-2)
        Serial.println("Strlcpy encountered an error.");
    else
        ready = false;
    
    vTaskDelay(100/portTICK_PERIOD_MS);
    std::cout << "Done, another one..." << std::endl;
  }
}

void printMessage(void* parameter){
    while(1){
        if(!ready){

            Serial.println(ptr);

            Serial.print("Free heap (bytes): ");        //Free heap when
            Serial.println(xPortGetFreeHeapSize());     //allocated for string

            vPortFree(ptr);

            Serial.print("Free heap (bytes): ");        //Free heap after
            Serial.println(xPortGetFreeHeapSize());     //freeing the string

            ptr=NULL;
            ready=true;
        }
    }
}

void setup() {                //acts like other task with priority 1

    Serial.begin(115200);
    
    //Wait a sec to see the serial output
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("---FreeRTOS echo demo---");

    xTaskCreatePinnedToCore(  
            listen,        
            "listen task",     
            2000,             //IF WE DONT GIVE TO THE TASK STACK ENOUGH MEMORY, THE PROCESSOR WILL REBOOT
            NULL,             
            1,                
            NULL,             
            app_cpu);         
    
    xTaskCreatePinnedToCore(  
            printMessage,        
            "print task",     
            2000,             //IF WE DONT GIVE TO THE TASK STACK ENOUGH MEMORY, THE PROCESSOR WILL REBOOT
            NULL,             
            1,                
            NULL,             
            app_cpu); 

    vTaskDelete(NULL);         //delete this task to make sure there is only one task running

}

void loop() {
  
}

