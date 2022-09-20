/*

C variables get allocated in memory
    - Global and static variables stay for the entire duration of the program

    - Local variables and pointers are pushed to the stack, and deallocated when the function returns
    Automatically allocated

    -  The heap also grows also as the program is executed.
    The heap is used for dynamic allocation where the programmer explicitely asks the memory space for the variable
    In C and C++ the heap has to be freed! malloc returns NULL if the memory is full

When a task or kernel element (semaphore, queue...) is created, that task gets a portion of the memory from the heap
Theat memory block is divided into the Task Control Block (TCB) and a stack unique to that task.
TCB is a struct that keeps important info of the task, like the location of the tasks stack or the priority level
When we create a task, we specify the number of bytes that the tasks stack will have

Tasks can be created so that they are stored in the static memory: #define configSUPPORT_STATIC_ALLOCATION 1

freeRTOS gives heap management schemes. (heap_1, heap_3, heap_4, heap_5,)
heap4 is the mostly often used. heap3 allows using C malloc like functions, in the rest these functions arent thread safe

*/

#include <Arduino.h>
#include <iostream>

//configure to use one core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

void testTask(void *parameter){
  while(1){
    int a = 1, b[100];  //400 bytes --> we will need 
                        //that size + the 768 bytes that all freeRTOS tasks need
    
    //Do something with the array so the compiler doesnt optimize it
    for(int i=0; i<100; ++i){
        b[i] = a+1; 
    }
    
    std::cout << b[0] << std::endl;
    //Serial.println(b[0]);

    //This function tells us how many bytes we have left in the task stack
    //This info is reported in words, so we should multiply by 4 to get the info in bytes in a 32 bit system
    //If this value approaches 0, were getting near to overflowing the stack
    Serial.print("High water mark (words): ");
    Serial.println (uxTaskGetStackHighWaterMark(NULL));

    //total amount of heap memory available (in bytes) to us
    Serial.print("Heap before malloc (bytes): ");
    Serial.println(xPortGetFreeHeapSize());

    //Pointer to use malloc and check if the heap size changed
    int *ptr = (int*)pvPortMalloc(1024 * sizeof(int));

    //check malloc output to prevent overflow, if its NULL it means that we dont have heap memory left
    if(ptr==NULL)
        Serial.println("Not enough memory.");
    else{
    //Do something with the pointer so that it isnt optimized out
        for (int i=0; i<1024; i++)
            ptr[i] = 3;

    }

    //Check if the heap size changed
    Serial.print("Heap after malloc (bytes): ");
    Serial.println(xPortGetFreeHeapSize());
    //in freeRTOS malloc isnt thread safe unless using heap3
    //in esp32 malloc can be used
    
    vPortFree(ptr);    //free up our allocated memory, like C free()

    vTaskDelay(100 / portTICK_PERIOD_MS);

  }
}

void setup() {                //acts like other task with priority 1

    Serial.begin(115200);
    
    //Wait a sec to see the serial output
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("---FreeRTOS Memory demo---");

    xTaskCreatePinnedToCore(  
            testTask,        
            "Test task",     
            2000,             //IF WE DONT GIVE TO THE TASK STACK ENOUGH MEMORY, THE PROCESSOR WILL REBOOT
            NULL,             
            1,                
            NULL,             
            app_cpu);         
    
    vTaskDelete(NULL);         //delete this task to make sure there is only one task running

}

void loop() {
  
}

