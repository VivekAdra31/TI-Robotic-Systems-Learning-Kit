// RSLK Self Test via UART

/* This example accompanies the books
   "Embedded Systems: Introduction to the MSP432 Microcontroller",
       ISBN: 978-1512185676, Jonathan Valvano, copyright (c) 2017
   "Embedded Systems: Real-Time Interfacing to the MSP432 Microcontroller",
       ISBN: 978-1514676585, Jonathan Valvano, copyright (c) 2017
   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
       ISBN: 978-1466468863, , Jonathan Valvano, copyright (c) 2017
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/

Simplified BSD License (FreeBSD License)
Copyright (c) 2017, Jonathan Valvano, All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are
those of the authors and should not be interpreted as representing official
policies, either expressed or implied, of the FreeBSD Project.
*/

#include "msp.h"
#include <stdint.h>
#include <string.h>
#include "..\inc\UART0.h"
#include "..\inc\EUSCIA0.h"
#include "..\inc\FIFO0.h"
#include "..\inc\Clock.h"
//#include "..\inc\SysTick.h"
#include "..\inc\SysTickInts.h"
#include "..\inc\CortexM.h"
#include "..\inc\TimerA1.h"
//#include "..\inc\Bump.h"
#include "..\inc\BumpInt.h"
#include "..\inc\LaunchPad.h"
#include "..\inc\Motor.h"
#include "../inc/IRDistance.h"
#include "../inc/ADC14.h"
#include "../inc/LPF.h"
#include "..\inc\Reflectance.h"
#include "../inc/TA3InputCapture.h"
#include "../inc/Tachometer.h"

#define P2_4 (*((volatile uint8_t *)(0x42098070)))
#define P2_3 (*((volatile uint8_t *)(0x4209806C)))
#define P2_2 (*((volatile uint8_t *)(0x42098068)))
#define P2_1 (*((volatile uint8_t *)(0x42098064)))
#define P2_0 (*((volatile uint8_t *)(0x42098060)))

// At 115200, the bandwidth = 11,520 characters/sec
// 86.8 us/character
// normally one would expect it to take 31*86.8us = 2.6ms to output 31 characters
// Random number generator
// from Numerical Recipes
// by Press et al.
// number from 0 to 31
uint32_t Random(void){
static uint32_t M=1;
  M = 1664525*M+1013904223;
  return(M>>27);
}
char WriteData,ReadData;
uint32_t NumSuccess,NumErrors;
void TestFifo(void){char data;
  while(TxFifo0_Get(&data)==FIFOSUCCESS){
    if(ReadData==data){
      ReadData = (ReadData+1)&0x7F; // 0 to 127 in sequence
      NumSuccess++;
    }else{
      ReadData = data; // restart
      NumErrors++;
    }
  }
}
uint32_t Size;
int Program5_1(void){
//int main(void){
    // test of TxFifo0, NumErrors should be zero
  uint32_t i;
  Clock_Init48MHz();
  WriteData = ReadData = 0;
  NumSuccess = NumErrors = 0;
  TxFifo0_Init();
  TimerA1_Init(&TestFifo,43);  // 83us, = 12kHz
  EnableInterrupts();
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      TxFifo0_Put(WriteData);
      WriteData = (WriteData+1)&0x7F; // 0 to 127 in sequence
    }
    Clock_Delay1ms(10);
  }
}

char String[64];
uint32_t MaxTime,First,Elapsed;
int Program5_2(void){
//int main(void){
    // measurement of busy-wait version of OutString
  uint32_t i;
  DisableInterrupts();
  Clock_Init48MHz();
  UART0_Init();
  WriteData = 'a';
  SysTick_Init(0x1000000,2); //OHL - using systick INT api
  MaxTime = 0;
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      String[i] = WriteData;
      WriteData++;
      if(WriteData == 'z') WriteData = 'a';
    }
    String[i] = 0; // null termination
    First = SysTick->VAL;
    UART0_OutString(String);
    Elapsed = ((First - SysTick->VAL)&0xFFFFFF)/48; // usec

    if(Elapsed > MaxTime){
        MaxTime = Elapsed;
    }
    UART0_OutChar(CR);UART0_OutChar(LF);
    Clock_Delay1ms(100);
  }
}

