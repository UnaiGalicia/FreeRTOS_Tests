#include <iostream>
#include <getit.h>
#include <Arduino.h>
#include <string.h>

char* getStringUser(uint8_t size, uint8_t *tam){

  char buf[size], c, *str;
  uint8_t value=0;
  memset(buf, 0, size);
  bool end=false;

  *tam=0;

  while(!end){
    
    if(Serial.available()>0){
      
      c = Serial.read();
      Serial.print(c);
      
      

      if(c=='\n'){
        str = (char*)pvPortMalloc((*tam) * sizeof(char));
        if(str==NULL){
          *tam=-1;
        }
        else{
          *tam = strlcpy(str, buf, *tam);
          if(*tam==-1)
            *tam=-2;
          /*Serial.print("\nYour choice: ");
          Serial.println(str);*/
        }
        end=true;
      }

      else if((*tam-1)==size)
        Serial.println("Too long, press enter!");
      else{
        buf[*tam] = c;
        (*tam)++;
      }
        
    }

    
  }

  return str;

}