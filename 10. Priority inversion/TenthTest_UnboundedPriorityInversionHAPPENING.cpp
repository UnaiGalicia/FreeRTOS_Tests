/*

    Priority inversion:

    A low priority task (Task L) is running and enters a critical section ,taking a mutex or semaphore.
    Then, a high priority task (Task H) interrupts task L and starts to run.

    At some point this high priority task tries to enter the same critical section.´
    When trying to take the mutex, it will be blocked, as task L already holds the lock

    So the scheduler returns execution to task L to finish doing its job in the critical section
    
    THIS IS WHERE PRIORITY INVERSION COMES INTO PLACE: The lower priority task is running when the higher priority task should be running
    The priorities have flipped since task L holds the lock and task H is blocked until the lock is released.

    When Task L releases the lock, Task H will be unblocked and will be able to immediately take the lock and run the critical section.

    When H is done, it will release the lock and perform other actions, and then yield to task L.

    This is named bounded priority inversion as the wait time for H is bounded by the time task L is inside the critical section.
    This is an inherent problem when using critical sections.
    The only way to prevent this is to not use critical sections or mutexes. Or use a multicore processor.
    If you have to use critical sections, keep them as short as possible.

    There are a number of ways to avoid using critical sections:
      - Service task: Assign a unique task to handle shared resources, e.g. a task just to use the serial port
      Other tasks can send messages to this task (e.g. via queue), and also receive messages (e.g. via other queue, or global variables that tasks that arent service tasks have read-only permissions)

UNBOUNDED PRIORITY INVERSION
  Lets assume the same scenario, bA medium priority task is introduced (Task M).
    Task L runs for a while and takes the lock. Task H runs and then tries to take the lock, but it can't.
    In this situation, task M gets runtime, but the task it is doing takes a long time, leaving task L in the critical section all that time
    Task H remains blocked while Task L has the lock, and task L wont exit the critical section as long as task M is running

    This is unbouded priority inversion, as Task M can run forever starving task H

    When Task M finishes, task L will continue its execution and when giving the lock, task H will execute.

  This is shown in the code. The first time that the 3 tasks execute, task H waits 5 s to execute.

*/

#include <Arduino.h>
#include <stdlib.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

static TickType_t cs_wait = 250;
static TickType_t med_wait = 5000;

// Globals
static SemaphoreHandle_t lock;


//*****************************************************************************
// Tasks

// Task L: Low priority 
void doTaskL(void *parameters) {

  TickType_t timestamp;

  //loop to avoid yielding
  while(1){

    //Take lock
    Serial.println("Task L trying to take lock...");
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    xSemaphoreTake(lock, portMAX_DELAY);

    //Print how long we spend waiting for a lock
    Serial.print("Task L got lock. Spent: ");
    Serial.print((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp);
    Serial.println(" ms waiting for lock. Doing some work...");

    //Hog the processor for a while doing nothing
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while((xTaskGetTickCount()*portTICK_PERIOD_MS)-timestamp < cs_wait);

    //release lock
    Serial.println("Task L releasing lock.");
    xSemaphoreGive(lock);

    //Go to sleep
    vTaskDelay(500 / portTICK_PERIOD_MS);


  }
}

// Task M: Medium priority 
void doTaskM(void *parameters) {

  TickType_t timestamp;

  //loop to avoid yielding
  while(1){

    /*Task M directly executes, without locks.
     * When L is executing the critical section while H is waiting for
     * L to release the lock,
     * M will directly execute, and as its priority is greater than L's,
     * it will get proccessor, blocking L and starving H
    */

    //Hog the processor for a while doing nothing
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while((xTaskGetTickCount()*portTICK_PERIOD_MS)-timestamp < med_wait);

    //Go to sleep
    vTaskDelay(500 / portTICK_PERIOD_MS);


  }
}

// Task H: High priority 
void doTaskH(void *parameters) {

  TickType_t timestamp;

  //loop to avoid yielding
  while(1){

    //Take lock
    Serial.println("Task H trying to take lock...");
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    xSemaphoreTake(lock, portMAX_DELAY);

    //Print how long we spend waiting for a lock
    Serial.print("Task H got lock. Spent: ");
    Serial.print((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp);
    Serial.println(" ms waiting for lock. Doing some work...");

    //Hog the processor for a while doing nothing
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while((xTaskGetTickCount()*portTICK_PERIOD_MS)-timestamp < cs_wait);

    //release lock
    Serial.println("Task H releasing lock.");
    xSemaphoreGive(lock);

    //Go to sleep
    vTaskDelay(500 / portTICK_PERIOD_MS);


  }
}


//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Priority inversion demo---");

  // Create kernel objects before starting tasks
  //binary semaphiores are used to test that they cause priority inversion
  //Mutexes have the fix fo runbounded priority inversion
  lock = xSemaphoreCreateBinary();
  
  xSemaphoreGive(lock); //Assure it starts at 1

  xTaskCreatePinnedToCore(doTaskL,
                            "task L",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

  vTaskDelay(1/portTICK_PERIOD_MS);
  //Delay is introduced to force priority inversion
  //L will take the lock first and wont give it until it finishes

  xTaskCreatePinnedToCore(doTaskH,
                            "task H",
                            1024,
                            NULL,
                            3,
                            NULL,
                            app_cpu);

  xTaskCreatePinnedToCore(doTaskM,
                            "task M",
                            1024,
                            NULL,
                            2,
                            NULL,
                            app_cpu);

  vTaskDelete(NULL);
}

void loop() {
  // Do nothing in this task
}