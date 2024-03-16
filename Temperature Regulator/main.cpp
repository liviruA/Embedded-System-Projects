#include "mbed.h"
#include "LCD_DISCO_F429ZI.h"
#include "TS_DISCO_F429ZI.h"
#include <random>
#include <math.h>
#define WAIT_TIME_MS 200
#define LCD_WIDTH 240
#define LCD_HEIGHT 320
#define plusMIDx 50 //Plus Midpoint X
#define plusMIDy 280 //Plus Midpoint Y
#define plusSIZE 60.0 //Plus sign size
#define minusMIDx 190  //Minus Midpoint X
#define minusMIDy 280  //Minus Midpoint Y
#define minusSIZE 60.0 //Minus sign size


LCD_DISCO_F429ZI LCD;
TS_DISCO_F429ZI TS;
PwmOut Fan(PD_14);



AnalogIn aIn(PA_0);
Ticker tmr;
TS_StateTypeDef tsState;
uint16_t tsX, tsY;
float aInput, volt, sensorTemp, threshTemp;
int FanPeriod = 300;
int pulseWidth = 0;

void displayLCD(){
    LCD.Clear(LCD_COLOR_WHITE);
    LCD.SetTextColor(LCD_COLOR_BLACK);
    uint8_t text[30];
    sprintf((char *)text, "Temperature:%.1f C", sensorTemp);
    LCD.DisplayStringAt(0, 80, (uint8_t *)&text, CENTER_MODE);
    sprintf((char *)text, "Threshold:%.1f C", threshTemp);
    LCD.DisplayStringAt(0, 120, (uint8_t *)&text, CENTER_MODE);
    LCD.SetTextColor(LCD_COLOR_BLACK);

    // plus sign
    LCD.FillRect(plusMIDx-plusSIZE/2+2.5, plusMIDy, plusSIZE, 5);
    LCD.FillRect(plusMIDx, plusMIDy-plusSIZE/2+2.5, 5, plusSIZE);

    // minus sign
    LCD.FillRect(minusMIDx-minusSIZE/2+2.5, minusMIDy, minusSIZE, 5);

}

void setThresh(){
    TS.GetState(&tsState);
    tsX = tsState.X;
    tsY = tsState.Y;
    if (tsState.TouchDetected) {
        if (tsX < LCD_WIDTH/2 && tsY < LCD_HEIGHT/4) {
            LCD.SetTextColor(LCD_COLOR_BLUE);
            LCD.FillRect(plusMIDx-plusSIZE/2-2.5, plusMIDy-plusSIZE/2-2.5, 70, 70);
            LCD.SetTextColor(LCD_COLOR_WHITE);
            LCD.FillRect(plusMIDx-plusSIZE/2+2.5, plusMIDy, plusSIZE, 5);
            LCD.FillRect(plusMIDx, plusMIDy-plusSIZE/2+2.5, 5, plusSIZE);
            threshTemp += 0.5;
        }
        else if (tsX > LCD_WIDTH/2 && tsY < LCD_HEIGHT/4) {
            LCD.SetTextColor(LCD_COLOR_RED);
            LCD.FillRect(minusMIDx-minusSIZE/2-2.5, minusMIDy-minusSIZE/2-2.5, 70, 70);
            LCD.SetTextColor(LCD_COLOR_WHITE);
            LCD.FillRect(minusMIDx-minusSIZE/2+2.5, minusMIDy, minusSIZE, 5);
            threshTemp -= 0.5;
        }
        else {
            return;
        }
    }
}

void FanControl(){
    FanPeriod = 350;
    Fan.period_us(FanPeriod);
    if (threshTemp<sensorTemp){
        if (abs(threshTemp-sensorTemp)>0.2) {
            pulseWidth = (int)(350)*abs(threshTemp-sensorTemp);
            Fan.pulsewidth_us(pulseWidth);
        }
        else{
            return;
        }
    }
    else {
        Fan.pulsewidth_us(0);
    } 
}


int main() {
    aIn.set_reference_voltage(3);
    volt = aIn.read_voltage();
    sensorTemp = 3*volt*100/(1024*10e-3);
    threshTemp = round(sensorTemp) + 1;
  
    while (true) {
        volt = aIn.read_voltage();
        sensorTemp= 3*volt*100/(1024*10e-3);
        displayLCD();
        setThresh();
        FanControl();
        thread_sleep_for(WAIT_TIME_MS);
    }
}
