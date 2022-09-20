


#include <Arduino.h>
#include <cstdlib>
#include <stdlib.h>
#include <getit.h>
#include <string.h>
#include <errno.h>

typedef struct{

    char msg[20];
    uint8_t num;

}blink;


// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Settings
static const uint8_t msg_queue_len = 5; //Max number of items the queue can hold
static const uint8_t pin = 25;
static const uint8_t times = 100;
//Globals
static QueueHandle_t queue1;
static QueueHandle_t queue2;
static const char def[]="delay ";   //our command, note the const

//Task: wait for item in the queue and print it
void terminalTask(void *parameters){
    
    size_t cells;
    uint16_t num = 0;
    uint8_t len=20, tam=0;
    char *cmd, *ms, *eptr;
    blink item;
    
    while(1){
        
        if(xQueueReceive(queue2, (void*)&item, 0) == pdTRUE){
            Serial.println("Task1 received: ");
            Serial.printf("\t%s\n", item.msg);
            Serial.printf("\t%u\n", item.num);
        }

        Serial.print("Enter command: ");

        cmd = getStringUser(len, &tam);
        
        //Serial.println(tam);

        if(strncmp(cmd, def, 6)==0){  //returns 0 if equal
            
            cells = tam-7;
            
            ms = (char*)pvPortMalloc((cells) * sizeof(char));
            memcpy(ms, cmd+6, cells);
            num = (uint16_t)strtol(ms, &eptr, 10);
            vPortFree(ms);
            
            if(errno==0){
                if((xQueueSend(queue1, (void*)&num, 10) != pdTRUE))
                    Serial.println("Queue full");
                else
                    vPortFree(cmd);
            }

            else{
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        
        else
            Serial.println("Command not supported.");

        
        
        

        //vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}

void blinkTask(void*parameters){

    uint8_t k=0;
    uint16_t t=0;
    bool start=false;
    blink b;

    while(1){

        if(xQueueReceive(queue1, (void *)&t, 0) == pdTRUE){
            // Best practice: use only one task to manage serial comms
            strcpy(b.msg, "Message received ");
            b.num = 1;
            xQueueSend(queue2, (void *)&b, 10);
            k=0;
            start = true;
        }
        if(start){
            digitalWrite(pin, HIGH);
            vTaskDelay(t/portTICK_PERIOD_MS);
            digitalWrite(pin, LOW);
            vTaskDelay(t/portTICK_PERIOD_MS);

            k++;

            if(k==times){
                b.num = times;
                strcpy(b.msg, "blinked");
                /*if((xQueueSend(queue2, (void*)&b, 10) != pdTRUE))
                 Serial.println("Queue full");*/
                //good practice to only allow one task to manage serial comms
                xQueueSend(queue2, (void*)&b, 10);

            }
        }
    }
}

void setup(){
    Serial.begin(115200);

    pinMode(pin, OUTPUT);

    Serial.println();
    Serial.println("---FreeRTOS Queue demo---");

    queue1 = xQueueCreate(msg_queue_len, sizeof(uint16_t));
    queue2 = xQueueCreate(msg_queue_len, sizeof(blink));

    xTaskCreatePinnedToCore(terminalTask, "Terminal task", 1500, NULL, 1, NULL, app_cpu);
    xTaskCreatePinnedToCore(blinkTask, "Blink task", 1500, NULL, 1, NULL, app_cpu);

    vTaskDelete(NULL);
}

void loop(){

}