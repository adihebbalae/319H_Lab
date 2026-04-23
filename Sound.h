// Sound.h
// Runs on MSPM0
// Play sounds on 5-bit DAC.
// Your name
// 11/5/2023
#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

#define MUSIC_TRACK_COUNT 3

// initialize a 11kHz SysTick, however no sound should be started
// initialize any global variables
// Initialize the 5 bit DAC
// This is called once
void Sound_Init(void);

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
void Sound_Start(const uint8_t *pt, uint32_t count);

// following 8 functions do not output to the DAC
// they configure pointers/counters and initiate the sound by calling Sound_Start
void Sound_Shoot(void);
void Sound_Killed(void);
void Sound_Explosion(void);
void Sound_Fastinvader1(void);
void Sound_Fastinvader2(void);
void Sound_Fastinvader3(void);
void Sound_Fastinvader4(void);
void Sound_Highpitch(void);
uint8_t Sound_Done(void);

// ---- Background music streaming from SD card ----
// Call Music_Init() after ST7735_InitR() and disk_initialize() in main.
// Call Music_Task() every iteration of the main loop to refill buffers.
void Music_Init(void);   // mount FatFS, open file, preload buffers
void Music_Start(void);  // begin streaming
void Music_Stop(void);   // pause streaming, output silence
uint8_t Music_Reset(void);  // restart current track from the beginning
void Music_Task(void);   // refill ping-pong buffer — call from main loop
void Music_SelectTrack(uint8_t track); // switch to track 0..MUSIC_TRACK_COUNT-1
uint8_t Music_GetTrack(void);
uint8_t Music_GetTrackCount(void);

// Music volume — shift applied to each 8-bit sample when mixed to 12-bit DAC.
// 4 = unity (default), 3 = half amplitude, 5 = 2x (may clip). Valid range 1..5.
void Music_SetVolume(uint8_t shift);

// Short "piece moved" SFX — first 880 samples (~80 ms) of shoot.
// Brief enough that the music dip is imperceptible.
void Sound_Move(void);

#endif