int Program5_3(void){
//int main(void){
    // measurement of interrupt-driven version of OutString
  uint32_t i;
  DisableInterrupts();
  Clock_Init48MHz();
  EUSCIA0_Init();
  WriteData = 'a';
  SysTick_Init(0x1000000,2); //OHL - using systick INT api
  MaxTime = 0;
  EnableInterrupts();
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      String[i] = WriteData;
      WriteData++;
      if(WriteData == 'z') WriteData = 'a';
    }
    String[i] = 0; // null termination
    First = SysTick->VAL;
    EUSCIA0_OutString(String);
    Elapsed = ((First - SysTick->VAL)&0xFFFFFF)/48; // usec
    if(Elapsed > MaxTime){
        MaxTime = Elapsed;
    }
    EUSCIA0_OutChar(CR);EUSCIA0_OutChar(LF);
    Clock_Delay1ms(100);
  }
}
int Program5_4(void){
//int main(void){
    // demonstrates features of the EUSCIA0 driver
  char ch;
  char string[20];
  uint32_t n;
  DisableInterrupts();
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  EUSCIA0_Init();     // initialize UART
  EnableInterrupts();
  EUSCIA0_OutString("\nLab 5 Test program for EUSCIA0 driver\n\rEUSCIA0_OutChar examples\n");
  for(ch='A'; ch<='Z'; ch=ch+1){// print the uppercase alphabet
     EUSCIA0_OutChar(ch);
  }
  EUSCIA0_OutChar(LF);
  for(ch='a'; ch<='z'; ch=ch+1){// print the lowercase alphabet
    EUSCIA0_OutChar(ch);
  }
  while(1){
    EUSCIA0_OutString("\n\rInString: ");
    EUSCIA0_InString(string,19); // user enters a string
    EUSCIA0_OutString(" OutString="); EUSCIA0_OutString(string); EUSCIA0_OutChar(LF);

    EUSCIA0_OutString("InUDec: ");   n=EUSCIA0_InUDec();
    EUSCIA0_OutString(" OutUDec=");  EUSCIA0_OutUDec(n); EUSCIA0_OutChar(LF);
    EUSCIA0_OutString(" OutUFix1="); EUSCIA0_OutUFix1(n); EUSCIA0_OutChar(LF);
    EUSCIA0_OutString(" OutUFix2="); EUSCIA0_OutUFix2(n); EUSCIA0_OutChar(LF);

    EUSCIA0_OutString("InUHex: ");   n=EUSCIA0_InUHex();
    EUSCIA0_OutString(" OutUHex=");  EUSCIA0_OutUHex(n); EUSCIA0_OutChar(LF);
  }
}
// =========TACHOMETER ================
#define P2_4 (*((volatile uint8_t *)(0x42098070)))
#define P2_3 (*((volatile uint8_t *)(0x4209806C)))
#define P2_2 (*((volatile uint8_t *)(0x42098068)))
#define P2_1 (*((volatile uint8_t *)(0x42098064)))
#define P2_0 (*((volatile uint8_t *)(0x42098060)))

uint16_t Period0;              // (1/SMCLK) units = 83.3 ns units
uint16_t First0;               // Timer A3 first edge, P10.4
int Done0;                     // set each rising
// max period is (2^16-1)*83.3 ns = 5.4612 ms
// min period determined by time to run ISR, which is about 1 us
void PeriodMeasure0(uint16_t time){
  P2_0 = P2_0^0x01;           // thread profile, P2.0
  Period0 = (time - First0)&0xFFFF; // 16 bits, 83.3 ns resolution
  First0 = time;                   // setup for next
  Done0 = 1;
}
uint16_t Period2;              // (1/SMCLK) units = 83.3 ns units
uint16_t First2;               // Timer A3 first edge, P8.2
int Done2;                     // set each rising
// max period is (2^16-1)*83.3 ns = 5.4612 ms
// min period determined by time to run ISR, which is about 1 us
void PeriodMeasure2(uint16_t time){
  P2_2 = P2_2^0x01;           // thread profile, P2.4
  Period2 = (time - First2)&0xFFFF; // 16 bits, 83.3 ns resolution
  First2 = time;                   // setup for next
  Done2 = 1;
}

