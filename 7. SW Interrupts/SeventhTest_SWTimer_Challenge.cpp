#include <Arduino.h>
#include <stdlib.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Globals
static const int pin = 25;
static const TickType_t delay = (5000/portTICK_PERIOD_MS);
static TimerHandle_t auto_reload_timer = NULL;
//Has to be auto-reload, as it will restart at any given time

//Callback functions
void myTimerCallback(TimerHandle_t xTimer){
    digitalWrite(pin, LOW);
}

void echoTask(void*parameters){
    char c;
    while(1){
        if(Serial.available()>0){
            c = Serial.read();
            Serial.print(c);
            digitalWrite(pin, HIGH);
            xTimerReset(auto_reload_timer, portMAX_DELAY);
        }
    }

}

void setup(){
    Serial.begin(115200);
    pinMode(pin, OUTPUT);

    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS Timer challenge---");

    auto_reload_timer = xTimerCreate(
                        "Auto-reload timer",           //Name of timer
                        delay,  //Period of timer (2000 ms) (in ticks), (milliseconds/porttick_period_ms => in ticks)
                        pdTRUE,                     // Auto-reload: Allows the timer to continuaally expire and execute the callback. We set it as pdTRUE
                        (void*)1,                   // Timer ID: pointer to something, if we want to create an unique ID. Has to be casted to void*
                        myTimerCallback);           //Callback function, leave the same

    xTaskCreatePinnedToCore(echoTask, "Echo Task", 1024, NULL, 1, NULL, app_cpu);

    if(auto_reload_timer == NULL)
        Serial.println("Could not create one of the timers...");
    else{
        //Wait and then start timer
        vTaskDelay(1000/portTICK_PERIOD_MS);
        Serial.println("Starting timers...");

        // Start timer (max block time if command queue is full, REMEMBER THAT TIMERS USE TST, THAT IS CONTROLLED BY A QUEUE!!)
        
        xTimerStart(auto_reload_timer, portMAX_DELAY);
        
    }
    
    vTaskDelete(NULL);

}

void loop(){
    
}