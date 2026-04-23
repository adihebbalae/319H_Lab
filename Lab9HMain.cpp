// Lab9HMain.cpp
// Runs on MSPM0G3507
// Lab 9 ECE319H
// Your name
// Last Modified: January 12, 2026

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/SlidePot.h"
#include "../inc/DAC5.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"
#include "images/tetris_images.h"
#include "Joystick.h"
#include "sounds/sounds.h"
#include "Game.h"
extern "C" void __disable_irq(void);
extern "C" void __enable_irq(void);
extern "C" void TIMG12_IRQHandler(void);
extern uint32_t SoundCount; //sound test

// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}

uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}

SlidePot Sensor(1500,0); // copy calibration from Lab 7
static volatile uint8_t JoyClickEdge = 0;
static uint8_t JoyClickPrev = 1;

// game engine runs at 30 Hz — samples inputs and sets semaphore for main loop
void TIMG12_IRQHandler(void){
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
    uint32_t clickNow;
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    JoyX   = Joystick_ReadX();
    JoyY   = Joystick_ReadY();
    SwEdge = Switch_InEdge();   // latches rising edges (debounced)
    clickNow = Joystick_ReadClick();
    JoyClickEdge = (JoyClickPrev == 1) && (clickNow == 0);
    JoyClickPrev = (uint8_t)clickNow;
    GPIOB->DOUTTGL31_0 = GREEN;
    Semaphore = 1;
    GPIOB->DOUTTGL31_0 = GREEN;
  }
}
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};
typedef enum {
  UI_HI = 0,
  UI_LANG_LABEL,
  UI_MUSIC_LABEL,
  UI_PLAY_LABEL,
  UI_MUSIC_OFF,
  UI_MUSIC_TRACK_A,
  UI_MUSIC_TRACK_B,
  UI_MUSIC_TRACK_C,
  UI_MUSIC_AUTO,
  UI_CLASSIC,
  UI_TUTORIAL,
  UI_LEARN_CONTROLS,
  UI_CLICK_HARD_DROP,
  UI_MOVE,
  UI_DROP,
  UI_ROTATE_CW,
  UI_ROTATE_CCW,
  UI_STICK_SIDEWAYS,
  UI_STICK_DOWN,
  UI_PRESS_SW5,
  UI_PRESS_SW3,
  UI_SW1_PAUSE_MENU,
  UI_SW5_START_GAME,
  UI_DO_ALL_4_STEPS,
  UI_PAUSED,
  UI_MAIN_MENU,
  UI_SW1_RESUME,
  UI_GAME,
  UI_OVER,
  UI_ONE_LINE,
  UI_LESSON,
  UI_DONE,
  UI_TRY,
  UI_AGAIN,
  UI_COUNT
} UiText_t;

static const char * const UiEnglish[UI_COUNT] = {
  "HI:",
  "LANG:",
  "MUSIC:",
  "PLAY:",
  "Off",
  "Track A",
  "Track B",
  "Track C",
  "Auto",
  "Classic",
  "Tutorial",
  "Learn controls",
  "click = hard drop",
  "MOVE",
  "DROP",
  "ROTATE CW",
  "ROTATE CCW",
  "stick sideways",
  "stick down",
  "press SW5",
  "press SW3",
  "SW1: pause/menu",
  "SW5: start game",
  "Do all 4 steps",
  "PAUSED",
  "MAIN MENU",
  "SW1: resume",
  "GAME",
  "OVER",
  "1 LINE",
  "LESSON",
  "DONE",
  "TRY",
  "AGAIN"
};

static const char * const UiSpanish[UI_COUNT] = {
  "MAX:",
  "IDIOMA:",
  "MUSICA:",
  "JUGAR:",
  "Apagada",
  "Pista A",
  "Pista B",
  "Pista C",
  "Auto",
  "Clasico",
  "Tutorial",
  "Aprende controles",
  "click = caida total",
  "MOVER",
  "BAJAR",
  "GIRA CW",
  "GIRA CCW",
  "stick lateral",
  "stick abajo",
  "pulsa SW5",
  "pulsa SW3",
  "SW1: pausa/menu",
  "SW5: iniciar",
  "Haz 4 pasos",
  "PAUSA",
  "MENU PRINC",
  "SW1: seguir",
  "FIN",
  "JUEGO",
  "1 LINEA",
  "LECCION",
  "LISTA",
  "INTENTA",
  "DE NUEVO"
};
// ============================================================
// Title screen — shared across main3c and main6
// ============================================================

typedef enum { PLAY_CLASSIC = 0, PLAY_TUTORIAL = 1 } PlayMode_t;
typedef enum { MENU_LANG = 0, MENU_MUSIC = 1, MENU_PLAY = 2, MENU_COUNT = 3 } MenuItem_t;
typedef enum {
  MUSIC_MODE_OFF = 0,
  MUSIC_MODE_TRACK_A,
  MUSIC_MODE_TRACK_B,
  MUSIC_MODE_TRACK_C,
  MUSIC_MODE_AUTO,
  MUSIC_MODE_COUNT
} MusicMode_t;

static MusicMode_t CfgMusicMode = MUSIC_MODE_TRACK_A;
static MenuItem_t MenuFocus  = MENU_LANG;
static PlayMode_t SelectedPlayMode = PLAY_CLASSIC;
static MusicMode_t AppliedMusicMode = MUSIC_MODE_COUNT;
static uint8_t AppliedMusicTrack = 0xFF;

// Layout (128x160 screen)
#define TITLE_BAR_H      12
#define TITLE_PENTRIS_Y  16
#define BOX_X            14
#define BOX_W            100
#define BOX_H            22
#define BOX1_Y           42
#define BOX2_Y           66
#define BOX3_Y           90
#define BLUR_ZONE_Y      118
#define BLUR_ZONE_BOT    152

#define BORDER_BLUE_TITLE 0x351D
#define FOCUS_COLOR       ST7735_YELLOW
#define UNFOCUS_COLOR     0x4208     // dim gray
#define BG_COLOR          ST7735_BLACK
#define TEXT_COLOR        ST7735_WHITE

// ---- Ghost blocks (blur background animation) ----
#define NUM_GHOSTS 4
typedef struct { int16_t x, y; uint8_t type; uint8_t speed; uint8_t counter; } Ghost_t;
static Ghost_t Ghosts[NUM_GHOSTS];

static void GhostInit(int i){
  static const int16_t cols[NUM_GHOSTS] = {16, 48, 80, 112};
  Ghosts[i].x = cols[i];
  Ghosts[i].y = BLUR_ZONE_Y + (int16_t)Random(32);
  Ghosts[i].type = (uint8_t)Random(NUM_PIECES);
  Ghosts[i].speed = 2 + (uint8_t)Random(4);
  Ghosts[i].counter = 0;
}
static void GhostsInitAll(void){ for(int i=0;i<NUM_GHOSTS;i++) GhostInit(i); }

static void GhostsUpdate(void){
  for(int i=0;i<NUM_GHOSTS;i++){
    Ghost_t *g = &Ghosts[i];
    if(++g->counter < g->speed) continue;
    g->counter = 0;
    // always erase old cell — ghosts init inside blur zone so this is always safe
    ST7735_FillRect(g->x, g->y, 8, 8, BG_COLOR);
    g->y += 1;
    if(g->y + 8 > BLUR_ZONE_BOT){
      g->y = BLUR_ZONE_Y;
      g->type = (uint8_t)Random(NUM_PIECES);
      g->speed = 2 + (uint8_t)Random(4);
    }
    ST7735_FillRect(g->x, g->y, 8, 8, DimColor(PieceColors[g->type]));
  }
}