void TimedPause(uint32_t time){
  Clock_Delay1ms(time);          // run for a while and stop
  Motor_Stop();
  while(LaunchPad_Input()==0);  // wait for touch
  while(LaunchPad_Input());     // wait for release
}

#define PERIOD 1000  // must be even

void toggle_GPIO(void){
    P2_4 ^= 0x01;     // create output
}

void IR_Move(int left,int center,int right)
{
    if(left<=80 && left != 0)
    {
        LaunchPad_Output(0x01);
        Motor_Backward(3000,3000);
        Clock_Delay1ms(1000);
        Motor_Right(3000,3000);
        Clock_Delay1ms(1000);
        Motor_Forward(3000,3000);

    }
    if( right <=80 && right !=0)
    {
        LaunchPad_Output(0x02);
        Motor_Backward(3000,3000);
        Clock_Delay1ms(1000);
        Motor_Left(3000,3000);
        Clock_Delay1ms(1000);
        Motor_Forward(3000,3000);
    }
    if(center <=80 && center != 0)
    {
        LaunchPad_Output(0x04);
        Motor_Backward(3000,3000);
                Clock_Delay1ms(1000);
                Motor_Left(3000,3000);
                Clock_Delay1ms(1000);
                Motor_Forward(3000,3000);
    }
    else
    {
        Motor_Forward(3000,3000);
        Clock_Delay1ms(1000);
    }
}

uint32_t main_count=0;
// =========TACHOMETER ================
/*void LaunchPad_Init(void){
  P1->SEL0 &= ~0x13;
  P1->SEL1 &= ~0x13;    // 1) configure P1.4 and P1.1 as GPIO
  P1->DIR &= ~0x12;     // 2) make P1.4 and P1.1 in
  P1->DIR |= 0x01;      //    make P1.0 out
  P1->REN |= 0x12;      // 3) enable pull resistors on P1.4 and P1.1
  P1->OUT |= 0x12;      //    P1.4 and P1.1 are pull-up
  P2->SEL0 &= ~0x07;
  P2->SEL1 &= ~0x07;    // 1) configure P2.2-P2.0 as GPIO
  P2->DIR |= 0x07;      // 2) make P2.2-P2.0 out
  P2->DS |= 0x07;       // 3) activate increased drive strength
  P2->OUT &= ~0x07;     //    all LEDs off
}*/


//------------LaunchPad_Output------------
// Output to LaunchPad LEDs
// Input: 0 off, bit0=red,bit1=green,bit2=blue
// Output: none
/*void LaunchPad_Output(uint8_t data){  // write three outputs bits of P2
  P2->OUT = (P2->OUT&0xF8)|data;
}*/
// =========BUMP=======================
uint8_t CollisionData, CollisionFlag, TempData;  // mailbox
void HandleCollision(uint8_t bumpSensor){
   CollisionData = bumpSensor;
   TempData = CollisionData;
   CollisionFlag = 1;
   P4->IFG &= ~0xED;                  // clear interrupt flags (reduce possibility of extra interrupt)
}

void MotorMovt(void){
    static uint32_t count=0;
    static uint8_t motor_state=0;

    for(int i = 0; i < 6; i++){

          if(TempData%2 == 0 && CollisionData != 63) {
              //EUSCIA0_OutString("The switch that just got pressed is");
              //UART0_OutUDec5("The switch that just got pressed is ");

              UART0_OutUDec5(i);
              EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
          }
          TempData/=2;
      }

    //Write this as part of lab3 Bonus Challenge
    if(CollisionData == 63 || CollisionData == 0){
        Motor_Forward(3000, 3000);
        CollisionData = 63;
    }
    else if(CollisionData == 51) {
        Motor_Backward(3000, 3000);
        Clock_Delay1ms(1000);
        Motor_Right(3000, 3000);
        Clock_Delay1ms(600);
        CollisionData = 63;
        Motor_Forward(3000, 3000);
    }
    else if((CollisionData > 56) && (CollisionData < 63)) {
        Motor_Backward(3000, 3000);
        Clock_Delay1ms(1000);
        Motor_Left(3000, 3000);
        Clock_Delay1ms(600);
        CollisionData = 63;
        Motor_Forward(3000, 3000);
    }
    else if((CollisionData <= 55) && (CollisionData > 6)) {
        Motor_Backward(3000, 3000);
        Clock_Delay1ms(1000);
        Motor_Right(3000, 3000);
        Clock_Delay1ms(600);
        CollisionData = 63;
        Motor_Forward(3000, 3000);
    }
}
// =========BUMP=======================

