/*
 * Semaphores: used to protect shared resources
 * 
 * They are like mutexes, but a semaphore can count to numbers greater than one
 * 
 * It is mainly used for signalling mechanism to other threads, rather than securing shared resources
 * 
 * Asemaphore is like a lock, but instead of 0 or 1, it can count more times
 * 
 * Lets say that we only want 2 tasks to have access to one part of our code at a time
 *  
 *  1. We create a semaphore and set its initial and max. value to 3
 *  
 *  2. Task A wants to use a shared resource, so the task first checks the semaphore.
 *      Since the semaphore is >0, task A can take 1 --> Semaphore decrements.
 *      Task A enters the critical section
 *  3. Task B wants to enter too
 *      Checks the semaphore, and as its greater than 0 it decrements by one
 *      Task B enters the critical section 
 * 
 *   Now the semaphore's value is 0
 *  4. If a third task tries to enter, it will be blocked, as the semaphore is 0.
 * 
 * Just like with mutexes, the task can wait until the semaphore is accesible or it can do other actions in the meantime
 * 
 *  5. When any task that entered the semaphore finishises the critical section, it returns one unit to the semaphore, allowing other tasks to enter
 * 
 * Checking and decrementing the sempahore functions are ATOMIC
 * 
 * THIS IS HOW SEMAPHORES WORK, BUT THEY ARE RARELY USED LIKE THIS.
 * 
 * USUALLY, THEY ARE USED TO SYNCHRONIZE THREADS
 * 
 * Usually, a producer-consumer scheme is used:
 *  One or more tasks add and remove data from a shared resource (buffer, linked list...)
 *  If access to the resource is not atomic, then mutexes should also be used
 * 
 * Whenever a producer task adds something to the buffer (shared resource), the semaphores value is incremented by one
 * Tasks then will check the semaphore's value and if it reaches its max, they wont be able to add anything else
 * 
 * Other tasks may read from the shared resource (consumers)
 *  Each time one of the consumers wants to access the shared resource, it must first decrement the semaphore (if its value is greater than 0)
 *  Each time a consumer reads, the value is taken away from the shared resource
 *  When the semaphore reaches 0, consumers will not be able to consume from the shared resource
 * 
 * The semaphore then allows tasks to add and remove items from the shared resource in a synchronized manner
 * 
 * This looks like a queue. If a queue can be used, then it should be as semaphores are more complex to debug and implement
 * 
 * The Mutex gives tasks ownership:
 *  If a task has the mutex, it owns the lock and thus the shared resource, no other tasks may take the lock, they have to wait their turn
 *  They implement priority inheritance: Involves automatically raising the priority of tasks that hold mutex locks to prevent a higher priority task from being blocked for a long time
 * 
 * semaphores DO NOT give ownership: Tasks SHOULD NOT INCREMENT AND DECREMENT THE SAME SEMAPHORE, rather they should use them to signal other tasks that some shared resource is ready to be read or consumed. One task increases and other decreases the semaphore's value.
 *  and dont implement priority inheritance
 * 
 * BINARY SEMAPHORES ARENT LIKE MUTEXES!! They are similar conceptually, but remember the ownership thing, for example.
 *  They are used with Interrupts to notify that data is ready, as mutexes arent suitable for interrupts
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
static SemaphoreHandle_t bin_sem; //Declare the binary sempahore as global so that all tasks can use it
static const int pin = 25;

//Task: wait for item in the queue and print it
void blink(void *parameters){
    
    //Copy the parameter into a local variable
    int num = *(int*)parameters;

    //Release the binary semaphore so that the creating function can finish
    //Add one to the semaphore so that setup can wait
    xSemaphoreGive(bin_sem);

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
    Serial.println("---FreeRTOS Semaphore demo---");
    
    Serial.print("Enter a delay: ");

    while(Serial.available() <= 0);

    delay_arg = getIntUser();//= Serial.parseInt();
    
    Serial.print("Sending ");
    Serial.println(delay_arg);

    //Create binary semaphore before starting tasks
    //Note that a semaphore is initialized to 0, so we dont have to take it before creating our task
    bin_sem = xSemaphoreCreateBinary();

    //xSemaphoreTake(mutex,portMAX_DELAY);
    xTaskCreatePinnedToCore(blink, "Blink task", 3000, (void *)&delay_arg, 1, NULL, app_cpu); 

    //Note that FreeRTOS docs dont recommend pasing arguments with variables that are in the stack memory

    // Do nothing until semaphore has been returned (maximum delay)
    //This will block setup until the parameter has been read (sem=0?)
    xSemaphoreTake(bin_sem, portMAX_DELAY);

    Serial.println("done.");

}

void loop(){

    vTaskDelay(1000/portTICK_PERIOD_MS);

}