// ---- Header / box drawing ----
static Language_t UiLanguage(Language_t lang){
  return (lang == Spanish) ? Spanish : English;
}
static const char *UiText(Language_t lang, UiText_t id){
  const char * const *table = (UiLanguage(lang) == Spanish) ? UiSpanish : UiEnglish;
  return table[(int)id];
}
static const char *LangName(Language_t selected, Language_t uiLang){
  if(UiLanguage(uiLang) == Spanish){
    return (selected == Spanish) ? "Espanol" : "Ingles";
  }
  return (selected == Spanish) ? "Spanish" : "English";
}
static const char *PlayModeName(PlayMode_t mode, Language_t lang){
  return UiText(lang, (mode == PLAY_TUTORIAL) ? UI_TUTORIAL : UI_CLASSIC);
}
static const char *MusicModeName(MusicMode_t mode, Language_t lang){
  switch(mode){
    case MUSIC_MODE_TRACK_A: return UiText(lang, UI_MUSIC_TRACK_A);
    case MUSIC_MODE_TRACK_B: return UiText(lang, UI_MUSIC_TRACK_B);
    case MUSIC_MODE_TRACK_C: return UiText(lang, UI_MUSIC_TRACK_C);
    case MUSIC_MODE_AUTO:    return UiText(lang, UI_MUSIC_AUTO);
    default:                 return UiText(lang, UI_MUSIC_OFF);
  }
}
static void CycleMusicMode(int8_t dir){
  int8_t next = (int8_t)CfgMusicMode + dir;
  if(next < 0) next = MUSIC_MODE_COUNT - 1;
  if(next >= MUSIC_MODE_COUNT) next = 0;
  CfgMusicMode = (MusicMode_t)next;
}
static uint8_t MusicTrackForMode(MusicMode_t mode, uint32_t score){
  switch(mode){
    case MUSIC_MODE_TRACK_B: return 1;
    case MUSIC_MODE_TRACK_C: return 2;
    case MUSIC_MODE_AUTO:
      if(score >= 6000U) return 2;
      if(score >= 2500U) return 1;
      return 0;
    default:
      return 0;
  }
}
static void ApplyMusicSelection(uint32_t score){
  if(CfgMusicMode == MUSIC_MODE_OFF){
    if(AppliedMusicMode != MUSIC_MODE_OFF){
      Music_Stop();
      AppliedMusicMode = MUSIC_MODE_OFF;
      AppliedMusicTrack = 0xFF;
    }
    return;
  }
  uint8_t targetTrack = MusicTrackForMode(CfgMusicMode, score);
  if(AppliedMusicTrack != targetTrack){
    Music_SelectTrack(targetTrack);
    AppliedMusicTrack = Music_GetTrack();
  }
  Music_Start();
  AppliedMusicMode = CfgMusicMode;
}

// Draw string at exact pixel coordinates (not character-grid row/col).
// Each char is 6px wide × 8px tall at scale 1.
static void TitleDrawStr(int16_t px, int16_t py, const char *s, uint16_t tc, uint16_t bc){
  for(int k = 0; s[k]; k++)
    ST7735_DrawCharS(px + k*6, py, s[k], tc, bc, 1);
}
static int16_t TextLen(const char *s){
  int16_t n = 0;
  while(s[n]) n++;
  return n;
}
static void DrawBigCentered(int16_t py, const char *s, uint16_t tc, uint16_t bc){
  int16_t x = BOARD_X + ((COLS*CELL) - TextLen(s)*12)/2;
  for(int i = 0; s[i]; i++){
    ST7735_DrawCharS(x + i*12, py, s[i], tc, bc, 2);
  }
}

static void DrawHiScoreBar(Language_t lang){
  ST7735_FillRect(0, 0, 128, TITLE_BAR_H, BORDER_BLUE_TITLE);
  // pixel y=2 keeps text inside the 12px bar; x=2 left-aligned
  TitleDrawStr(2,  2, UiText(lang, UI_HI), ST7735_WHITE,  BORDER_BLUE_TITLE);
  char buf[8];
  snprintf(buf, sizeof(buf), "%5lu", (unsigned long)HighScore);
  TitleDrawStr(22, 2, buf,  ST7735_YELLOW, BORDER_BLUE_TITLE);
}

static void DrawPentrisTitle(void){
  ST7735_FillRect(0, TITLE_PENTRIS_Y, 128, 16, BG_COLOR);
  const char *title = "PENTRIS";
  int16_t x = (128 - 7*12) / 2;  // size-2: 12px/char, 7 chars
  for(int i=0;i<7;i++)
    ST7735_DrawCharS(x + i*12, TITLE_PENTRIS_Y, title[i], ST7735_YELLOW, BG_COLOR, 2);
}

static void DrawMenuBox(int idx, int16_t y, const char *label, const char *value){
  uint16_t border = (idx == MenuFocus) ? FOCUS_COLOR : UNFOCUS_COLOR;
  ST7735_DrawFastHLine(BOX_X,          y,            BOX_W, border);
  ST7735_DrawFastHLine(BOX_X,          y + BOX_H-1,  BOX_W, border);
  ST7735_DrawFastVLine(BOX_X,          y,            BOX_H, border);
  ST7735_DrawFastVLine(BOX_X + BOX_W-1, y,           BOX_H, border);
  ST7735_FillRect(BOX_X+1, y+1, BOX_W-2, BOX_H-2, BG_COLOR);
  // y+4 and y+13 are pixel offsets inside the 22px-tall box
  TitleDrawStr(BOX_X + 4, y + 4,  label, TEXT_COLOR, BG_COLOR);
  TitleDrawStr(BOX_X + 4, y + 13, value, (idx == MenuFocus) ? FOCUS_COLOR : TEXT_COLOR, BG_COLOR);
}

static void DrawAllBoxes(Language_t lang){
  DrawMenuBox(0, BOX1_Y, UiText(lang, UI_LANG_LABEL),  LangName(lang, lang));
  DrawMenuBox(1, BOX2_Y, UiText(lang, UI_MUSIC_LABEL), MusicModeName(CfgMusicMode, lang));
  DrawMenuBox(2, BOX3_Y, UiText(lang, UI_PLAY_LABEL),  PlayModeName(SelectedPlayMode, lang));
}

// Title loop. Expects TIMG12 armed + IRQ enabled. Returns chosen language.
// Music_Init() must already have mounted the SD card and loaded a track.
static PlayMode_t TitleScreen(void){
  Language_t lang = UiLanguage(myLanguage);
  MenuFocus = MENU_LANG;

  ST7735_FillScreen(BG_COLOR);
  DrawHiScoreBar(lang);
  DrawPentrisTitle();
  DrawAllBoxes(lang);
  GhostsInitAll();

  Music_SetVolume(4);                   // full volume on title screen
  ApplyMusicSelection(0);

  uint8_t joyCooldown = 0;
  uint8_t hb = 0;
  Semaphore = 0;
  SwEdge    = 0;
  while(1){
    while(!Semaphore) Music_Task(); // feed SD buffer during idle — prevents starvation
    Semaphore = 0;
    Music_Task();

    if(++hb >= 15){ hb = 0; LED_Toggle(1<<17); }   // D3 ~1 Hz heartbeat
    GhostsUpdate();
    // hi-score bar drawn once at init; ghosts stay below y=118 so no redraw needed

    if(joyCooldown > 0) joyCooldown--;
    else {
      bool acted = false;
      if(JoyY > 3000){
        MenuFocus = (MenuFocus == 0) ? (MenuItem_t)(MENU_COUNT-1) : (MenuItem_t)(MenuFocus-1);
        acted = true;
      } else if(JoyY < 1000){
        MenuFocus = (MenuItem_t)((MenuFocus + 1) % MENU_COUNT);
        acted = true;
      } else if(JoyX < 1000 || JoyX > 3000){
        if(MenuFocus == MENU_LANG){
          lang = (lang == English) ? Spanish : English;
          acted = true;
        } else if(MenuFocus == MENU_MUSIC){
          CycleMusicMode((JoyX < 1000) ? 1 : -1);
          ApplyMusicSelection(0);
          acted = true;
        } else if(MenuFocus == MENU_PLAY){
          SelectedPlayMode = (SelectedPlayMode == PLAY_CLASSIC) ? PLAY_TUTORIAL : PLAY_CLASSIC;
          acted = true;
        }
      }
      if(acted){
        joyCooldown = 6;                // ~200 ms at 30 Hz
        Sound_Move();                   // audible click on navigation
        LED_Toggle(1<<16);              // D2 flash on navigation
        DrawHiScoreBar(lang);
        DrawAllBoxes(lang);
      }
    }

    // SW5 toggles the current item or starts the selected play mode.
    if(SwEdge & 0x01){
      SwEdge &= ~0x01;
      if(MenuFocus == MENU_PLAY){
        Sound_Shoot();
        LED_Toggle(1<<16);
        Music_SetVolume(4);                   // full volume for gameplay
        myLanguage = lang;                    // propagate to phrase lookups
        return SelectedPlayMode;
      } else if(MenuFocus == MENU_LANG){
        lang = (lang == English) ? Spanish : English;
        Sound_Move();
        LED_Toggle(1<<16);
        DrawHiScoreBar(lang);
        DrawAllBoxes(lang);
      } else if(MenuFocus == MENU_MUSIC){
        CycleMusicMode(1);
        ApplyMusicSelection(0);
        Sound_Move();
        LED_Toggle(1<<16);
        DrawHiScoreBar(lang);
        DrawAllBoxes(lang);
      }
    }
  }
}