void RSLK_Reset(void){
    DisableInterrupts();

    LaunchPad_Init();
    //Initialise modules used e.g. Reflectance Sensor, Bump Switch, Motor, Tachometer etc
    // ... ...

    EnableInterrupts();
}

uint8_t Data;
uint32_t Position;

volatile uint32_t nr,nc,nl;
volatile uint32_t  ADCflag;
void SensorRead_ISR(void){  // runs at 2000 Hz
  uint32_t raw17,raw12,raw16;
  P1OUT ^= 0x01;         // profile
  P1OUT ^= 0x01;         // profile
  ADC_In17_12_16(&raw17,&raw12,&raw16);  // sample
  nr = LPF_Calc(raw17);  // right is channel 17 P9.0
  nc = LPF_Calc2(raw12); // center is channel 12, P4.1
  nl = LPF_Calc3(raw16); // left is channel 16, P9.1
  ADCflag = 1;           // semaphore
  P1OUT ^= 0x01;         // profile
}


// RSLK Self-Test
int main(void) {
  uint32_t cmd=0xDEAD, menu=0;

  DisableInterrupts();
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  //SysTick_Init(48000,2);  // set up SysTick for 1000 Hz interrupts
  Motor_Init();
  //Motor_Stop();
  LaunchPad_Init();
  //Bump_Init();
  //Bumper_Init();
  //IRSensor_Init();
  //Tachometer_Init();
  EUSCIA0_Init();     // initialize UART
  EnableInterrupts();

  while(1){                     // Loop forever
      // write this as part of Lab 5
      EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("RSLK Testing"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[0] RSLK Reset"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[1] Motor Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[2] IR Sensor Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[3] Bumper Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[4] Reflectance Sensor Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[5] Tachometer Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[6] Extra IR Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);

      EUSCIA0_OutString("CMD: ");
      cmd=EUSCIA0_InUDec();
      EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);

      switch(cmd){
          case 0:
              RSLK_Reset();
              menu =1;
              cmd=0xDEAD;
              break;
          case 1:
              EUSCIA0_OutString("Motor Forward 3 Seconds (Timer)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              Motor_Forward(3000, 3000);
              Clock_Delay1ms(3000);
              EUSCIA0_OutString("Motor Backward 3 Seconds (Timer)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              Motor_Backward(3000,3000);
              Clock_Delay1ms(3000);
              EUSCIA0_OutString("Motor Left 4 Seconds (Timer)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              Motor_LW(3000,3000);
              Clock_Delay1ms(2000);
              Motor_LWB(3000,3000);
              Clock_Delay1ms(2000);
              EUSCIA0_OutString("Motor Right 4 Seconds (Timer)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              Motor_RWF(3000,3000);
              Clock_Delay1ms(2000);
              Motor_RWB(3000,3000);
              Clock_Delay1ms(2000);
              Motor_Stop();
              break;
          case 2:
                uint32_t raw17,raw12,raw16;
                int32_t n; uint32_t s;
                Clock_Init48MHz();  //SMCLK=12Mhz
                ADCflag = 0;
                s = 256; // replace with your choice
                ADC0_InitSWTriggerCh17_12_16();   // initialize channels 17,12,16
                ADC_In17_12_16(&raw17,&raw12,&raw16);  // sample
                LPF_Init(raw17,s);     // P9.0/channel 17
                LPF_Init2(raw12,s);    // P4.1/channel 12
                LPF_Init3(raw16,s);    // P9.1/channel 16
                UART0_Init();          // initialize UART0 115,200 baud rate
                LaunchPad_Init();
                TimerA1_Init(&SensorRead_ISR,250);    // 2000 Hz sampling
                for(n=0; n<2000; n++){
                      while(ADCflag == 0){};
                      ADCflag = 0; // show every 2000th point
                    }
                while(1){
                    UART0_OutUDec5(LeftConvert(nl));UART0_OutString(" mm (Left)");
                    UART0_OutUDec5(CenterConvert(nc));UART0_OutString(" mm (Center)");
                    UART0_OutUDec5(RightConvert(nr));UART0_OutString(" mm (Right)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                    Clock_Delay1ms(1000);
                }
              break;
          case 3:
              DisableInterrupts();
                Clock_Init48MHz();   // 48 MHz clock; 12 MHz Timer A clock
                LaunchPad_Init(); // built-in switches and LEDs
                // write this as part of Lab 14, section 14.4.4 Integrated Robotic System
                CollisionFlag = 0;
                Motor_Init();        // activate Lab 14 software
                BumpInt_Init(&HandleCollision);
                TimerA1_Init(&MotorMovt,50000);  // 10 Hz
                //TExaS_Init(LOGICANALYZER_P4_765320);

                TimedPause(500);

                EnableInterrupts();
                while(1){
                  WaitForInterrupt();


                }
              break;
          case 4:
              Clock_Init48MHz();

              while(1){
                Data = Reflectance_Read(1000);
                Position = Reflectance_Position(Data);

                for(int i=0;i<8;i++)
                {
                    if(Data%2==1)
                    {
                        UART0_OutUDec5(0);
                    }
                    else
                    {
                        UART0_OutUDec5(1);
                    }
                    Data/=2;
                }
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                Clock_Delay1ms(10);
              }
              break;
          case 5:
              DisableInterrupts();
                Clock_Init48MHz();   // 48 MHz clock; 12 MHz Timer A clock
                LaunchPad_Init(); // built-in switches and LEDs
                //P2->SEL0 &= ~0x15;
                //P2->SEL1 &= ~0x15;   // configure P2.0 and P2.2 as GPIO
                //P2->DIR |= 0x15;     // P2.0 and P2.2 outputs
                P2->SEL0 &= ~0x11;
                P2->SEL1 &= ~0x11;  // configure P2.0 and P2.4 as GPIO
                P2->DIR |= 0x11;    // P2.0 and P2.4 outputs
                First0 = First2 = 0; // first will be wrong
                Done0 = Done2 = 0;   // set on subsequent
                Motor_Init();        // activate Lab 13 software
                TimerA1_Init(&toggle_GPIO,10);    // 50Khz sampling
                TimerA3Capture_Init(&PeriodMeasure0,&PeriodMeasure2);

                Motor_Forward(3000,3000); // 50%
                EnableInterrupts();
                while(1){
                //    P2_4 ^= 0x01;     // create output
                //    Clock_Delay1us(PERIOD/2);
                  WaitForInterrupt();
                  main_count++;
                  if(main_count%1000){
                      UART0_OutString("Period0 = ");UART0_OutUDec5(Period0);UART0_OutString(" Period2 = ");UART0_OutUDec5(Period2);EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  }
                }
              break;
          case 6:
          {
              LaunchPad_Init();
              uint32_t raw17,raw12,raw16;
                              int32_t n; uint32_t s;
                              Clock_Init48MHz();  //SMCLK=12Mhz
                              ADCflag = 0;
                              s = 256; // replace with your choice
                              ADC0_InitSWTriggerCh17_12_16();   // initialize channels 17,12,16
                              ADC_In17_12_16(&raw17,&raw12,&raw16);  // sample
                              LPF_Init(raw17,s);     // P9.0/channel 17
                              LPF_Init2(raw12,s);    // P4.1/channel 12
                              LPF_Init3(raw16,s);    // P9.1/channel 16
                              UART0_Init();          // initialize UART0 115,200 baud rate
                              LaunchPad_Init();
                              TimerA1_Init(&SensorRead_ISR,250);    // 2000 Hz sampling
                              for(n=0; n<2000; n++){
                                    while(ADCflag == 0){};
                                    ADCflag = 0; // show every 2000th point
                                  }
                              while(1){
                                  int32_t left= LeftConvert(nl);
                                  int32_t center = CenterConvert(nc);
                                  int32_t right = RightConvert(nr);

                                  IR_Move(left,center,right);

                              }
                            break;
          }
          default:
              menu=1;
              break;
      }

      if(!menu)Clock_Delay1ms(3000);
      else{
          menu=0;
      }

      // ....
      // ....
  }
}
