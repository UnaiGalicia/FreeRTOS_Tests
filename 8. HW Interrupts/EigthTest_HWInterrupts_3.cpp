/*
    Now we will implement a binary semaphore within an ISR

    We'll have two tasks running at priority 1and priority 2:
        - Task A runs for while and idles
        - At some point task B (priority 2) interrupts and runs for a short
        However task b reaches a point were reaches a point were its waiting a semaphore so it goes into a blocking state
        Therefore, task A will be selected by the scheduler.
        So task A runs for while untila HW interrupt occurs, which calls an ISR

        The ISR uses a shared resource (global var...) and when its done it gives the semaphore
        This action along with a special API yield call will force the scheduler to run and see that task b has been unlocked as the semaphore is available (deferred interrupt)

        So task b is chosen by the scheduler. task b processes the data that the ISR made available

        This is be a deferred interrupt. the isr should be as short as possible, to defer the processing of the data to a task instead
        By computing in a task, we at least give other tasks the chance to run

        Then task B will quickly go to wait again for the semaphore, giving task A chance to run

*/

#include <Arduino.h>
#include <stdlib.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif


//Settings
//default base timer clk is 80 MHz
static const uint16_t timer_divider = 80;   //REMEMBER THAT THE DIVIDER HAS TO FIT A 16 BIT VARIABLE
 /* clock divider/prescaler: 
  * If the clock is ticking at 80 MHz and we set a divider of 80,
  * The new clock will tick at 1 MHz 
  * 
  * So if we count 1,000,000 ticks of the new clock (1 MHz),
  * the interrupt will be made every 1 Hz
  */
static const uint64_t timer_max_count = 1000000;                //REMEMBER THAT THE COUNTER HAS TO FIT A 16 BIT VARIABLE
static const TickType_t task_delay = 2000 / portTICK_PERIOD_MS; //delay to allow the isr execute a few times before the task runs again

//We will use an ADC, which is GPIO36 or A0 (physically VP)
static const uint8_t adc_pin = A0;

//Globals

//we're using the ESP32 HAL Timer
static hw_timer_t *timer = NULL;

//Store ADC value - volatile is needed to tell the compiler that
//the value of the variable may change outside the scope of the currently executing task (in our case, inside an ISR)
//if else, the compiler might think that we aren't using the variable and delete it
static volatile uint16_t val;

static SemaphoreHandle_t bin_sem = NULL;

//*****************************************
//Interrupt Service Routines - ISRs

//This function executes when timer reaches max (and resets)
//This is stored this in the internal RAM instead of FLASH for faster access
//For that, we use IRAM_ATTR attribute qualifier 
void IRAM_ATTR ontimer(){

    BaseType_t task_woken = pdFALSE;

    val = analogRead(adc_pin);

    //Give semaphore to tell tp the task that the new value is ready
    //Fromisr is used AS WE ARE IN A ISR. This function will never block
    //If we take an unavailable mutex or semaphore, it will be dealt with differently
    //The task woken parameter is another feature of these functions
    xSemaphoreGiveFromISR(bin_sem, &task_woken);

    //task_woken determines:will another task be unlocked because a mutex/seamphore was given?
    //if a higher priority task was unblocked because of this action, the scheduler should be called
    //to start running that task immediatly as soon as the ISR is done.
    //We record that fact in the boolean value "task_woken". which gets passed to the portYIELD_FROM_ISR() function

    /* xSemaphoreGiveFromISR() will set *pxHigherPriorityTaskWoken to pdTRUE if giving the semaphore caused a task to unblock, 
    and the unblocked task has a priority higher than the currently running task. 
    If xSemaphoreGiveFromISR() sets this value to pdTRUE then a context switch should be requested before the interrupt is exited.*/

    //Exit from ISR (in vanilla freertos the if is not necessary)
    if(task_woken){
        portYIELD_FROM_ISR();
    }


}

void printValues(void * parameters){

    //Loop forever, wait for semaphore, print value
    while(1){

        xSemaphoreTake(bin_sem, portMAX_DELAY);
        Serial.println(val);

    }
}

void setup(){

    Serial.begin(115200);
    

    //Wait a sec
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS HW Interrupts demo 3---");

    //create semaphore before it is used (in task or ISR)
    bin_sem = xSemaphoreCreateBinary();

    // Force reboot if we cant create semaphore
    if(bin_sem == NULL){
        Serial.println("ERROR: COULD NOT CREATE SEMAPHORE");
        ESP.restart();
    }

    //Priority 2, higher than setup/loop
    xTaskCreatePinnedToCore(printValues, "Print values", 1024, NULL, 2, NULL, app_cpu);

    //Create and start HW timer(num, divider, countUp)
    timer = timerBegin(0, timer_divider, true);

    //Provide ISR to timer (timer, callback function(executed when timer ends), edge)
    timerAttachInterrupt(timer, &ontimer, true);

    //At what count should ISR trigger (timer, count, autoreload (automatically reload after triggering the ISR))
    timerAlarmWrite(timer, timer_max_count, true);

    //Allow ISR to trigger
    timerAlarmEnable(timer);

    
}

void loop(){
    
}

/*

Newer versions of freertos have directed task notifications.
If you exactly know which task is waiting to use some piece of data,
you can use a notification instead of a semaphore to let it know.
The advantage is that these task notifications are faster and more efficient
*/