// Forward declarations for helpers defined alongside main5 — referenced by
// the test mains (main3c..main3f) which appear earlier in source order.
static void HardwareInit(void);
static void FeedWait_ms(uint32_t ms);
static void DrawGameScene(void);
static void InitTutorialBoard(void);
static bool TutorialPractice(void);
static bool PauseMenu(void);
static void RunGame(bool tutorialMode);

// use main1 to observe special characters
int main1(void){ // main1
    char l;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_BLACKTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(0x0000);            // set screen to black
  for(int myPhrase=0; myPhrase<= 2; myPhrase++){
    for(int myL=0; myL<= 3; myL++){
         ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
      ST7735_OutChar(' ');
         ST7735_OutString((char *)Phrases[myPhrase][myL]);
      ST7735_OutChar(13);
    }
  }
  Clock_Delay1ms(3000);
  ST7735_FillScreen(0x0000);       // set screen to black
  l = 128;
  while(1){
    Clock_Delay1ms(2000);
    for(int j=0; j < 3; j++){
      for(int i=0;i<16;i++){
        ST7735_SetCursor(7*j+0,i);
        ST7735_OutUDec(l);
        ST7735_OutChar(' ');
        ST7735_OutChar(' ');
        ST7735_SetCursor(7*j+4,i);
        ST7735_OutChar(l);
        l++;
      }
    }
  }
}

// use main2 to observe graphics
int main2(void){ // main2
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_BLACKTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_DrawBitmap(22, 159, PlayerShip0, 18,8); // player ship bottom
  ST7735_DrawBitmap(53, 151, Bunker0, 18,5);
  ST7735_DrawBitmap(42, 159, PlayerShip1, 18,8); // player ship bottom
  ST7735_DrawBitmap(62, 159, PlayerShip2, 18,8); // player ship bottom
  ST7735_DrawBitmap(82, 159, PlayerShip3, 18,8); // player ship bottom
  ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
  ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
  ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
  ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
  ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);

  for(uint32_t t=500;t>0;t=t-5){
    SmallFont_OutVertical(t,104,6); // top left
    Clock_Delay1ms(50);              // delay 50 msec
  }
  ST7735_FillScreen(0x0000);   // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString((char *)"GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString((char *)"Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString((char *)"Earthling!");
  ST7735_SetCursor(2, 4);
  ST7735_OutUDec(1234);
  while(1){
  }
}

// use main3 to test hardware and LEDs
int main3(void){ // main3
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  Joystick_Init();
  LED_Init(); // initialize LED
  Sensor.Init();
  ST7735_InitPrintf(INITR_BLACKTAB);  // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(ST7735_BLACK);
  uint32_t x = 0;
  uint32_t y = 0;
  while(1){

    // //Test 0: Test LEDS
    // LED_On((1<<15));
    // LED_On((1<<16));
    // LED_On((1<<17));
    // Clock_Delay1ms(500);
    // LED_Off((1<<15));
    // LED_Off((1<<16));
    // LED_Off((1<<17));
    // Clock_Delay1ms(500);


    // Test 1: Test Buttons  
    int switchState = Switch_In(); // Reads buttons
    if(switchState & (1<<2)) {  // if button 4 pressed
      LED_On(1<<15);            // turn on LED D1
    } else {
      LED_Off(1<<15);           // turn off LED D1
    }

    if(switchState & (1<<0)) {  // if button 4 pressed
      LED_On(1<<17);            // turn on LED D3
    } else {
      LED_Off(1<<17);           // turn off LED D3
    }



    // // Test 2: Toggle LED on button edge
    // int switchState1 = Switch_In() & 1 ;
    // static uint32_t lastState1 = 0;
    // if((lastState1 == 0) && (switchState1 != 0)) {  // button press detected
    //   LED_Toggle(1<<15);        // toggle LED 2
    // }
    //  lastState1 = switchState1;

    // int switchState2 = Switch_In() & 4 ;
    // static uint32_t lastState2 = 0;
    // if((lastState2 == 0) && (switchState2 != 0)) {  // button press detected
    //   LED_Toggle(1<<15);        // toggle LED 2
    // }
    //  lastState2 = switchState2;


    // // Test 3: Test Joystick
    // x = Joystick_ReadX();
    // y = Joystick_ReadY();
    // uint32_t click = Joystick_ReadClick(); // 0 if pressed, 1 if released
    
    // // Test X-axis: center ~2048, left ~0, right ~4095
    // // Turn on D1 if joystick pushed right (x > 3000)
    // if (x > 3000) {
    //   LED_On(1<<16);   // D1 on = right
    // } else {
    //   LED_Off(1<<16);
    // }

    // // Test Y-axis: center ~2048, down ~0, up ~4095
    // // Turn on D2 if joystick pushed up (y > 3000)
    // if (y > 3000) {
    //   LED_On(1<<15);   // D2 on = up
    // } else {
    //   LED_Off(1<<15);
    // }


    // // Test 4: Test click toggle D3 on each press edge
    // static uint32_t lastClick = 1;  // unpressed = 1 (active LOW)
    // if ((lastClick == 1) && (click == 0)) {  // falling edge = pressed
    //   LED_Toggle(1<<17);  // D3 toggles
    // }
    // lastClick = click;
  

    // // Test 5: Test slidepot
    // uint32_t raw = Sensor.In();
    // ST7735_SetCursor(0, 0);
    // ST7735_OutUDec(raw);
    // ST7735_OutString("   ");
    // Clock_Delay1ms(200);
  }
}

