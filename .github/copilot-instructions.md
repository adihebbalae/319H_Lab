# ECE319H Lab 9 — Copilot Instructions

## Project Overview
Space Invaders-style game on MSPM0G3507 (Cortex-M0+). 128 KiB ROM, 32 KiB RAM.
Display: ST7735R LCD (128x160). Audio: 5-bit R-2R DAC on PB0-PB4 via ULN2003B driver.
Custom PCB — pin assignments differ from standard LaunchPad documentation.

## PCB Pin Assignments (source of truth: PCB_Specs.md)

### Buttons (active HIGH, 10kΩ pull-DOWN to GND)
- PA28 = SW1 (UP)
- PA27 = SW2 (LEFT)
- PA26 = SW3 (DOWN)
- PA25 = SW4 (RIGHT) — also ADC0 ch2, avoid ADC0 ch2 init or it conflicts
- PA24 = SW5 (BTN1)

### LEDs (via ULN2003B inverting driver — MCU HIGH = LED ON)
- PA15 = D1
- PA16 = D2
- PA17 = D3
- PA8  = D4 (direction indicator)

### Joystick (analog XY + digital click)
- PB17 = VRy (up/down axis) → ADC1 channel 4
- PB19 = VRx (left/right axis) → ADC1 channel 6
- PB20 = click button, 10kΩ pull-UP → active LOW (0 = pressed, 1 = released)

### Slide Potentiometer
- PB18 = wiper → ADC1 channel 5

### DAC (5-bit R-2R ladder)
- PB0 (LSB) through PB4 (MSB)
- DAC5_Out() must be "friendly" — only write bits 4:0, preserve all other Port B pins

### LCD SPI
- PB6=TFT_CS, PB9=SCK, PB8=MOSI, PA13=DC, PB23=RST

### SD Card (shares SPI with LCD)
- PA12=SDC_CS, PB9=SCK, PB8=MOSI, PB7=SDC_MISO

## Driver Implementation Notes

### Switch.cpp
- Switch_Init: configure PA28-PA24 as GPIO inputs (0x00040081 = input, no pull)
- Switch_In: `return ((GPIOA->DIN31_0 >> 24) & 0x1F);`
  - bit4=SW1(UP), bit3=SW2(LEFT), bit2=SW3(DOWN), bit1=SW4(RIGHT), bit0=SW5(BTN1)

### LED.cpp
- LED_Init: configure PA15, PA16, PA17 as GPIO outputs (0x00000081), enable DOE
- LED_On/Off/Toggle: use GPIOA DOUTSET/DOUTCLR/DOUTTGL registers
- Do NOT configure PA26/PA25/PA24 as LED outputs — those are button pins

### DAC5.cpp (inc/DAC5.cpp)
- DAC5_Init: IOMUX PB0-PB4 as 0x81 (output), set DOE bits
- DAC5_Out: `GPIOB->DOUT31_0 = (GPIOB->DOUT31_0 & ~0x1F) | (data & 0x1F);`

### Joystick.cpp
- IOMUX: PB17=0x00 (analog), PB19=0x00 (analog), PB20=0x00050081 (digital pull-up)
- ADC1 init: single-channel mode, CTL1=0x00000000 (ENDADD=0)
- CRITICAL: Must clear ENC (CTL0 bit0) before changing MEMCTL[0] between reads
- Use separate Joystick_ReadX() and Joystick_ReadY() functions:
  ```cpp
  ADC1->ULLMEM.CTL0 &= ~0x00000001;  // clear ENC to unlock MEMCTL
  ADC1->ULLMEM.MEMCTL[0] = <channel>;
  ADC1->ULLMEM.CTL0 |= 0x00000001;   // re-enable
  ADC1->ULLMEM.CTL1 |= 0x00000100;   // start
  while((ADC1->ULLMEM.STATUS & 0x01) == 0x01) {}
  return ADC1->ULLMEM.MEMRES[0];
  ```
- Joystick_ReadClick: `return ((GPIOB->DIN31_0 >> 20) & 0x01);`

### Sound.cpp
- SysTick at 11kHz (period = 80000000/11025 ≈ 7256 for 80MHz clock)
- Global state: `const uint8_t *SoundPt`, `uint32_t SoundCount`, `uint32_t Index`
- SysTick_Handler: output `SoundPt[Index] >> 3` (scale 8-bit→5-bit), increment Index, decrement SoundCount, stop when SoundCount==0
- Sound_Start sets pointer+count; does NOT call DAC directly
- sounds/sounds.h contains: shoot[4080], invaderkilled[3377], explosion[2000], fastinvader1-4, highpitch[1802]

## ADC Conventions
- ADC init pattern (from SlidePot.cpp):
  ```cpp
  ADC1->ULLMEM.GPRCM.RSTCTL = 0xB1000003;
  ADC1->ULLMEM.GPRCM.PWREN  = 0x26000001;
  Clock_Delay(24);
  ADC1->ULLMEM.GPRCM.CLKCFG = 0xA9000000;
  ADC1->ULLMEM.CLKFREQ = 7;  // 40MHz
  ADC1->ULLMEM.CTL0 = 0x03010000;
  ```
- Do NOT init ADC0 channel 2 (PA25) — conflicts with SW4 button
- SlidePot uses ADC1 ch5 (PB18); Joystick uses ADC1 ch4 (PB17) and ch6 (PB19)
- If both SlidePot and Joystick are used together, they share ADC1 — be careful of MEMCTL[0] being overwritten

## IOMUX Values
- `0x00000081` = GPIO output
- `0x00040081` = GPIO input, no pull (bit 18 = input enable)
- `0x00050081` = GPIO input, pull-down (bit 16 set)
- `0x00060081` = GPIO input, pull-up (bit 17 set)
- `0x00050081` = input with pull-up (for active-low like PB20 joystick click)
- `0x00000000` = analog mode (for ADC pins)

## ROM/RAM Budget
- 128 KiB Flash ROM total — sound arrays in sounds.h consume significant space
- Space Invaders sounds already ~15 KB
- Images consume 20-40 KB
- For custom sounds: use WC.m in Octave (`pkg load signal; WC('filename', 5)`) to convert WAV→C array at 11kHz, 5-bit
- For long music: use SD card (PA12=CS) with FatFs driver in SDCFile/ — stream audio to avoid ROM limits

## Game Engine Structure (main5)
- Timer TIMG12 ISR at 30Hz: sample slidepot, read switches, move sprites, start sounds, set semaphore
- NO LCD output in ISR — all ST7735 calls must be in main
- Main loop: wait semaphore → clear semaphore → update LCD → check game state

## Known Hardware Issues
- PA25 reads always HIGH when ADC0 is initialized (even to another channel) due to ADC0 affecting the pin mux — avoid ADC0 init entirely if using PA25 as SW4
- DAC5 dual-channel ADC sequence (CTL1 ENDADD) unreliable — use single-channel reads with ENC toggle instead
