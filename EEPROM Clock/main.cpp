#include "DebouncedInterrupt.h"
#include "LCD_DISCO_F429ZI.h"
#include "mbed.h"
#include <time.h>

// initalizing PINS and assigning hexadecimal value for Microcontroller
#define SDA_PIN PC_9
#define SCL_PIN PA_8
#define EEPROM_ADDR 0xA0


Ticker ticker1; //ticker for later use
LCD_DISCO_F429ZI LCD; // intializing LCD
static bool updateLCD;
I2C i2c(SDA_PIN, SCL_PIN); //integer intergrated circuit 

//Initializing user and external buttons -> specific physical pins 
InterruptIn user_button(BUTTON1);
DebouncedInterrupt pbutton1(PA_5);
DebouncedInterrupt pbutton2(PA_6);
DebouncedInterrupt pbutton3(PA_7);


//Prewritten functions for EEPROM
void WriteEEPROM(int address, unsigned int ep_address, char *data, int size);
void ReadEEPROM(int address, unsigned int ep_address, char *data, int size);

//Three main states - show clock time, adjust time, or show saved time
typedef enum {clock_Time, adjust_Time, saved_Time} State_Manager;
static State_Manager curST;


typedef enum { no_Activity, record_Time, display_record, read_RealTime, set_Time, save_Time} Activity;
//Defining Functions
void None();
void ReadRealTime();
void SetTime();
void saveTime();
void RecordTime();
void DisplayRecord();

// order matters
static void (*possible_Activity[])(void) = {None, RecordTime, DisplayRecord, ReadRealTime, SetTime, saveTime};
static Activity curAC;

// Manipulating Times
static int curTimePointer;
static int totalSaved = 2;
static int curEE_Location = 0;
static unsigned int EE_Location[2] = {0,10000};
static time_t tempTime;
static tm *timeDetail;

void InitStateMachine();

//Button Function Definitions 

void Pbutton1ISR(){
    if (curST == clock_Time){
        
        curAC = set_Time; //we move to the activity level where we can alter the time 
        curTimePointer = 0; //start on Hour
        updateLCD = true; 
    } 
    else if (curST == adjust_Time){
        curAC = save_Time;
    }
}

void Pbutton2ISR(){
    if (curST == adjust_Time){
        
    curTimePointer = (curTimePointer + 1) % 2;
    curAC = set_Time;
    updateLCD = true;

    }
}

void Pbutton3ISR(){
    if (curST == adjust_Time){
       if (curTimePointer == 0){ //if we are pointing to hours
           timeDetail->tm_hour = (timeDetail->tm_hour +1)%24;
       } 
       else { //otherwise pointing to minutes
           timeDetail->tm_min = (timeDetail->tm_min +1)%60;
       }
       updateLCD=true;
       curAC = set_Time; 
    } else if (curST == saved_Time) {
        curST = clock_Time;
        curAC = no_Activity;
    
    } else if (curST == clock_Time) {
        curAC = display_record;
        updateLCD = true;
    }
   
}

//record time when the user button is pressed
void UserButtonISR() {
    curAC = record_Time;
    }

void None() {
//Do nothing 
    } 

void ReadRealTime() {
    
    char store_Time[20];
    time(&tempTime);
    timeDetail = localtime(&tempTime); //storing timeDetails for possible Manipulation 
    
    //Setting LCD screen vars
    LCD.Clear(LCD_COLOR_WHITE);
    LCD.SetBackColor(LCD_COLOR_WHITE);
    
    //Setting text vars
    LCD.SetTextColor(LCD_COLOR_DARKBLUE);
    LCD.SetFont(&Font24);
    
    //setting placement
    LCD.DisplayStringAt(0,40, (uint8_t *)"HH:MM:SS",CENTER_MODE); 
    strftime(store_Time, 20, "%H:%M:%S", localtime(&tempTime));
    LCD.DisplayStringAt(0,80, (uint8_t *)&store_Time, CENTER_MODE);
   
    //at the end of functional code, we set our state and action to follow
    curST = clock_Time;
    curAC = no_Activity;
}

void saveTime() {
    timeDetail->tm_sec = 0; //time is always set to 0 

    set_time(mktime(timeDetail)); //saving time to the time we set

    //at the end of functional code, we set our state and action to follow
    curST = clock_Time;
    curAC = no_Activity;
}