// use main3b to test SPI LCD graphics with actual pentris BMP sprites
// Uses TetrisBG background image + PentrisT/L/O/I/Z sprite BMPs from tetris_images.h
// Board: 120px wide, 146px tall (x=4..123, y=10..155) inside the blue frame
// Borders: left=4px, right=4px, bottom=4px (all halved vs original 8px)
// Score/level bar at top 10px in blue matching the background border
int main3b(void) {
  __disable_irq();
  PLL_Init();
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_BLACKTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo

  // ── palette & layout constants ───────────────────────────────────────────
  const uint16_t BAR_BLUE = 0x351D;  // border color from 8-bit example background
  const uint16_t WHITE    = 0xFFFF;  // board background = piece transparent color
  const int16_t  BX       = 4;       // board left x  (inside 4-px left border)
  const int16_t  BY       = 10;      // board top  y  (below 10-px score bar)
  const int16_t  BW       = 120;     // board width  (4-px borders each side → 15 cells @ 8px)
  const int16_t  BH       = 146;     // board height  → floor at BY+BH = 156 (4-px bottom border)
  const int16_t  STEP     = 2;       // fall speed (pixels per frame)
  const int32_t  FMS      = 50;      // ms per frame (~20 fps)

  // ── helper: draw string at explicit pixel coords with chosen fg/bg color ─
  // ST7735_OutChar always forces ST7735_BLACK background, so we use DrawCharS.
  auto DrawStr = [](int16_t px, int16_t py, const char *s,
                    uint16_t tc, uint16_t bc) {
    for (int k = 0; s[k]; k++)
      ST7735_DrawCharS(px + k * 6, py, s[k], tc, bc, 1);
  };

  // ── helper: draw decimal uint32 at pixel coords ───────────────────────
  auto DrawNum = [](int16_t px, int16_t py, uint32_t n,
                    uint16_t tc, uint16_t bc) {
    char buf[8]; int k = 7; buf[k] = '\0';
    if (n == 0) { buf[--k] = '0'; }
    else { while (n > 0) { buf[--k] = '0' + (char)(n % 10); n /= 10; } }
    for (int j = k; buf[j]; j++)
      ST7735_DrawCharS(px + (j - k) * 6, py, buf[j], tc, bc, 1);
  };

  // ── draw full-screen 8-bit background (blue border + white interior) ───
  // TetrisBG is bottom-row-first (native BMP order). y=159 = bottom edge.
  ST7735_DrawBitmap(0, 159, TetrisBG, 128, 160);

  // ── score/level bar: 10-px blue strip reinforced over background top ───
  ST7735_FillRect(0, 0, 128, 10, BAR_BLUE);
  DrawStr( 0, 1, "SCR:", WHITE, BAR_BLUE);
  DrawStr(62, 1, "LVL:", WHITE, BAR_BLUE);
  DrawStr(98, 1, "1",   WHITE, BAR_BLUE);

  // ── scripted piece fall positions ─────────────────────────────────────
  // Five PENTRIS pieces (5-cell) placed across the 112-px board.
  // Piece dimensions:  PentrisT 24×24, PentrisL 16×32, PentrisO 16×24,
  //                    PentrisI  8×40, PentrisZ 32×16
  // landTop = top y when piece bottom edge is at BY+BH = 152.
  // Transparent areas in sprites are 0xFFFF, matching white board interior.
  struct PScript {
    const uint16_t *img;
    int16_t iw, ih;   // sprite pixel dimensions (w × h)
    int16_t bx;       // absolute left-x on LCD
    int16_t landTop;  // top y when settled (= 152 - ih)
  };
  // Pieces distributed evenly: 96px of sprites + 24px gaps = 120px total
  // Gaps of 6px between each piece: offsets 0, 30, 52, 74, 88
  const PScript script[5] = {
    //             img        w   h    bx          landTop
    {PentrisT,   24, 24,  BX,        BY + BH - 24},  // T5: x=4..27   24-px tall
    {PentrisL,   16, 32,  BX + 30,   BY + BH - 32},  // L5: x=34..49  32-px tall
    {PentrisO,   16, 24,  BX + 52,   BY + BH - 24},  // O5: x=56..71  24-px tall
    {PentrisI,    8, 40,  BX + 74,   BY + BH - 40},  // I5: x=78..85  40-px tall
    {PentrisZ,   32, 16,  BX + 88,   BY + BH - 16},  // Z5: x=92..123 16-px tall
  };

  uint32_t score = 0;

  while (1) {
    // ── reset board to white then redraw background border ───────────────
    ST7735_FillRect(BX, BY, BW, BH, WHITE);
    score = 0;
    // clear score digits area and write "0"
    ST7735_FillRect(24, 0, 38, 10, BAR_BLUE);
    DrawStr(24, 1, "0", WHITE, BAR_BLUE);

    for (int p = 0; p < 5; p++) {
      const PScript &ps = script[p];
      int16_t top     = BY;  // start at board top
      int16_t prevTop = -1;  // no previous frame yet

      // ── fall loop ────────────────────────────────────────────────────
      while (top <= ps.landTop) {
        // 1. Draw sprite at NEW position FIRST — the overlapping region
        //    (all rows below 'prevTop + STEP') already has valid pixels,
        //    so there is no visible black gap between erase and draw.
        ST7735_DrawBitmap(ps.bx, top + ps.ih - 1, ps.img, ps.iw, ps.ih);

        // 2. Erase ONLY the STEP-pixel trailing strip at the old top edge
        //    (the rows now uncovered after the piece moved down by STEP).
        if (prevTop >= 0) {
          ST7735_FillRect(ps.bx, prevTop, ps.iw, STEP, WHITE);
        }
        prevTop = top;
        top    += STEP;
        Clock_Delay1ms(FMS);
      }

      // ── snap to exact landing position ───────────────────────────────
      if (prevTop != ps.landTop) {
        ST7735_DrawBitmap(ps.bx, ps.landTop + ps.ih - 1,
                          ps.img, ps.iw, ps.ih);
        if (prevTop >= 0)
          ST7735_FillRect(ps.bx, prevTop, ps.iw,
                          ps.landTop - prevTop, WHITE);
      }

      // ── lock flash (brief blink to confirm landing) ──────────────────
      Clock_Delay1ms(60);
      ST7735_FillRect(ps.bx, ps.landTop, ps.iw, ps.ih, WHITE);
      Clock_Delay1ms(30);
      ST7735_DrawBitmap(ps.bx, ps.landTop + ps.ih - 1,
                        ps.img, ps.iw, ps.ih);

      // ── update score display ─────────────────────────────────────────
      score += 100;
      ST7735_FillRect(24, 0, 38, 10, BAR_BLUE);
      DrawNum(24, 1, score, WHITE, BAR_BLUE);

      Clock_Delay1ms(300);
    }

    // ── board full — pause then restart ─────────────────────────────────
    Clock_Delay1ms(2000);
  }
}

// ============================================================
// main3c — Title screen test
// Boots the title screen and loops it forever. Pressing SW5 on Start
// flashes LED D2 twice (to prove selection was registered) and returns
// straight back to the menu. Lets you iterate on menu UI without the game.
// LED heartbeat on D3 runs inside TitleScreen (~1 Hz).
int main3c(void){
  HardwareInit();
  TimerG12_IntArm(80000000/30, 1); // priority 1 � below SysTick(0), above everything else
  __enable_irq();          // MUST be before Music_Init — init_spi() waits on TIMG6 ISR
  Music_Init();            // mount SD, open TETRIS8.BIN
  // Music_Init → disk_initialize → SPI1_Init (diskio.cpp) resets LCD (PB15) and
  // clobbers SPI1->CTL0 to 3-wire mode. Re-init the LCD before any drawing.
  ST7735_InitPrintf(INITR_BLACKTAB);
  Music_Start();

  while(1){
    TitleScreen();
    // Visible acknowledgement of SW5 press on Start
    for(int i = 0; i < 4; i++){
      LED_Toggle(1<<16);
      FeedWait_ms(120);
    }
    LED_Off(1<<16);
  }
}

