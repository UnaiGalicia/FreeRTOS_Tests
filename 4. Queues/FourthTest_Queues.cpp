/*
 *
 * We can use globl variables to pass info between tasks
 * But there are some drawbacks with this option
 *  - If two tasks change the variable, one change will be lost
 *  - If in a 32 bit architecture a variable occupies 3 words, and a task starts writting a new value by 
 *  writing the first word, if at that time other task reads the variable, it will get a wrong value
 * 
 * Therefore, this procedure is not thread safe, as we cant guarantee it willl be written or read without being interrupted by other task
 * 
 * We can replace these variables with queues, a kernel object in FreeRTOS
 * A queue is a FIFO system, and they are usually globally declared so that all tasks have access to it
 * 
 * Tasks copy info into the queue, putting the info in the first position.
 * Writting to a queue using system calls is atomic
 * Adding something to a queue is done by value, not by reference
 * Pointers can be placed into queus, but the referenced value has to be in scope for the receiving task
 * Any type can be put into a queue, assuming enough memory was allocated
 * 
 * Another task asynchronously reaqds the queue at any time removing the first element and shifting the rest forward
 * 
 * A timer can be set in the receiving task to wait for a specific number of ticks while the queue is empty, false will be returned if nothing was written
 * Similarly, if the queue is full, a sending task can wait for x ticks until there is room for an element,
 * 
 * xQueueCreate, Delete, Send and Receive are the most basic functions for queues
 * There are static queues, where queues are created in heap
 * Elements for the queue should not be received from an ISR routine, there are special queue functions for that
 * 
 */

#include <Arduino.h>
#include <cstdlib>
#include <stdlib.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Settings
static const uint8_t msg_queue_len = 5; //Max number of items the queue can hold

//Globals
static QueueHandle_t msg_queue; //Declare the queue as global so that all tasks can use it

//Task: wait for item in the queue and print it
void printMessages(void *parameters){
    
    int item; //I plan to send ints to the queue...

    while(1){

        /* xQueueReceive: Task tries to receive a value from the queue
            - Queue Handle
            - Local variable's address where the queue variable will be copied to
            Casting to a void pointer is not necessary, but done to remind what the function expects(?)
            - Timeout in number of ticks: Task will go into blocked state this number of ticks while something appears into the queue
            If we set this to 0, the task will check the queue and if there are elements it will return inmediately pdTRUE, else it will return pdFALSE
        */

        if(xQueueReceive(msg_queue, (void*)&item, 0) == pdTRUE){
            Serial.println(item);
        }
        //Serial.println(item); //last vtaskdelay of the loop()

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}

void setup(){
    Serial.begin(115200);

    Serial.println();
    Serial.println("---FreeRTOS Queue demo---");

    /* xQueueCreate: Task creates the queue
            - Returns the queue itself
            - The number of elements that the queue will hold MAX
            - The size of each of those elements            
    */

    msg_queue = xQueueCreate(msg_queue_len, sizeof(int));

    xTaskCreatePinnedToCore(printMessages, "Print messages", 1024, NULL, 1, NULL, app_cpu);

}

void loop(){

    static int num = 0;

    /* xQueueSend: Task sends something into the queue
            - Returns pdTRUE when success
            - The queue element
            - What to send
            - Timeout after X ticks, if after that time it wasnt possible, return pdFALSE
    */

    //Try to add item to queue for 10 ticks, fail if queue is full

    if(xQueueSend(msg_queue, (void*)&num, 10) != pdTRUE)
        Serial.println("Queue full");

    num++;
    
    vTaskDelay(1000/portTICK_PERIOD_MS);    //queue wont fill
    //vTaskDelay(500/portTICK_PERIOD_MS);     //In this scenario, the queue will be filled faster than read, thus filling the queue
    //vTaskDelay(2000/portTICK_PERIOD_MS);    //In this scenario, the queue will starve. The local variable of the queue wont change (see the println thats outside the if in the print task)
}