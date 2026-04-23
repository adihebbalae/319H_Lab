// Sound.cpp
// Runs on MSPM0
// Sound assets in sounds/sounds.h
// Background music streamed from SD card using ping-pong buffers.
// Pattern from Valvano SDCFile/SDCFile.c main5 (8-bit streaming to DAC).
// your name
// your data 
#include <stdint.h>
#include <string.h>
#include <ti/devices/msp/msp.h>
#include "Sound.h"
#include "sounds/sounds.h"
// #include "../inc/DAC5.h"
#include "../inc/DAC.h"
#include "../inc/Timer.h"
#include "../SDCFile/ff.h"
#include "../SDCFile/diskio.h"
#include "LED.h"

#define SOUND_PERIOD  7256  // 80,000,000 / 11,025 Hz ≈ 7256

// ---- ping-pong music buffers (Valvano main5 pattern) ----
// 8192 bytes = 744ms of audio per buffer at 11025 Hz.
// Refills every ~22 frames instead of every 1.4 frames (512-byte buffer).
// RAM cost: 2×8192 = 16 KB out of 32 KB. Free RAM after all globals: ~13 KB — safe.
// Do NOT go higher: next doubling (16KB each) leaves <5KB for stack+heap.
#define MUSIC_BUFSIZE  8192
static uint8_t  MusicBufA[MUSIC_BUFSIZE];
static uint8_t  MusicBufB[MUSIC_BUFSIZE];
static uint8_t * volatile front8; // buffer SysTick is currently reading
static uint8_t * volatile back8;  // buffer main loop is currently filling
static volatile uint32_t Count8;  // index into front8
static volatile int flag8;    // 1 = main loop needs to refill back8
static volatile uint8_t MusicPlaying; // 1 = streaming active

static FATFS  MusicFS;
static FIL    MusicFile;
static uint8_t MusicMounted;
static uint8_t MusicFileOpen;
static uint8_t MusicTrack = 0;
static uint8_t MusicNeedsReset;
static const char * const MusicTrackFiles[MUSIC_TRACK_COUNT] = {
  "TETRIS8.BIN",
  "TETRISB.BIN",
  "TETRISC.BIN"
};

// ---- sound-effect state (flash arrays) ----
static const uint8_t * volatile SoundPt;
volatile uint32_t SoundCount;
static volatile uint32_t SoundIndex;

// ---- music volume (shift count applied to 8-bit samples -> 12-bit DAC) ----
static volatile uint8_t MusicShift = 4;   // 4 = unity gain

static void MusicFillSilence(uint8_t *buf, uint32_t len){
  memset(buf, 0x80, len);  // 8-bit unsigned PCM midpoint
}

void SysTick_IntArm(uint32_t period, uint32_t priority){
  SysTick->CTRL  = 0x00;
  SysTick->LOAD  = period - 1;
  SCB->SHP[1]    = (SCB->SHP[1] & (~0xC0000000)) | priority << 30;
  SysTick->VAL   = 0;
  SysTick->CTRL  = 0x07;
}

void Sound_Init(void){
  // DAC5_Init();
  DAC_Init();
  SoundPt    = 0;
  SoundCount = 0;
  SoundIndex = 0;
  front8     = MusicBufB;  // buffer being output to DAC
  back8      = MusicBufA;  // buffer being filled from SD
  MusicFillSilence(MusicBufA, MUSIC_BUFSIZE);
  MusicFillSilence(MusicBufB, MUSIC_BUFSIZE);
  Count8     = 0;
  flag8      = 0;
  MusicPlaying = 0;
  MusicMounted = 0;
  MusicFileOpen = 0;
  MusicNeedsReset = 0;
  SysTick_IntArm(SOUND_PERIOD, 0);  // priority 0 = highest — DAC output must never be delayed
  SysTick->VAL  = 0;
  SysTick->CTRL = 0x07;
}

extern "C" void SysTick_Handler(void);

void SysTick_Handler(void){ // called at 11025 Hz
  // sound effects take priority over music
  if(SoundCount > 0){
    // DAC5_Out(SoundPt[SoundIndex] >> 3);  // 8-bit (0-255) → 5-bit (0-31)
    DAC_Out((uint32_t)SoundPt[SoundIndex] << 3); // 8-bit (0-255) → 12-bit (0-4080)
    SoundIndex++;
    SoundCount--;
    // if(SoundCount == 0) DAC5_Out(16); // 5-bit DAC midpoint = 16 for silence
    if(SoundCount == 0) DAC_Out(2048); // 12-bit DAC midpoint = 2048 for silence
    return;
  }
  if(MusicPlaying){
    // TETRIS8.BIN contains 8-bit samples (0-255): scale to 12-bit by MusicShift.
    // 4 = unity, 3 = half amplitude (quieter title screen), 5 = 2x (may clip).
    uint32_t sample = ((uint32_t)front8[Count8] << MusicShift);
    if(sample > 4095) sample = 4095;
    DAC_Out(sample);
    Count8 += 1;
    if(Count8 >= MUSIC_BUFSIZE){
      Count8 = 0;
      uint8_t *pt = front8;
      front8 = back8;
      back8  = pt;   // swap buffers
      flag8  = 1;    // signal main loop to refill back8
    }
  }
}

uint8_t Sound_Done(void){ 
  return SoundCount == 0; 
}

void Sound_Start(const uint8_t *pt, uint32_t count){
  SoundCount = 0;      // stop ISR from using stale data during update
  SoundPt    = pt;
  SoundIndex = 0;
  SoundCount = count;  // arm ISR last
}