// ============================================================
// main3d — Pentris 5-line clear demo
// Pre-fills bottom 5 rows (leaving col 7 empty), then drops a vertical
// I-piece via sprite animation to complete all 5 rows simultaneously.
// ClearLines() detects the full rows, flashes them, and collapses the board.
int main3d(void) {
  HardwareInit();
  TimerG12_IntArm(80000000/30, 1); // priority 1 - below SysTick(0), above everything else
  __enable_irq();
  ST7735_InitPrintf(INITR_BLACKTAB);

  // I-piece (type 3, rot 0 = vertical, 5 cells tall x 1 cell wide) at col 7
  // With BOARD_X=4, COLS=15, ROWS=18: 4-px left/right borders, 6-px bottom border
  const int16_t DROP_COL  = 7;
  const int16_t DROP_ROW  = ROWS - 5;                    // = 13 (rows 13-17)
  const int16_t BX_piece  = BOARD_X + DROP_COL * CELL;   // = 4 + 56 = 60
  const int16_t landTop   = BOARD_Y + DROP_ROW * CELL;   // = 10 + 104 = 114
  const int16_t IW = 8, IH = 40;
  const uint16_t BAR_BLUE = 0x351D;

  while (1) {
    InitGame();

    // Pre-fill bottom 5 rows with piece colors, leaving col 7 empty.
    // Each row gets a different color so the stack looks like real settled pieces.
    for (int r = DROP_ROW; r < ROWS; r++) {
      uint8_t colorIdx = 1 + ((r - DROP_ROW) % NUM_PIECES);
      for (int c = 0; c < COLS; c++) {
        if (c != DROP_COL) Board[r][c] = colorIdx;
      }
    }

    // Set current piece to I at the drop position
    CurType  = 3;        // I-piece (vertical)
    CurRot   = 0;
    CurRow   = DROP_ROW;
    CurCol   = DROP_COL;
    NextType = 0;

    // Draw background + filled board + score bar — show the setup
    ST7735_DrawBitmap(0, 159, TetrisBG, 128, 160);
    DrawBoardFull();
    // Overpaint top bar to eliminate TetrisBG artifacts (weird line fix)
    ST7735_FillRect(0, 0, 128, BOARD_Y, BAR_BLUE);
    DrawScoreLevel();
    DrawNextPiece();
    Clock_Delay1ms(800);

    // Animate I-piece sprite falling from board top to landing row
    int16_t top     = BOARD_Y;
    int16_t prevTop = -1;
    while (top <= landTop) {
      ST7735_DrawBitmap(BX_piece, top + IH - 1, PentrisI, IW, IH);
      if (prevTop >= 0)
        ST7735_FillRect(BX_piece, prevTop, IW, 2, ST7735_WHITE);
      prevTop = top;
      top    += 2;
      Clock_Delay1ms(40);
    }
    // Snap to exact landing position
    if (prevTop != landTop) {
      ST7735_DrawBitmap(BX_piece, landTop + IH - 1, PentrisI, IW, IH);
      if (prevTop >= 0)
        ST7735_FillRect(BX_piece, prevTop, IW, landTop - prevTop, ST7735_WHITE);
    }
    Clock_Delay1ms(100);

    // Lock piece into Board[][] then clear the 5 completed rows
    LockPiece();
    uint8_t cleared = ClearLines();   // flashes full rows 4x then collapses
    if (cleared) UpdateScore(cleared);

    DrawBoardFull();
    ST7735_FillRect(0, 0, 128, BOARD_Y, BAR_BLUE);
    DrawScoreLevel();

    Clock_Delay1ms(3000);
  }
}
// ============================================================
// main3e — Blur background animation test
// Black screen, just the ghost-block animation running forever.
// LED D3 heartbeat via GhostsUpdate-driven frame loop; LED D2 pulses 1 Hz.
int main3e(void){
  HardwareInit();
  TimerG12_IntArm(80000000/30, 1); // priority 1 � below SysTick(0), above everything else
  __enable_irq();
  Music_Init();
  ST7735_InitPrintf(INITR_BLACKTAB);  // restore LCD after Music_Init clobbers SPI1
  Music_Start();                       // optional — helps verify Music_Task is called

  ST7735_FillScreen(BG_COLOR);
  GhostsInitAll();
  Music_SetVolume(3);

  uint8_t hb = 0;
  Semaphore = 0;
  while(1){
    while(!Semaphore);
    Semaphore = 0;
    Music_Task();
    GhostsUpdate();
    if(++hb >= 30){ hb = 0; LED_Toggle(1<<16); }   // D2 ~1 Hz
  }
}

// ============================================================
// main3f — High-score header render test
// Renders the hi-score bar + PENTRIS title and increments HighScore
// every 2 seconds so you can visually verify formatting/positioning.
int main3f(void){
  HardwareInit();
  TimerG12_IntArm(80000000/30, 1); // priority 1 � below SysTick(0), above everything else
  __enable_irq();

  ST7735_FillScreen(BG_COLOR);
  HighScore = 0;

  while(1){
    DrawHiScoreBar(myLanguage);
    DrawPentrisTitle();
    LED_Toggle(1<<17);                 // D3 heartbeat once per increment
    FeedWait_ms(2000);
    HighScore += 137;                  // arbitrary step to hit varied digits
    if(HighScore > 99999) HighScore = 0;
  }
}


// 32-sample 5-bit sine wave (values 0-31)
const uint8_t SineTest[32] = {
  16,19,22,24,27,28,30,31,31,31,30,
  28,27,24,22,19,16,13,10, 8, 5, 4,
   2, 1, 1, 1, 2, 4, 5, 8,10,13
};

// use main4a to test sound outputs
int main4a(void){ 
  uint32_t last=0,now;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  // LED_Init(); // initialize LED -> dont initialize because PA15
  Sound_Init();  // initialize sound
  // TExaS_Init(ADC0,6,0); // ADC1 channel 6 is PB20, TExaS scope
  __enable_irq();
  // Test A: continuous sine wave (uncomment to test ISR-driven DAC)
  // Sound_Start(SineTest, 32);
  while(1){
    // // Test 0: Test Speaker out (square wave, bypasses ISR)
    // DAC5_Out(31);
    // Clock_Delay1ms(1);
    // DAC5_Out(0);
    // Clock_Delay1ms(1);

    // // Test 1: Sine wave loop via ISR
    // if(Sound_Done()) Sound_Start(SineTest, 32);

    // Test 2: Button-triggered sounds with edge detection
    uint32_t sw = Switch_InEdge();          // rising-edge only, debounced
    if(sw & 0x01) Sound_Start(shoot,         sizeof(shoot));         // BTN1
    if(sw & 0x10) Sound_Start(explosion,     sizeof(explosion));     // UP
    if(sw & 0x08) Sound_Start(invaderkilled, sizeof(invaderkilled)); // LEFT
    if(sw & 0x04) Sound_Start(fastinvader1,  sizeof(fastinvader1)); // DOWN
    // if(sw & 0x02) Sound_Start(highpitch,     sizeof(highpitch));     // RIGHT

      Clock_Delay1ms(20);
  }
}

// // use main4b to Test audio streaming
// // LEDs: D1(PA15)=SD mounted, D2(PA16)=file opened, D3(PA17)=blinks on each SD read
// int main4b(void){
//   __disable_irq();
//   PLL_Init();
//   LaunchPad_Init();
//   Switch_Init();
//   LED_Init();     // D1/D2/D3 used as SD card debug indicators
//   Sound_Init();   // init DAC5 + SysTick at 11025 Hz
//   __enable_irq(); // MUST enable IRQs before Music_Init: init_spi() waits on TIMG6 ISR
//   Music_Init();   // mount SD card, open tetris_theme_loop.bin, preload buffers
//   Music_Start();  // begin streaming immediately
//   uint32_t lastSW = 0;
//   while(1){
//     Music_Task(); // MUST be called every loop — refills SD ping-pong buffer

//     // BTN1 (bit0) toggles music on/off
//     // SW3  (bit2) triggers shoot sound effect (interrupts music briefly)
//     uint32_t sw   = Switch_In() & 0x05; // only BTN1(bit0) + SW3(bit2) soldered
//     uint32_t edge = sw & ~lastSW;
//     lastSW = sw;
//     if(edge & 0x01){                    // BTN1 — toggle music
//       static uint8_t playing = 1;
//       playing = !playing;
//       if(playing) Music_Start();
//       else        Music_Stop();
//     }
//     if(edge & 0x04){                    // SW3/DOWN — shoot sound effect
//       Sound_Shoot();                    // interrupts music briefly, then music resumes
//     }
//     Clock_Delay1ms(20); // must be < 46ms (512 samples @ 11025 Hz) to prevent underrun
//   }
// }

