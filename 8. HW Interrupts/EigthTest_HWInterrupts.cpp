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
static const uint64_t timer_max_count = 1000000;    //REMEMBER THAT THE COUNTER HAS TO FIT A 16 BIT VARIABLE
static const uint8_t pin = 25;

//were using the ESP32 HAL Timer
static hw_timer_t *timer = NULL;

//*****************************************
//Interrupt Service Routines - ISRs

//This function executes when timer reaches max (and resets)
//This is stored this in the internal RAM instead of FLASH for faster access
//For that, we use IRAM_ATTR attribute qualifier 
void IRAM_ATTR ontimer(){

    //Toggle LED
    int pin_state = digitalRead(pin);
    digitalWrite(pin, !pin_state);

}

void setup(){

    pinMode(pin, OUTPUT);

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