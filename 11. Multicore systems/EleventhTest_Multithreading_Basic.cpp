/*
    Multicore systems

    Vanilla FreeRTOS does not support multicore execution. ESP-IDF lets using multicore.
    Other RTOSes might run differently (Scheduler, ...)

    AMP: Assymetric MultiProcessing
      Two or more cores that can communicate are needed
      One core is in charge of running the OS and sends out tasks/jobs to other cores
      The secondary cores may have their own OS to run tasks, but AMP requires one core that sends out the tasks.

    SMP: Symetrical MultiPorcessing
      Each core/processor is treated equally and runs the same OS
      A shared list of tasks can be accessed by all cores and each cores pulls from that list to figure out what to work on.
      Usually the same architecture is followed on each core

    Cores/processors have a shared memory and IO bus, where they all can access common memory and peripherals

    The ESP32 has 2 cores (Usually), which follow extensa lx6 architecture:
      - Protocol core (PRO_CPU, CPU 0): Intended to run tasks for various communication protovols like WiFi pr Bluetooth
        The intention is to have the user run their program in CPU1,  separate from the networking stacks in core 0.
        Sticking to this model while using WiFi or Bluetooth is important, in order to give comms a full proc, as a missed frame can mean missing a lot of data 
        However if not running these protocols, both cores can be used for any purpose.
      - Application core (APP_CPU, CPU 1)

    They have a shared bus to access mempry and peripherals

    The first 64 kB of shared instruction RAM is set aside to the cache.
    Cache looks shared, but in reality each core has its own memory:
      - The first 32 kB is cache for core 0
      - The second 32 kB is cache for core 1

    Each core runs an independent scheduler, each with its own tick timer. That means that tasks in core 0 will not execute in synch in core 1.
    The task control block, stack and code for each task exists in shared memory, so either core is capable of running any given task.

    In ESP-IDF we have the option of letting the scheduler in either core choose the higherst priority task in the ready state from a shared list or pinning a task to a core on creation.
    If we pin a task to a core, we are telling that that task should always run in a particular core.



    Again, some RTOS implementations may or may not have multicore support out of the box, but FreeRTOS does not.

    Each multicore processor is different, so checking documents that specify the multicore architecture is interesting.
    In ESP-IDF's RTOS SMP case: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/freertos-smp.html

    FreeRTOS maintains a list of tasks in the ready state. 
    This list is an array where each element corresponds to a priority level and contains a linked list of all the tasks, in the ready state, at that priority level.
    In ESP-IDF, this list is shared between the cores,. when the scheduler in each core is called, it looks at this list to determine which task should be run. 
    The task control block of each task contains a field that determines if it can run on core 0 or core 1 or no affinity.
    If that field matches the calling core or has no affinity, the core knows it can run that task.
    A shared index variable for each priority list is updated whenever a task is chosen by a scheduler.
    It is possible for schedulers to skip tasks when you mix pinned and unpinned tasks, because of this it becoimes very hard to predict which core will run each task when.
    While it is technically deterministic, it becomes nearly impossible to calculate the exact timing of events and instruction execution

    When a task is pinned to a core, its capable of relying on that cores cache to make it faster to work with pieces of data that get used often.
    If you assign no affinity to a task and it switches which core it runs on, the working data will no longer be in cache memory.
    As a result the core will need to spend more time fetching the data from main memopry and updatin the cache.

    These cache misses make it harder to predict the exact timing of events when we dont know when a task will switch cores.

    In some architectures, interrups might be shared among the cores. on the ESP32 each core has its own set of 32 interrupts.
    In code, you can assign an external event to trigger one of these interrupts, such as a pin level change or a dma buffer fillilng up.
    When an interrupt triggers it halts processing and its core  to run the associated ISR in that core.
    Once again, it becomes difcicult to know exactly which task was halted if we let the schedulers arbitrarily choose which task to run in each core.

    As we've already seen, each core contains a few internal peripherals that can trigger interrupts (timers...)
    These core-specific internal peripherlas cannot trigger interrupts in the other core

    This is FOR THE ESP32, OTHER UCONTROLLERS MIGHT BE DIFFERENT

  So, what's better, pinning tasks to cores or to let the scheduler decide?

    Benefits of letting schedulers in cores decide which task to run next:
     - Easier to use: just set the task to no affinity!
     - Easier way to get close to optimal core usage
     - Automatized load balancing
    
    Benefits of pinning a task to a core:
     - You'll know the location of interrupts unless all interrupts are shared.
     On the ESP32, if we create an interrupt in core 0, that interrupt will always run on ISR in core 0, regardless of where the created task is running when the interrupt happens
    - Gain more control usage (lose load balancing and optimization)
    - Fewer cache misses -> More optimal
    - More deterministic: you kniow which core is running which task


*/

#include <Arduino.h>
#include <stdlib.h>

//core definitions: PRO_CPU -> 0, APP_CPU 1
static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// settings
static const TickType_t time_hog = 200; //Time hogging the CPU.

//**************************************************************
//Functions

static void hog_delay(uint32_t ms) {
  for(uint32_t i=0; i<ms; i++){
    for(uint32_t j=0; j<40000; j++) 
      asm("nop");    
    }
}

//**************************************************************
//Tasks

//Task L (low priority)
void doTaskL(void*parameters) { 
  TickType_t timestamp;
  char str[20];

  while(1) {
    sprintf(str, "Task L, Core %i\r\n", xPortGetCoreID());
    Serial.print(str);

    hog_delay(time_hog);
    
    }
}

//Task H (High priority)
void doTaskH(void*parameters) { 
  TickType_t timestamp;
  char str[20];

  while(1) {
    sprintf(str, "Task H, Core %i\r\n", xPortGetCoreID());
    Serial.print(str);

    //Hog the processor for 200 ms while doing nothing (Bad idea)
    hog_delay(time_hog);  
    
    }
}

void setup(){

  Serial.begin(115200);

  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Multicore Demo---");


/*
TEST 1: Running low and high priority tasks in the same core

  The high priority task will take the processor for him and wont let it go, not letting task L runtime.
  Also, after some time, the ESP32 will reboot due to a special watchdog timer that is only in the core 0.
  The watchdog checks every few seconds if the scheduler is called. This prevents wireless communication tasks to enter in loops.
  This watchdog is not set to core 1 by default.

  xTaskCreatePinnedToCore(doTaskL, "Task L", 2048, NULL, 1, NULL, pro_cpu);
  xTaskCreatePinnedToCore(doTaskH, "Task H", 2048, NULL, 2, NULL, pro_cpu);
*/

/* TEST 2:

  tskNO_AFFINITY tells the task that it can run in either core.
  Each scheduler will find whichever highest priority task is in the ready state and run it.

  xTaskCreatePinnedToCore(doTaskL, "Task L", 2048, NULL, 1, NULL, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(doTaskH, "Task H", 2048, NULL, 2, NULL, tskNO_AFFINITY);
*/

/*Test 3: Running each task in different cores*/
  xTaskCreatePinnedToCore(doTaskL, "Task L", 2048, NULL, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(doTaskH, "Task H", 2048, NULL, 2, NULL, pro_cpu);


  vTaskDelete(NULL);

}

void loop(){

}