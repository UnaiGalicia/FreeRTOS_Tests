/*

    Full dining philosophers challenge
    Instead of 2 tasks, we have 5 tasks, and they all call the same function,
    where they try to take the left chopstik, the right chopstick and eat for a while
    when they're done, they return the chopsticks and the function exits

    id all philosophers get a chance to eat without getting deadlocked, a done message is printed

    Delays have been added to force deadlock. They can't be erased.

    Hierarchy solution and arbitrary solutions have to be applied.

*/

#include <Arduino.h>
#include <stdlib.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
enum { NUM_TASKS = 5 };           // Number of tasks (philosophers)
enum { TASK_STACK_SIZE = 2048 };  // Bytes in ESP32, words in vanilla FreeRTOS

// Globals
static SemaphoreHandle_t bin_sem;   // Wait for parameters to be read
static SemaphoreHandle_t done_sem;  // Notifies main task when done
static SemaphoreHandle_t chopstick[NUM_TASKS];

static SemaphoreHandle_t arbitrator;
static TickType_t mutexTime = 1000 / portTICK_PERIOD_MS;
static TickType_t mutexTime2 = 1 / portTICK_PERIOD_MS;
//Very short mutex time to see how arbitrator does not let us in
//*****************************************************************************
// Tasks

int calcPos(int num){
    int tmp;

    tmp = (num!=NUM_TASKS)?num:0;

    return tmp;

}


// The only task: eating
void eat(void *parameters) {

  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);
  while(1){   //While loop to make tasks denied by arbitrator print the message
  //Take the arbitrator if possible
  if(xSemaphoreTake(arbitrator, mutexTime2) == pdTRUE){
    // Take left chopstick
    xSemaphoreTake(chopstick[(num)%NUM_TASKS], mutexTime);
    sprintf(buf, "Philosopher %i took chopstick %i", num, num);
    Serial.println(buf);

    // Add some delay to force deadlock
    vTaskDelay(1 / portTICK_PERIOD_MS);

    // Take right chopstick
    xSemaphoreTake(chopstick[(num+1)%NUM_TASKS], mutexTime);
    sprintf(buf, "Philosopher %i took chopstick %i", num, (num+1)%NUM_TASKS);
    Serial.println(buf);

    // Do some eating
    sprintf(buf, "Philosopher %i is eating", num);
    Serial.println(buf);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Put down right chopstick
    xSemaphoreGive(chopstick[(num+1)%NUM_TASKS]);
    sprintf(buf, "Philosopher %i returned chopstick %i", num, (num+1)%NUM_TASKS);
    Serial.println(buf);

    // Put down left chopstick
    xSemaphoreGive(chopstick[(num)%NUM_TASKS]);
    sprintf(buf, "Philosopher %i returned chopstick %i", num, num);
    Serial.println(buf);

    //Give the arbitrator when finished to let other tasks
    xSemaphoreGive(arbitrator);   

    // Notify main task and delete self
    xSemaphoreGive(done_sem);
    break;  //exit
    
  } else {
  //If taking the arbitrator wasn't possible, print (will be printed after mutexTime)
  sprintf(buf, "Arbitrator did not let philosopher %i enter", num);
  Serial.println(buf);


  }}
  vTaskDelete(NULL);
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  char task_name[20];

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Dining Philosophers Challenge---");

  // Create kernel objects before starting tasks
  bin_sem = xSemaphoreCreateBinary();
  done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);
  for (int i = 0; i < NUM_TASKS; i++) {
    chopstick[i] = xSemaphoreCreateMutex();
  }

  arbitrator = xSemaphoreCreateMutex();

  // Have the philosphers start eating
  for (int i = 0; i < NUM_TASKS; i++) {
    sprintf(task_name, "Philosopher %i", i);
    xTaskCreatePinnedToCore(eat,
                            task_name,
                            TASK_STACK_SIZE,
                            (void *)&i,
                            1,
                            NULL,
                            app_cpu);
    xSemaphoreTake(bin_sem, mutexTime);
  }


  // Wait until all the philosophers are done
  for (int i = 0; i < NUM_TASKS; i++) {
    xSemaphoreTake(done_sem, mutexTime);
  }

  // Say that we made it through without deadlock
  Serial.println("Done! No deadlock occurred!");
}

void loop() {
  // Do nothing in this task
}