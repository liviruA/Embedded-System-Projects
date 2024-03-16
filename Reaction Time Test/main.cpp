#include "LCD_DISCO_F429ZI.h"
#include "fonts.h"
#include "mbed.h"
#include <random>
#include <iostream>

LCD_DISCO_F429ZI LCD;
DigitalOut led3(PG_13);
DigitalIn userButton(BUTTON1);
Ticker flipper;
InterruptIn interrupt(BUTTON1);
Timeout StateDelay;
Timer timer1;
InterruptIn external_button(PA_6, PullUp);
uint32_t elapsed_ms;
uint32_t best_time = 10000;

int state = 1;
int randNum;

void ledFlasher() {
    led3 = !led3;
}


void StateManager() {
    if (state==0) {
        state++;
    }
    else if (state==1) {
        state++;
    }
    else {
        state=0;
    }
}

void state0() {
    flipper.attach(&ledFlasher, 100ms);
    StateManager();
}

void timer(){
    uint8_t line1[30];
    uint8_t line2[30];
    timer1.start();
    while (1) {
        if (userButton.read()==1) {
            timer1.stop();
            elapsed_ms = chrono::duration_cast<chrono::milliseconds>(timer1.elapsed_time()).count();
            break;
        }
    }
    if (best_time > elapsed_ms) {
        LCD.Clear(LCD_COLOR_WHITE);
        best_time = elapsed_ms;
        sprintf((char *)line1, "Best time: %d ms", best_time);
        sprintf((char *)line2, "Current time: %d ms", elapsed_ms);
        LCD.DisplayStringAt(0, 40, (uint8_t *)&line1, CENTER_MODE);
        LCD.DisplayStringAt(0, 80, (uint8_t *)&line2, CENTER_MODE);
    }
    else {
        LCD.Clear(LCD_COLOR_WHITE);
        sprintf((char *)line1, "Best time: %d ms", best_time);
        sprintf((char *)line2, "Current time: %d ms", elapsed_ms);
        LCD.DisplayStringAt(0, 40, (uint8_t *)&line1, CENTER_MODE);
        LCD.DisplayStringAt(0, 80, (uint8_t *)&line2, CENTER_MODE);
    }
    timer1.reset();
}

void state2() {
    LCD.DisplayStringAt(0, 40, (uint8_t *)"Go! Go! Go!", CENTER_MODE);
    led3=1;
    timer();
}

void state1() {
    LCD.Clear(LCD_COLOR_WHITE);
    led3=0;
    flipper.detach();
    randNum = (rand()%5+1)*1000;
    StateDelay.attach(&StateManager, 400ms);
    thread_sleep_for(randNum);
    if (state==2) {
        state2();
    }

}

void check_false_press() {
    if (state==2) {
        state=0;
        flipper.attach(&ledFlasher, 100ms);
    }
}

void SetState() {
    if (state==1) {
        state1();
    }
    else if (state==0) {
        state0();
    }
}

void master_reset() {
    state=1;
    flipper.attach(&ledFlasher, 100ms);
    best_time=0;
}


int main() {
  LCD.SetFont(&Font16);
  LCD.SetTextColor(LCD_COLOR_DARKBLUE);
  flipper.attach(&ledFlasher, 100ms);
  external_button.fall(&master_reset);
  interrupt.fall(&check_false_press);
  while(1) {
    if (userButton.read()==1){
        SetState();
    }
  }

}