// use main4c to test internal 12-bit DAC (PA15) for sound with buttons and audio streaming.
// PA15 is floating (LED D1 desoldered, PCB trace cut) — safe to use as DAC output.
//
// ====================== REQUIRED CHANGES IN OTHER FILES =======================
//
// inc/Sound.cpp — swap DAC5 for internal DAC throughout:
//
//   1. INCLUDES — replace DAC5 with internal DAC:
//        Remove:  #include "../inc/DAC5.h"
//        Add:     #include "../inc/DAC.h"
//
//   2. Sound_Init() — replace DAC5_Init with DAC_Init:
//        Change:  DAC5_Init();
//        To:      DAC_Init();
//
//   3. SysTick_Handler() — sound-effect output: scale 8-bit (0-255) → 12-bit (0-4080):
//        Change:  DAC5_Out(SoundPt[SoundIndex] >> 3);   // 8-bit → 5-bit
//        To:      DAC_Out((uint32_t)SoundPt[SoundIndex] << 4); // 8-bit → 12-bit
//
//   4. SysTick_Handler() — silence when effect ends: midpoint is 2048 for 12-bit DAC:
//        Change:  if(SoundCount == 0) DAC5_Out(16);
//        To:      if(SoundCount == 0) DAC_Out(2048);
//
//   5. SysTick_Handler() — music streaming output: SD .BIN is 8-bit unsigned PCM,
//        scale the same way as sound effects:
//        Change:  DAC5_Out(front8[Count8]);
//        To:      DAC_Out((uint32_t)front8[Count8] << 4);
//
// Music_Init() in Sound.cpp calls LED_On(1<<15) as a "mounted" indicator.
// Since PA15 is the DAC output pin (not GPIO), that write is harmless — PA15 IOMUX
// is set to the DAC peripheral by DAC_Init(), so the GPIO DOUT write has no effect
// on the pin. Leave the LED_On calls in place or remove them at your discretion.
//
// Do NOT call LED_Init() before Sound_Init() in this main — LED_Init configures
// PA15 IOMUX as GPIO output, which will stomp the DAC IOMUX configuration.
// D2 (PA16) and D3 (PA17) are unaffected and may still be used as indicators if
// LED_Init is modified to skip PA15 (remove PA15 from LED_Init in LED.cpp).
//
// ==============================================================================
//
// Controls (SW4/RIGHT permanently dead — bit1 always masked):
//   BTN1 (bit0) — toggle background music on/off
//   SW3  (bit2) — shoot sound effect
//   SW2  (bit3) — invader-killed sound effect
//   SW1  (bit4) — explosion sound effect
int main4c(void) { // main4c — internal DAC sound test
  __disable_irq();
  PLL_Init();
  LaunchPad_Init();
  Switch_Init();
  LED_Init();     // PA15 skipped in LED_Init — D2(PA16) and D3(PA17) safe to use
  Sound_Init();   // calls DAC_Init() (PA15 IOMUX → DAC), arms SysTick at 11025 Hz
  __enable_irq(); // enable IRQs before Music_Init — SPI init waits on a timer ISR
  // SD card debug: D2(PA16) = SD mounted, D3(PA17) = TETRIS12.bin opened
  // If D2 off after Music_Init → SD card not inserted / FatFS mount failed
  // If D2 on but D3 off → file not found (check filename and SD root)
  // If both on → streaming is active; if still no sound, check .bin sample format
  Music_Init();   // mount SD, open TETRIS12.bin, preload ping-pong buffers
  Music_Start();  // begin streaming immediately on entry

  uint8_t musicOn = 1;

  while(1){
    Music_Task(); // refill SD ping-pong buffer — must be called every main-loop iteration
                  // latency budget: < 46 ms (512 samples @ 11025 Hz); 20 ms poll is safe

    // Rising-edge switch read (Switch_InEdge masks bit1 — SW4 always dead)
    uint32_t sw = Switch_InEdge();

    // Sound effects (interrupt streaming; streaming resumes automatically when done)
    if(sw & 0x04) Sound_Shoot();          // SW3/DOWN  — shoot
    if(sw & 0x08) Sound_Killed();         // SW2/LEFT  — invader killed
    if(sw & 0x10) Sound_Explosion();      // SW1/UP    — explosion

    // BTN1 (bit0) — toggle background music on/off
    if(sw & 0x01){
      musicOn ^= 1;
      if(musicOn) Music_Start();
      else        Music_Stop();
    }

    Clock_Delay1ms(20); // 20 ms debounce/poll window
  }
}

// ============================================================
// Shared hardware init + game runner (used by main5 and main6)
// ============================================================

// One-shot hardware init — must run before TIMG12 is armed.
// Never initialize PA15 as GPIO: it is the internal DAC output.
static void HardwareInit(void){
  __disable_irq();
  PLL_Init();
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_BLACKTAB);
  ST7735_FillScreen(ST7735_BLACK);
  Sensor.Init();
  Switch_Init();
  Joystick_Init();
  JoyClickPrev = (uint8_t)Joystick_ReadClick();
  JoyClickEdge = 0;
  LED_Init();            // PA16/PA17 only — PA15 (DAC) is skipped inside LED_Init
  Sound_Init();
  // Music_Init() moved out — must be called AFTER __enable_irq() since init_spi() waits on TIMG6
}

// Busy-wait that keeps SD audio fed. Same idea as Game.cpp FlashDelay_ms.
static void FeedWait_ms(uint32_t ms){
  uint32_t cycles = ms / 5;
  if(cycles == 0) cycles = 1;
  for(uint32_t i = 0; i < cycles; i++){
    Music_Task();
    Clock_Delay1ms(5);
  }
}

static void DrawGameScene(void){
  ST7735_DrawBitmap(0, 159, TetrisBG, 128, 160);
  ST7735_FillRect(0, 0, 128, BOARD_Y, BORDER_BLUE_TITLE);
  DrawBoardFull();
  DrawScoreLevel();
  DrawNextPiece();
  DrawCurrentPiece(PieceColors[CurType]);
}

static void InitTutorialBoard(void){
  InitGame();
  for(int c = 0; c < COLS; c++){
    Board[ROWS-1][c] = 0;
  }
  for(int c = 0; c < 5; c++){
    Board[ROWS-1][c] = 1 + (c % NUM_PIECES);
  }
  for(int c = 10; c < COLS; c++){
    Board[ROWS-1][c] = 1 + (c % NUM_PIECES);
  }
  CurType      = 3;
  CurRot       = 0;
  CurRow       = 0;
  CurCol       = 7;
  NextType     = 3;
  GravCounter  = 0;
  MoveCounter  = 0;
  Score        = 0;
  Level        = 1;
  Lines        = 0;
  GameOver     = 0;
}

static void WaitForSwitchDismiss(void){
  SwEdge = 0;
  while(1){
    while(!Semaphore) Music_Task();
    Semaphore = 0;
    Music_Task();
    if(SwEdge & 0x1D){
      SwEdge = 0;
      return;
    }
  }
}

static void DrawBoardOverlay(const char *line1, const char *line2){
  ST7735_FillRect(BOARD_X, 52, COLS*CELL, 52, BG_COLOR);
  DrawBigCentered(60, line1, ST7735_WHITE, BG_COLOR);
  DrawBigCentered(84, line2, ST7735_YELLOW, BG_COLOR);
}

