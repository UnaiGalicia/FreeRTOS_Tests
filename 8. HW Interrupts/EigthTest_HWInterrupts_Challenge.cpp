/*
    This program uses 2 tasks and one HW interrupt
        - The HW interrupt takes samples @ 10Hz from the ADC pin
        When it takes 10 samples it unblocks task A

        - Task A (averageCalc) calculates the average of the 10 samples
        and stores it in a global variable (avg)

        - Task B (terminal) reads Serial and waits for the 'avg' user command
        when introduced, the task prints the average.

        The used circular buffer only stores 20 samples. If the user
        does not introduce the 'avg' command at time, avgs are lost.
*/

#include <Arduino.h>
#include <stdlib.h>
#include <getit.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif


//Settings
//default base timer clk is 80 MHz
static const uint16_t timer_divider = 80;       //REMEMBER THAT THE DIVIDER HAS TO FIT A 16 BIT VARIABLE
 /* clock divider/prescaler: 
  * If the clock is ticking at 80 MHz and we set a divider of 80,
  * The new clock will tick at 1 MHz 
  * 
  * So if we count 100,000 ticks of the new clock (1 MHz),
  * the interrupt will be made every 10 Hz
  */
static const uint64_t timer_max_count = 100000; //REMEMBER THAT THE COUNTER HAS TO FIT A 16 BIT VARIABLE

//We will use an ADC, which in ESP32 is GPIO36 or A0 (physically VP)
static const uint8_t adc_pin = A0;


//Globals

//we're using the ESP32 HAL Timer
static hw_timer_t *timer = NULL;

static SemaphoreHandle_t bin_sem = NULL;
static SemaphoreHandle_t avgMutex = NULL;

enum {TAM = 20};                    //Circular buffer's size
enum{MSG_LEN = 100};
static uint16_t circBuf[TAM];       //Circular buffer   
static uint8_t rd, wr;              //Circular buffer's head and tail
static bool fullFlag=false;         //Flag to tell that buffer is full
static volatile uint8_t count=0;    //Volatile!!
static float avg=0;                 //store the avg of samples

/* Circular buffer: 3 rules
    - When data is added, wr advances
    - When data is consumed, rd advances
    - If the end of the buffer is reached, pointers will wrap around to the beginning
        This is achieved with modulus operation.
*/

//************************************************************
//Interrupt Service Routines - ISRs

void IRAM_ATTR ontimer(){

    BaseType_t task_woken = pdFALSE;

    if(!fullFlag){
        circBuf[wr] = analogRead(adc_pin);
        
        wr = (wr+1)%TAM;
        count++;

        if(wr == rd)
            fullFlag = true;
    }

    if(count == 10){
        //Serial.println("hi!")
        count = 0;
        xSemaphoreGiveFromISR(bin_sem, &task_woken);
        /*If we count up to 10, we'll unblock the task that calculates
        the averago of the 10 samples and restart the counter*/       
    }

    if(task_woken){
        portYIELD_FROM_ISR();
    }

}

//************************************************************
//Functions

bool isBufferEmpty(void){

    if((rd == wr) && !fullFlag)
        return true;
    else
        return false;   

}

uint16_t readCircBuf(){
    
    uint16_t tmp;
    tmp = circBuf[rd];
    rd = (rd+1)%TAM;
    fullFlag = false;
    return tmp;

}

//************************************************************
//FreeRTOS TASKS

void averageCalc(void * parameters){

    //Loop forever, wait for semaphore, print value
    while(1){
        
        //Take the sem. that th interrupt wil give us
        //when 10 samples are available in the circ. buffer
        xSemaphoreTake(bin_sem, portMAX_DELAY);
        
        if(isBufferEmpty()){
            Serial.println("Buffer is empty.");
        }
        else{
            //avg is a shared resource between this and the "terminal" task
            //mutex to keep it safe.
            xSemaphoreTake(avgMutex, portMAX_DELAY);

            for (short i = 9; i >= 0; --i)
                avg += readCircBuf();
            
            avg /= 10;

            //return average mutex
            xSemaphoreGive(avgMutex);
        }

    }
}

void terminal(void *parameters){

    char *str;
    uint8_t size;
    
    while(1){

        str = getStringUser(MSG_LEN, &size);
        
        //if the avg command is received, print the average
        if(strcmp(str, "avg")==0){
            Serial.print("Average: ");
            xSemaphoreTake(avgMutex, portMAX_DELAY);
            Serial.println(avg);
            xSemaphoreGive(avgMutex);
        }

        vPortFree(str); //free the command string
        // Don't hog the CPU. Yield to other tasks for a while
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void setup(){

    Serial.begin(115200);
    

    //Wait a sec to make sure that we see everything
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    Serial.println();
    Serial.println("---FreeRTOS HW Interrupts challenge---");

    //create semaphore before it is used (in task or ISR)
    bin_sem = xSemaphoreCreateBinary();
    avgMutex = xSemaphoreCreateMutex();

    // Force reboot if we cant create semaphore
    if(bin_sem == NULL || avgMutex == NULL){
        Serial.println("ERROR: COULD NOT CREATE SEMAPHORE");
        ESP.restart();
    }

    //Create both tasks
    xTaskCreatePinnedToCore(terminal, "Terminal task", 1024, NULL, 1, NULL, app_cpu);
    xTaskCreatePinnedToCore(averageCalc, "Calculate avg", 1024, NULL, 1, NULL, app_cpu);

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
    //Never reached
}
