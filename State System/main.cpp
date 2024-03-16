#include "mbed.h"

InterruptIn userButton(BUTTON1);

Ticker flipper;

DigitalOut led3(PG_13);
DigitalOut led4(PG_14);

int state = 0; // default state on startup
int counter=0;
int counter1=0;

void SetState() {
  if (state != 1) {
    state = 1;
  } else {
    state = 2;
  }
}



void ButtonISR() {
    if (state == 0) {
      led3=0;
      led4 = !led4;
    } else if (state == 1) {
      if (counter1%2 == 0){
          led4=1;
          led3=0;
      }
      else if (counter1%2 == 1){
          led4=0;
          led3=1;
      }
      counter1++;

    } else {
      if (counter%4==0){
          led4=1;
          led3=0;
      }
      else if (counter%4==1){
          led4=0;
          led3=0;
      }
      else if (counter%4==2){
          led4=0;
          led3=1;
      }
      else if (counter%4==3){
          led4=0;
          led3=0;
      }
      counter++;
    }
}

int main() {
  userButton.fall(&SetState);
  __enable_irq();
  flipper.attach(&ButtonISR, 1s);

  while (true) {
    
  }
}