static bool TutorialPractice(void){
  const uint8_t STEP_MOVE = 0x01;
  const uint8_t STEP_DROP = 0x02;
  const uint8_t STEP_CW   = 0x04;
  const uint8_t STEP_CCW  = 0x08;
  const uint8_t STEP_ALL  = STEP_MOVE | STEP_DROP | STEP_CW | STEP_CCW;
  uint8_t done = 0;
  int8_t flashRow = -1;
  uint8_t flashTicks = 0;

  auto DrawStepRow = [&](int row, int16_t y, const char *label, const char *hint, uint8_t bit){
    bool complete = (done & bit) != 0;
    bool flashing = (flashRow == row) && (flashTicks > 0) && ((flashTicks & 1) != 0);
    uint16_t border = complete ? 0x07E0 : UNFOCUS_COLOR;
    if(flashing) border = FOCUS_COLOR;
    ST7735_DrawFastHLine(BOX_X, y, BOX_W, border);
    ST7735_DrawFastHLine(BOX_X, y + BOX_H-1, BOX_W, border);
    ST7735_DrawFastVLine(BOX_X, y, BOX_H, border);
    ST7735_DrawFastVLine(BOX_X + BOX_W-1, y, BOX_H, border);
    ST7735_FillRect(BOX_X+1, y+1, BOX_W-2, BOX_H-2, BG_COLOR);
    TitleDrawStr(BOX_X + 4, y + 4, label, complete ? 0x07E0 : TEXT_COLOR, BG_COLOR);
    TitleDrawStr(BOX_X + 4, y + 13, hint, flashing ? FOCUS_COLOR : TEXT_COLOR, BG_COLOR);
    if(complete){
      ST7735_FillRect(BOX_X + BOX_W - 10, y + 7, 6, 6, 0x07E0);
    }
  };

  auto DrawStaticScreen = [&](){
    Language_t lang = UiLanguage(myLanguage);
    ST7735_FillScreen(BG_COLOR);
    ST7735_FillRect(0, 0, 128, TITLE_BAR_H, BORDER_BLUE_TITLE);
    TitleDrawStr(32, 2, UiText(lang, UI_TUTORIAL), ST7735_WHITE, BORDER_BLUE_TITLE);
    TitleDrawStr(10, 20, UiText(lang, UI_LEARN_CONTROLS), TEXT_COLOR, BG_COLOR);
    TitleDrawStr(10, 30, UiText(lang, UI_CLICK_HARD_DROP), TEXT_COLOR, BG_COLOR);
    TitleDrawStr(12, 138, UiText(lang, UI_SW1_PAUSE_MENU), TEXT_COLOR, BG_COLOR);
  };

  auto DrawDynamicScreen = [&](){
    Language_t lang = UiLanguage(myLanguage);
    DrawStepRow(0, 40,  UiText(lang, UI_MOVE),       UiText(lang, UI_STICK_SIDEWAYS), STEP_MOVE);
    DrawStepRow(1, 64,  UiText(lang, UI_DROP),       UiText(lang, UI_STICK_DOWN),     STEP_DROP);
    DrawStepRow(2, 88,  UiText(lang, UI_ROTATE_CW),  UiText(lang, UI_PRESS_SW5),      STEP_CW);
    DrawStepRow(3, 112, UiText(lang, UI_ROTATE_CCW), UiText(lang, UI_PRESS_SW3),      STEP_CCW);
    ST7735_FillRect(0, 148, 128, 12, BG_COLOR);
    if(done == STEP_ALL){
      TitleDrawStr(18, 150, UiText(lang, UI_SW5_START_GAME), ST7735_YELLOW, BG_COLOR);
    } else {
      TitleDrawStr(22, 150, UiText(lang, UI_DO_ALL_4_STEPS), TEXT_COLOR, BG_COLOR);
    }
  };

  DrawStaticScreen();
  DrawDynamicScreen();
  Semaphore = 0;
  SwEdge = 0;

  while(1){
    while(!Semaphore) Music_Task();
    Semaphore = 0;
    Music_Task();

    uint32_t edge = SwEdge;
    bool redraw = false;
    auto CompleteStep = [&](uint8_t bit, int8_t row){
      if((done & bit) == 0){
        done |= bit;
        flashRow = row;
        flashTicks = 8;
        Sound_Move();
        LED_Toggle(1<<16);
        redraw = true;
      }
    };

    if(edge & 0x10){
      bool quit = PauseMenu();
      if(quit) return false;
      DrawStaticScreen();
      DrawDynamicScreen();
      continue;
    }

    if((done == STEP_ALL) && (edge & 0x01)){
      Sound_Shoot();
      return true;
    }
    if(JoyX < 1000 || JoyX > 3000) CompleteStep(STEP_MOVE, 0);
    if(JoyY < 1000)                CompleteStep(STEP_DROP, 1);
    if(edge & 0x01)                CompleteStep(STEP_CW,   2);
    if(edge & 0x04)                CompleteStep(STEP_CCW,  3);

    if(flashTicks > 0){
      flashTicks--;
      redraw = true;
      if(flashTicks == 0) flashRow = -1;
    }
    if(redraw) DrawDynamicScreen();
  }
}

// ============================================================
// PauseMenu — overlays the game board with pause options.
// Returns true if player chose "Main Menu" (RunGame returns to TitleScreen).
// SW1 (bit4) at any time immediately resumes the game (returns false).
// ============================================================
static bool PauseMenu(void){
  const int PM_LANG = 0, PM_MUSIC = 1, PM_MENU = 2;
  int pmFocus = PM_LANG;
  Language_t lang = UiLanguage(myLanguage);

  // Pause box Y positions — sit inside board area (BOARD_Y=10, 14-px header below it)
  const int16_t PB1Y = BOARD_Y + 18;  // LANG box
  const int16_t PB2Y = BOARD_Y + 42;  // MUSIC box
  const int16_t PB3Y = BOARD_Y + 66;  // MAIN MENU box

  // Draw one pause box — captures pmFocus, lang by ref
  auto DrawPBox = [&](int idx, int16_t y, const char *lbl, const char *val){
    uint16_t bc = (pmFocus == idx) ? FOCUS_COLOR : UNFOCUS_COLOR;
    ST7735_DrawFastHLine(BOX_X,           y,          BOX_W, bc);
    ST7735_DrawFastHLine(BOX_X,           y+BOX_H-1,  BOX_W, bc);
    ST7735_DrawFastVLine(BOX_X,           y,          BOX_H, bc);
    ST7735_DrawFastVLine(BOX_X+BOX_W-1,   y,          BOX_H, bc);
    ST7735_FillRect(BOX_X+1, y+1, BOX_W-2, BOX_H-2, BG_COLOR);
    TitleDrawStr(BOX_X+4, y+4,  lbl, TEXT_COLOR, BG_COLOR);
    TitleDrawStr(BOX_X+4, y+13, val, (pmFocus==idx)?FOCUS_COLOR:TEXT_COLOR, BG_COLOR);
  };

  auto DrawPAll = [&](){
    DrawPBox(PM_LANG,  PB1Y, UiText(lang, UI_LANG_LABEL),  LangName(lang, lang));
    DrawPBox(PM_MUSIC, PB2Y, UiText(lang, UI_MUSIC_LABEL), MusicModeName(CfgMusicMode, lang));
    DrawPBox(PM_MENU,  PB3Y, UiText(lang, UI_MAIN_MENU),   UiText(lang, UI_PRESS_SW5));
    TitleDrawStr(BOX_X+4, BOARD_Y+90, UiText(lang, UI_SW1_RESUME), TEXT_COLOR, BG_COLOR);
  };

  // Black out board area, draw pause header bar
  ST7735_FillRect(BOARD_X, BOARD_Y, COLS*CELL, ROWS*CELL, BG_COLOR);
  ST7735_FillRect(BOARD_X, BOARD_Y, COLS*CELL, 14, BORDER_BLUE_TITLE);
  const char *paused = UiText(lang, UI_PAUSED);
  int16_t pausedX = BOARD_X + ((COLS*CELL) - TextLen(paused)*6)/2;
  TitleDrawStr(pausedX, BOARD_Y+3, paused, ST7735_WHITE, BORDER_BLUE_TITLE);
  DrawPAll();

  uint8_t joyCooldown = 0;
  SwEdge = 0;
  while(1){
    while(!Semaphore) Music_Task();
    Semaphore = 0;
    Music_Task();

    // SW1 = immediate resume without selecting anything
    if(SwEdge & 0x10){ SwEdge &= ~0x10; myLanguage = lang; return false; }

    if(joyCooldown > 0){ joyCooldown--; }
    else {
      bool acted = false;
      if(JoyY > 3000){
        pmFocus = (pmFocus == PM_LANG) ? PM_MENU : pmFocus - 1;
        acted = true;
      } else if(JoyY < 1000){
        pmFocus = (pmFocus + 1) % 3;
        acted = true;
      } else if(JoyX < 1000 || JoyX > 3000){
        if(pmFocus == PM_LANG){ lang = (lang==English)?Spanish:English; acted=true; }
        else if(pmFocus == PM_MUSIC){ CycleMusicMode((JoyX < 1000) ? 1 : -1); ApplyMusicSelection(Score); acted=true; }
      }
      if(acted){ joyCooldown = 6; Sound_Move(); DrawPAll(); }
    }

    if(SwEdge & 0x01){
      SwEdge &= ~0x01;
      if(pmFocus == PM_MENU){
        myLanguage = lang;
        return true;   // quit to title screen
      } else if(pmFocus == PM_LANG){
        lang = (lang==English)?Spanish:English; Sound_Move(); DrawPAll();
      } else if(pmFocus == PM_MUSIC){
        CycleMusicMode(1); ApplyMusicSelection(Score); Sound_Move(); DrawPAll();
      }
    }
  }
}