void SetTime() {
    if (updateLCD) { //checking if bool is true in order to execute
        
        char store_Time[20];

        //LCD screen vars
        LCD.Clear(LCD_COLOR_WHITE);
        LCD.SetBackColor(LCD_COLOR_WHITE);
        
        //setting text vars
        LCD.SetFont(&Font24);
        LCD.SetTextColor(LCD_COLOR_DARKBLUE);

        //setting placement of title
        LCD.DisplayStringAt(0,80, (uint8_t *)"Set Time:",LEFT_MODE);
        
        if (curTimePointer ==0) { //SETTING HIGHLIGHT COLOR
            LCD.SetBackColor(LCD_COLOR_BLUE);
        }
        //setting placement of seconds
        strftime(store_Time, 20,"Hour: %H",timeDetail);
        LCD.DisplayStringAt(0,120, (uint8_t *)&store_Time, LEFT_MODE);

        if (curTimePointer == 1) { //HIGHLIGHT COLOR
            LCD.SetBackColor(LCD_COLOR_BLUE);
        }
        else{
            LCD.SetBackColor(LCD_COLOR_WHITE);
        } 
        //setting placement of seconds
        strftime(store_Time, 20, "Minute: %M", timeDetail);
        LCD.DisplayStringAt(0,150, (uint8_t *)&store_Time, LEFT_MODE);

        updateLCD = false;
    }
    
    //at the end of functional code, we set our state and action to follow
    curST = adjust_Time;
    curAC = no_Activity;
}

void RecordTime(){
    char store_Time[20]; //array to store time for EEprom

    //storing time in given format
    strftime(store_Time, 20, "%H:%M:%S", timeDetail);

    //using the prewritten write prom function, we store the formatted time value in given address 
    WriteEEPROM(EEPROM_ADDR, EE_Location[curEE_Location],store_Time,20);

    //swithcing the location so that we dont overwrite time and have a max of two storage spaces
    if (curEE_Location == 0) {
        curEE_Location = 1;
    } else {
        curEE_Location = 0;
    }

    //incrementing values saved so we can keep track
    totalSaved++; 

    //at the end of functional code, we set our state and action to follow
    curST = clock_Time;
    curAC = no_Activity;
}

void DisplayRecord(){

    if (updateLCD) {
        char recorded_Data[20];

        // setting LCD vars
        LCD.Clear(LCD_COLOR_WHITE);
        LCD.SetBackColor(LCD_COLOR_WHITE);
        
        // setting text vars
        LCD.SetFont(&Font24);
        LCD.SetTextColor(LCD_COLOR_DARKBLUE);

        //Reading from our stored memory address
        ReadEEPROM(EEPROM_ADDR, EE_Location[0], recorded_Data,20);
        
        //Displaying Times
        LCD.DisplayStringAt(0, 80, (uint8_t *)"Recorded Time:", CENTER_MODE);
        LCD.DisplayStringAt(0, 120, (uint8_t *)&recorded_Data, CENTER_MODE);

        if (totalSaved>1){ //if more than one stored value 
            ReadEEPROM(EEPROM_ADDR, EE_Location[1], recorded_Data, 20); //read second value
            LCD.DisplayStringAt(0, 150, (uint8_t *)&recorded_Data, CENTER_MODE);
        }
        updateLCD = false;
    }

    //at the end of functional code, we set our state and action to follow
    curST = saved_Time;
    curAC = no_Activity;
}


//Working Clock
void updateTime_ISR() {
    if (curST == clock_Time) {
        curAC = read_RealTime;
    }
}
 
void InitStateMachine(){
    curTimePointer = 0;
    updateLCD = false;
    ticker1.attach(&updateTime_ISR, 1s);
    curST = clock_Time;
    curAC = read_RealTime;
}

//Prewritten code ------------------------------------------------------------------------------------------------------------------

void WriteEEPROM(int address, unsigned int eeaddress, char *data, int size) {
  char i2cBuffer[size + 2];
  i2cBuffer[0] = (unsigned char)(eeaddress >> 8);   // MSB
  i2cBuffer[1] = (unsigned char)(eeaddress & 0xFF); // LSB

  for (int i = 0; i < size; i++) {
    i2cBuffer[i + 2] = data[i];
  }

  int result = i2c.write(address, i2cBuffer, size + 2, false);
  thread_sleep_for(6);
}

void ReadEEPROM(int address, unsigned int eeaddress, char *data, int size) {
  char i2cBuffer[2];
  i2cBuffer[0] = (unsigned char)(eeaddress >> 8);   // MSB
  i2cBuffer[1] = (unsigned char)(eeaddress & 0xFF); // LSB

  // Reset eeprom pointer address
  int result = i2c.write(address, i2cBuffer, 2, false);
  thread_sleep_for(6);

  // Read eeprom
  i2c.read(address, data, size);
  thread_sleep_for(6);
}

//----------------------------------------------------------------------------------------------------------------------

int main() {
  
  __enable_irq();

  //enable buttons
  pbutton1.attach(&Pbutton1ISR, IRQ_FALL,200,false);
  pbutton2.attach(&Pbutton2ISR, IRQ_FALL,200,false);
  pbutton3.attach(&Pbutton3ISR, IRQ_FALL,200,false);
  user_button.fall(&UserButtonISR);
  
  // set RTC to January 1, 2024, 00:00:00 (HH:MM:SS)
  tm t;
  t.tm_year = 124; // years since 1900
  t.tm_mon = 0;
  t.tm_mday = 0;
  t.tm_hour = 0;
  t.tm_min = 0;
  t.tm_sec = 0;
  set_time(mktime(&t));

  //start clock
  InitStateMachine();

  while (1) {
   possible_Activity[curAC]();
  }
}

