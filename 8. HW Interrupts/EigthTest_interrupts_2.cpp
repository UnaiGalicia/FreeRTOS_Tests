/*
 * HW Inteerupts
 * Microcontrollers usually integrate peripherals that can generate interrupts (timers, counters, push button...)
 * 
 * Remember that in FreeRTOS a HW interrupt will have greater than any other task, independent from that tasks priority level
 * 
 * We will assume that any HW interrupt will pause execution of any task and force processor to execute the associated interrupt servkice routine (ISR),
 * and when the ISR is done, execution will return to the task that was previously executing. In reality, an interrupt can generate another interrupt and so on (nested interrupts)
 *  
 * The ESP32 has 4 HW timers, each has a 16 bit preescaler and a 64 bit counter.
 * The default base timer clock is 80 MHz.
 * 
 * We will use a HW timer of the ESP32
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
static const uint16_t timer_divider = 8;   //REMEMBER THAT THE DIVIDER HAS TO FIT A 16 BIT VARIABLE
 /* clock divider/prescaler: 
  * If the clock is ticking at 80 MHz and we set a divider of 8,
  * The new clock will tick at 10 MHz 
  * 
  * So if we count 1,000,000 ticks of the new clock (10 MHz),
  * the interrupt will be made every 10 Hz
  */
static const uint64_t timer_max_count = 1000000;                //REMEMBER THAT THE COUNTER HAS TO FIT A 16 BIT VARIABLE
static const TickType_t task_delay = 2000 / portTICK_PERIOD_MS; //delay to allow the isr execute a few times before the task runs again
static const uint8_t pin = 25;

//Globals

//we're using the ESP32 HAL Timer
static hw_timer_t *timer = NULL;

//Simple integer counter - volatile is needed to tell the compiler that
//the value of the variable may change outside the scope of the currently executing task (in our case, inside an ISR)
//if else, the compiler might think that we aren't using the variable and delete it
static volatile int isr_counter;

//Special mutex to prevent tasks in the other core from entering into a critical section
//This mutex works like a normal mutex, but they cause any task that attemps to take them to loop forever,
//waiting for it available. Thats why it should be used with specially designated critical functions, as we dont want the current core to wait forever
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

//*****************************************
//Interrupt Service Routines - ISRs

//This function executes when timer reaches max (and resets)
//This is stored this in the internal RAM instead of FLASH for faster access
//For that, we use IRAM_ATTR attribute qualifier 
void IRAM_ATTR ontimer(){

    /*ESP-IDF VERSION OF A CRITICAL SECTION IN AN ISR
     * protect our critical section with spinlock + special function (portENTER...)
     * In addition to preventing tasks in the other core from taking the spinlock,
     * it disables all interrupts in the current core, preventing nested interrupts,
     * but might delay other interrupts that jump while the program is here
     * So, keep isrs and critical sections short
     * FreeRTOS docs recommends not to call other FreeRTOS API functions from a critical section
    */
    portENTER_CRITICAL_ISR(&spinlock);
    isr_counter++;
    portEXIT_CRITICAL_ISR(&spinlock);

    /*Vanilla FreeRTOS version:
     * UBaseType_t saved_int_status;
     * saved_int_status = taskENTER_CRITICAL_FROM_ISR();
     * isr_counter++;
     * taskEXIT_CRITICAL_FROM_ISR(saved_int_status);
    */ 

    /*ONLY USE FUNCTIONS THAT END BY "FROM_ISR" OR "_ISR" INSIDE
    ISR CRITICAL REGIONS*/
}

void printValues(void * parameters){

    while(1){

        while(isr_counter > 0){

            Serial.println(isr_counter);

            //ESP-IDF version of critical section (in a task)
            portENTER_CRITICAL(&spinlock);
            isr_counter--;
            portEXIT_CRITICAL(&spinlock);

            /*Much like a normal mutex, however, a mutex
            doesn't work here as it doesnt stop an interrupt.
            Instead, we use non ISR critical section functions
            to disable interrupts and prevent
            */
            /*Vanilla FreeRTOS version:
            * taskENTER_CRITICAL();
            * isr_counter--;
            * portEXIT_CRITICAL();
            */ 
            /* If a HW interrupt occurs while executing this critical section,
             * it will not be dropped, instead, the ISR will trigger as soon as
             * the current execution leaves the critical section and interrupts
             * are reenabled.*/
        }
        //wait two secs while ISR increments the counter a few times
        vTaskDelay(task_delay);

    }
}

void setup(){

    Serial.begin(115200);
    pinMode(pin, OUTPUT);

    //Wait a sec
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS HW Interrupts demo 2---");

    xTaskCreatePinnedToCore(printValues, "Print values", 1024, NULL, 1, NULL, app_cpu);

    //Create and start HW timer(num, divider, countUp)
    timer = timerBegin(0, timer_divider, true);

    //Provide ISR to timer (timer, callback function(executed when timer ends), edge)
    timerAttachInterrupt(timer, &ontimer, true);

    //At what count should ISR trigger (timer, count, autoreload (automatically reload after triggering the ISR))
    timerAlarmWrite(timer, timer_max_count, true);

    //Allow ISR to trigger
    timerAlarmEnable(timer);

    vTaskDelete(NULL);
}

void loop(){
    
}