// Runs one game from InitGame() through GameOver. Expects TIMG12 armed + IRQs on.
// Returns when the player dismisses the GAME OVER screen (brief delay + any switch).
static void RunGame(bool tutorialMode){
  if(tutorialMode){
    InitTutorialBoard();
  } else {
    InitGame();
  }
  DrawGameScene();
  if(tutorialMode){
    Language_t lang = UiLanguage(myLanguage);
    DrawBoardOverlay(UiText(lang, UI_ONE_LINE), UiText(lang, UI_LESSON));
    FeedWait_ms(1000);
    DrawGameScene();
  }

  Music_SetVolume(4);    // full volume for gameplay
  ApplyMusicSelection(Score);

  Semaphore = 0;
  SwEdge    = 0;
  JoyClickEdge = 0;
  JoyClickPrev = (uint8_t)Joystick_ReadClick();

  auto HandleLockedPiece = [&]() -> bool {
    LockPiece();
    uint8_t cleared = ClearLines();    // flashes rows then collapses
    if(cleared){
      UpdateScore(cleared);
      Sound_Killed();
    }
    ApplyMusicSelection(Score);
    DrawBoardFull();
    DrawScoreLevel();
    if(tutorialMode){
      Language_t lang = UiLanguage(myLanguage);
      if(cleared){
        DrawBoardOverlay(UiText(lang, UI_LESSON), UiText(lang, UI_DONE));
        FeedWait_ms(1200);
        WaitForSwitchDismiss();
        return true;
      }
      DrawBoardOverlay(UiText(lang, UI_TRY), UiText(lang, UI_AGAIN));
      FeedWait_ms(900);
      InitTutorialBoard();
      JoyClickEdge = 0;
      JoyClickPrev = (uint8_t)Joystick_ReadClick();
      DrawGameScene();
      return false;
    }
    SpawnPiece();
    DrawNextPiece();
    DrawCurrentPiece(PieceColors[CurType]);
    return false;
  };

  while(1){
    while(!Semaphore);
    Semaphore = 0;
    Music_Task();

    if(!tutorialMode && GameOver){
      Language_t lang = UiLanguage(myLanguage);
      DrawBoardOverlay(UiText(lang, UI_GAME), UiText(lang, UI_OVER));
      DrawScoreLevel();
      Sound_Explosion();
      FeedWait_ms(1500);
      WaitForSwitchDismiss();
      if(Score > HighScore) HighScore = Score;
      return;
    }

    // Snapshot all input for this tick before any rendering (avoids race with ISR)
    uint32_t edge = SwEdge;
    bool hardDrop = (JoyClickEdge != 0);
    JoyClickEdge = 0;
    int8_t oldRow = CurRow;
    int8_t oldCol = CurCol;
    uint8_t oldRot = CurRot;

    // ---- Pause (SW1 = bit4) ----
    if(edge & 0x10){
      bool quit = PauseMenu();
      if(quit) return;  // user chose Main Menu — go back to TitleScreen
      // Resume: repair top bar and redraw full game state
      JoyClickEdge = 0;
      JoyClickPrev = (uint8_t)Joystick_ReadClick();
      DrawGameScene();
      ApplyMusicSelection(Score);
      continue;
    }

    // ---- Rotation ----
    uint8_t rotBeforeInput = CurRot;
    if(edge & 0x01) RotatePiece(+1);
    if(edge & 0x04) RotatePiece(-1);
    if(CurRot != rotBeforeInput) Sound_Move();

    // ---- Horizontal move with DAS ----
    int8_t colBeforeInput = CurCol;
    bool movingH = false;
    if(JoyX > 3000){
      if(MoveCounter == 0 || MoveCounter > 10)
        if(IsValid(CurRow, CurCol-1, CurType, CurRot)) CurCol--;
      if(MoveCounter < 255) MoveCounter++;
      movingH = true;
    } else if(JoyX < 1000){
      if(MoveCounter == 0 || MoveCounter > 10)
        if(IsValid(CurRow, CurCol+1, CurType, CurRot)) CurCol++;
      if(MoveCounter < 255) MoveCounter++;
      movingH = true;
    }
    if(!movingH) MoveCounter = 0;
    if(CurCol != colBeforeInput) Sound_Move();

    // ---- Hard drop (joystick click) ----
    if(hardDrop){
      while(IsValid(CurRow+1, CurCol, CurType, CurRot)){
        CurRow++;
      }
      if(HandleLockedPiece()) return;
      continue;
    }

    // ---- Gravity ----
    uint8_t gravPeriod;
    if(tutorialMode){
      gravPeriod = (JoyY < 1000) ? 4 : 22;
    } else if(JoyY < 1000){
      gravPeriod = 2;
    } else {
      // Score-driven difficulty: Level rises with score in Game.cpp.
      int16_t gp = 29 - 2*(int16_t)Level;
      gravPeriod = (gp > 4) ? (uint8_t)gp : 4;
    }

    if(++GravCounter >= gravPeriod){
      GravCounter = 0;
      if(IsValid(CurRow+1, CurCol, CurType, CurRot)){
        CurRow++;
      } else {
        if(HandleLockedPiece()) return;
        continue;
      }
    }

    if(CurRow != oldRow || CurCol != oldCol || CurRot != oldRot){
      DrawCurrentPieceDelta(oldRow, oldCol, oldRot);
    }
  }
}

// ALL ST7735 OUTPUT MUST OCCUR IN MAIN
// main5: restart-on-game-over loop with no title screen (legacy gameplay test).
int main5(void){
  HardwareInit();
  TimerG12_IntArm(80000000/30, 1); // priority 1 � below SysTick(0), above everything else
  __enable_irq();
  Music_Init();
  ST7735_InitPrintf(INITR_BLACKTAB);  // restore LCD after Music_Init clobbers SPI1
  Music_Start();

  while(1){
    RunGame(false);                        // updates HighScore internally
  }
}

// main6 — full product flow: title screen -> game -> title screen -> ...
int main(void){
  HardwareInit();
  TimerG12_IntArm(80000000/30, 1); // priority 1 � below SysTick(0), above everything else
  __enable_irq();
  Music_Init();
  ST7735_InitPrintf(INITR_BLACKTAB);  // restore LCD after Music_Init clobbers SPI1
  Music_Start();

  while(1){
    PlayMode_t mode = TitleScreen();
    if(mode == PLAY_TUTORIAL){
      if(TutorialPractice()){
        RunGame(true);
      }
    } else {
      RunGame(false);
    }
  }
}
