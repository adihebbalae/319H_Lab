// Sound.cpp
// Runs on MSPM0
// Sound assets in sounds/sounds.h
// your name
// your data 
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "Sound.h"
#include "sounds/sounds.h"
#include "../inc/DAC5.h"
#include "../inc/Timer.h"


#define SOUND_PERIOD  7256  // 80,000,000 / 11,025 Hz ≈ 7256

// 5-bit 32-element sine wave from dac.xls
const uint8_t SinWave[32] = {
  16, 19, 22, 24, 27, 28, 30, 31, 31, 31, 30, 
  28, 27, 24, 22, 19, 16, 13, 10,  8,  5,  4, 
   2,  1,  1,  1,  2,  4,  5,  8, 10, 13
};

// Global state
const uint8_t *SoundPt;      // pointer to current sound array
uint32_t SoundCount;         // samples remaining
uint32_t Index = 0;   

void SysTick_IntArm(uint32_t period, uint32_t priority){
  // write this
  SysTick->CTRL = 0;
  SysTick->LOAD = period - 1;
  SCB->SHP[1] &= 0x00FFFFFF;
  SCB->SHP[1] |= (priority << 30);
}
// initialize a 11kHz SysTick, however no sound should be started
// initialize any global variables
// Initialize the 5 bit DAC
void Sound_Init(void){
  DAC5_Init();
  SysTick_IntArm(SOUND_PERIOD, 2);  // arm at 11kHz, priority 2
  SysTick->VAL = 0;
  SysTick->CTRL = 0x07;
 
}
extern "C" void SysTick_Handler(void);

void SysTick_Handler(void){ // called at 11 kHz
  // output one value to DAC if a sound is active
  if (SoundCount > 0) {
    DAC5_Out(SinWave[Index] >> 3);
    Index = (Index + 1) & 0x1F;     // Increment index and wrap around to 0 when it reaches 32
    SoundCount--;
    if (SoundCount == 0) {
      DAC5_Out(16);
    }
  }
}

//******* Sound_Start ************
// This function does not output to the DAC. 
// Rather, it sets a pointer and counter, and then enables the SysTick interrupt.
// It starts the sound, and the SysTick ISR does the output
// feel free to change the parameters
// Sound should play once and stop
// Input: pt is a pointer to an array of DAC outputs
//        count is the length of the array
// Output: none
// special cases: as you wish to implement
void Sound_Start(const uint8_t *pt, uint32_t count){
  SoundPt = pt;
  SoundCount = count;
  Index = 0;
}

void Sound_Shoot(void){
// write this
  Sound_Start( shoot, 4080);
}
void Sound_Killed(void){
// write this

}
void Sound_Explosion(void){
// write this

}

void Sound_Fastinvader1(void){

}
void Sound_Fastinvader2(void){

}
void Sound_Fastinvader3(void){

}
void Sound_Fastinvader4(void){

}
void Sound_Highpitch(void){

}
