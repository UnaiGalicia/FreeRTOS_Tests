/*

UNBOUNDED PRIORITY INVERSION
  Lets assume the same scenario, bA medium priority task is introduced (Task M).
    Task L runs for a while and takes the lock. Task H runs and then tries to take the lock, but it can't.
    In this situation, task M gets runtime, but the task it is doing takes a long time, leaving task L in the critical section all that time
    Task H remains blocked while Task L has the lock, and task L wont exit the critical section as long as task M is running

    This is unbouded priority inversion, as Task M can run forever starving task H

    When Task M finishes, task L will continue its execution and when giving the lock, task H will execute.

  This is shown in the code. The first time that the 3 tasks execute, task H waits 5 s to execute.

----

One possible solution is to use priority ceiling protocol

Every shared resource/mutex/semaphore is given a priority ceiling value, 
which needs to be equal to o greater than the priority of the 
highest priority task that will use that resource or lock.

In our example, the lock should have at least priority 3 (Task H).

But when task L takes the lock, its priority level is automatically
boosted to that of the resources' priority ceiling.

This assumes that the RTOS can dynamically re-assign priorities to tasks once created.

With this system, task H will try to take the lock but it will be blocked,
as task L has the lock. 

Once Task L releases the lock, task H may take it and enter the critical section.
As task L's priority level was boosted to 3, task M won't be able to take execution.

When task H is done, task M is allowed to run.
Also, while task M is running, task H can preempt it,
including running the critical section.

HOWEVER, BOUNDED PRIORITY INVERSION CAN STILL OCCUR
As task L can prevent task H from running so long as it has the lock.

Its possible to implement priority ceiling in FreeRTOS but it would
require a lot of work and possibly modifying the queue source code.

There is no reason to do that when we can usea solution already built in FreeRTOS:
PRIORITY INHERITANCE

It is similar to priority ceiling, with a big difference:

Any task holding a lock has its priority level boosted to that of
any task at a higher priority that attempts to take the lock.

In our exapmle, task L takes the lock and starts to work in the critical section
Task H preempts task L, runs for a while and attempts to take the lock.
Because a lower priority task is holding the lock, task has its priority
boosted to be the same of task requesting the lock, that is, inheriting the priority level of a task.

In our example, Task L would get task H's priority level, execute the critical section,
release the lock, and task H would take the lock and execute the critical section.
Task L's priority level would be restored.

Task H would leave ethe lock, continue executing until ending, and then yielding to task M.
If during the execution of task M task H executes, it can preempt it. Task L can't, just like in usual freertos task hierarchy

Bounded priority inversion can still happen, but in FreeRTOS
mutexes automatically handle bounded priority.

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


    /*Only changing to use a mutex has caused the correct functioning.
    Task L enters in the critical section and when task H tries to get
    the lock, task L's priority is raised, not letting task H executing.
    Then task H runs and task M doesnt preempt it, as its priority is lower
    */

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
  //We will use a mutex instead of semaphore, as
  //mutexes have the fix to bounded priority inversion
  lock = xSemaphoreCreateMutex();
  
  //mutex already starts to 1
 // xSemaphoreGive(lock); //Assure it starts at 1

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