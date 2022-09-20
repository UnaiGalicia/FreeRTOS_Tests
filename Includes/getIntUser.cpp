#include <iostream>
#include <getit.h>
#include <Arduino.h>
#include <string.h>
#include <errno.h>

const uint8_t len=32;



uint16_t getIntUser(){
  
  errno = 0;

  char buf[len], *ptr, c;
  uint16_t  value=0;
  uint8_t ind=0;
  memset(buf, 0, len);
  bool check = false, end=false;


  while(!end){
    
    if(Serial.available()>0){
      
      c = Serial.read();
      Serial.print(c);
      check = isDigit(c);
      
      if(c=='\n'){
        /*Serial.print("\nYour choice: ");
        Serial.println(buf);*/
        //Serial.println("start Again");
        value = (uint16_t)(strtol(buf, &ptr, 10));
        if(errno==0){
            memset(buf, 0, len);
            ind=0;
            end=true;
        }
        else{
            fprintf(stderr, "%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
      }

      else{

        if(check){
          if(ind<(len-1)){
            buf[ind]=c;
            ++ind;
          }
          else
            Serial.println("Too long, press enter!");
            
        }
      }  
    }
  }

    return value;

}

/*
When communicating with a computer, using serial comms, the computer sends to the ESP
information bit-to-bit. Then, the UART chip of the ESP assembles eight bits into a byte,
and stores these bytes in the serial receive buffer, which can store 64 bytes max.

Arduino library Serial.read() function reads the FIRST AVAILABLE BYTE FROM THE SERIAL 
RECEIVE BUFFER, and once readed, it removes it from the buffer. That's why serial read returns
a char, takes one byte at a time. 

Remember that serial comms can end with CR or LF, so its a good idea to check if these were received.

To know if the serial receive buffer has anything inside, Serial.available() can be used.
This functions tells us how many bytes are available to be read in the buffer.

*/