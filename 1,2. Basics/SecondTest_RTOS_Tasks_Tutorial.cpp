/*
 *  Second FreeRTOS demo: Task scheduling
 *      Two tasks with different priorities are created
 *      
 *      The lower priority one prints a string, and then the task
 *  is blocked for 1000. 
 * 
 *      The higher priority one prints an '*'
 * 
*/ 

#include <Arduino.h>
#include <iostream>

#if CONFIG_FREERTOS_UNICORE           //configure to use one core
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//String to print by task 1:
static const char msg[] = "My name is Slim Shady";

/*Task Handles: 
 *  Handles for both tasks so that 
 *  we can control their state from a third task
*/  

static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;

/*
 *  TASK STATES
 *      - RUNNING:  Only 1 task at a given time can be running
 *      - READY:    Scheduler can move this task to ready
 *      - BLOCKED:  A task that can't change to running state. 
 *                  The task will wait until the unblocking event
 *                  occurs (semaphore released, time expired...)
 *      - SUSPENDED: The task can't change to running state.
 *                   The task will wait until other task explicitly
 *                   "unsuspends" it. Good to "sleep" tasks.
 * 
 * When the scheduler switches tasks, FreeRTOS has to store 
 * at which point of the program the task was and also its 
 * variables and CPU register values, that is the context of the task
 * 
 * The context is stored in the stack, thats why we need stack size
*/ 

/***********************************************************************
 * Tasks
 * 
 * Task1: Print string in terminal WITH LOWER PRIORITY
*/

void startTask1(void *parameter){

    int msgLen = strlen(msg);                   //read string length

    while(1){                                   //Task loop printing
        printf("\n");
        for(int i=0; i<msgLen; ++i){
            printf("%c", msg[i]);
            vTaskDelay(50 / portTICK_PERIOD_MS);               //As the 2nd task will have higher priority,
        }
        printf("\n");                           //We'll see how this printf is interrupted by the other task
        
    }

}

//TASK2: Print * to the terminal with higher priority

void startTask2(void *parameter){

    while(1){
        printf("*");
        vTaskDelay(100/portTICK_PERIOD_MS); //print every 100 ms
    }

}


void setup(){

    Serial.begin(300);      //use slow baud rate to see the whole process

    vTaskDelay(2000/portTICK_PERIOD_MS);    //wait por si acaso
    printf("\n---FreeRTOS DEMO WITH TASKS---\n\b");

    printf("Setup and loop functions running on core %d with priority %d\n",
             xPortGetCoreID(), uxTaskPriorityGet(NULL));

    xTaskCreatePinnedToCore(startTask1,
                                "Task 1",
                                1024,
                                NULL,
                                1,          //Lower priority
                                &task_1,    //Here we assign the handle
                                app_cpu);

    xTaskCreatePinnedToCore(startTask2,
                                "Task 2",
                                1024,
                                NULL,
                                2,          //Higher priority
                                &task_2,    //Here we assign the handle
                                app_cpu);
    
}   

void loop() //Remember that setup and loop are the 3rd task that controls the other 2
{
    //We suspend the higher priority task at the beginning...
    for(int i=0; i<3; i++){
        vTaskSuspend(task_2);   //HERE WE USE THE HANDLE
        vTaskDelay(3000/portTICK_PERIOD_MS);    //WE KEEP THE TASK SUSPENDED FOR 2 SECS
                                                /*DURING THIS PERIOD OF TIME TASK3 (SETUPLOOP) DOES NOTHING 
                                                 * AND MAKES TASK1 THE ONLY ONE RUNNING
                                                 * TASKS:
                                                 *  TASK1: RUNNING
                                                 *  TASK2: SUSPENDED
                                                 *  TASK3: WAITING..*/
        vTaskResume(task_2);    // Suspension goes away for 2
        vTaskDelay(3000/portTICK_PERIOD_MS);
    }

    if(task_1 != NULL){
        vTaskDelete(task_1);    //completely remove the first task
        task_1=NULL;            //IMPORTANT TO DO THIS AND THE IF(TASK_1!=NULL)
    }

}