// ---- Music streaming ----

static void MusicCloseFile(void){
  if(MusicFileOpen){
    f_close(&MusicFile);
    MusicFileOpen = 0;
  }
}

static uint8_t MusicEnsureOpen(uint8_t track){
  if(MusicFileOpen){
    return 1;
  }
  if(f_open(&MusicFile, MusicTrackFiles[track], FA_READ) != FR_OK){
    return 0;
  }
  MusicFileOpen = 1;
  return 1;
}

static uint8_t MusicReadLooping(uint8_t track, uint8_t *buf, uint32_t len){
  uint32_t total = 0;
  while(total < len){
    UINT br = 0;
    if(!MusicEnsureOpen(track)){
      return 0;
    }
    if(f_read(&MusicFile, buf + total, len - total, &br) != FR_OK){
      MusicCloseFile();
      return 0;
    }
    total += br;
    if(total >= len){
      return 1;
    }
    MusicCloseFile();
  }
  return 1;
}

static uint8_t MusicLoadTrack(uint8_t track){
  uint8_t wasPlaying = MusicPlaying;
  MusicPlaying = 0;
  DAC_Out(2048);
  MusicCloseFile();
  if(!MusicReadLooping(track, MusicBufB, MUSIC_BUFSIZE)){
    return 0;
  }
  if(!MusicReadLooping(track, MusicBufA, MUSIC_BUFSIZE)){
    return 0;
  }
  front8 = MusicBufB;
  back8  = MusicBufA;
  Count8 = 0;
  flag8  = 0;
  MusicTrack = track;
  MusicNeedsReset = 0;
  MusicPlaying = wasPlaying;
  return 1;
}

void Music_Init(void){
  if(f_mount(&MusicFS, "", 0) != FR_OK) return;  // D2/D3 stay OFF = mount failed
  MusicMounted = 1;
  if(!MusicLoadTrack(MusicTrack)){
    MusicMounted = 0;
    f_mount(0, "", 0);
    return;  // D2 stays OFF = file not found (check filename + SD card root)
  }
  LED_On(1<<16);  // D2 ON = file opened OK
}

void Music_Start(void){
  if(!MusicMounted) return;
  if(MusicNeedsReset && !MusicLoadTrack(MusicTrack)){
    return;
  }
  MusicPlaying = 1;
}

void Music_Stop(void){
  MusicPlaying = 0;
  MusicNeedsReset = 1;
  // DAC5_Out(16);
  DAC_Out(2048);
}

uint8_t Music_Reset(void){
  if(!MusicMounted){
    return 0;
  }
  return MusicLoadTrack(MusicTrack);
}

// Call every main-loop iteration — refills back8 when ISR has swapped buffers.
// Mirrors Valvano's main5 while(1) { if(flag8){...} } pattern.
void Music_Task(void){
  if (!MusicMounted || !MusicPlaying) {
    return;
  }
  if (flag8){
    flag8 = 0;
    LED_On(1<<17);   // D3 ON while reading from SD card
    uint8_t ok = MusicReadLooping(MusicTrack, back8, MUSIC_BUFSIZE);
    LED_Off(1<<17);  // D3 OFF when read complete
    if(!ok){
      // Do not rewind from inside the refill loop. If the SD read path is flaky,
      // automatic reset causes the song to restart over and over. Play silence
      // for this buffer and require an explicit restart instead.
      MusicFillSilence(back8, MUSIC_BUFSIZE);
      MusicCloseFile();
      MusicPlaying = 0;
      MusicNeedsReset = 1;
      DAC_Out(2048);
    }
  }
}

void Music_SelectTrack(uint8_t track){
  if(track >= MUSIC_TRACK_COUNT){
    track = 0;
  }
  if(!MusicMounted){
    MusicTrack = track;
    return;
  }
  if(track == MusicTrack){
    return;
  }
  uint8_t previous = MusicTrack;
  if(!MusicLoadTrack(track)){
    MusicLoadTrack(previous);
  }
}

uint8_t Music_GetTrack(void){
  return MusicTrack;
}

uint8_t Music_GetTrackCount(void){
  return MUSIC_TRACK_COUNT;
}

// ---- Sound effect helpers ----

void Sound_Shoot(void){
  Sound_Start(shoot, 4080);
}
void Sound_Killed(void){
  Sound_Start(invaderkilled, 3377);
}
void Sound_Explosion(void){
  Sound_Start(explosion, 2000);
}
void Sound_Fastinvader1(void){
  Sound_Start(fastinvader1, sizeof(fastinvader1));
}
void Sound_Fastinvader2(void){
  Sound_Start(fastinvader2, sizeof(fastinvader2));
}
void Sound_Fastinvader3(void){
  Sound_Start(fastinvader3, sizeof(fastinvader3));
}
void Sound_Fastinvader4(void){
  Sound_Start(fastinvader4, sizeof(fastinvader4));
}
void Sound_Highpitch(void){
  Sound_Start(highpitch, 1802);
}

// Short movement cue — first ~80 ms (880 samples @ 11025 Hz) of the shoot sample.
// Used on horizontal move / rotate; keeps the music dip imperceptible.
void Sound_Move(void){
  Sound_Start(shoot, 880);
}

// Range-checked so bad values can't drive the DAC into heavy clipping or silence.
void Music_SetVolume(uint8_t shift){
  if(shift < 1) shift = 1;
  if(shift > 5) shift = 5;
  MusicShift = shift;
}
