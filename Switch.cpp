/*
 * Switch.cpp
 *
 *  Created on: Nov 5, 2023
 *      Author:
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
// LaunchPad.h defines all the indices into the PINCM table

// SW4 (PA25, bit1) is PERMANENTLY DISABLED.
// PA25 is shared with ADC0 ch2 — ANY ADC0 or ADC1 initialization corrupts
// its GPIO mux, causing it to read stuck-HIGH. Masking it here keeps all
// callers clean without requiring them to know about the hardware conflict.
#define SWITCH_MASK 0x1D  // bits 4,3,2,0 = SW1(UP),SW2(LEFT),SW3(DOWN),SW5(BTN1)
                          // bit1 (SW4/RIGHT/PA25) excluded

void Switch_Init(void){
  IOMUX->SECCFG.PINCM[PA28INDEX] = 0x00040081; // SW1 UP
  IOMUX->SECCFG.PINCM[PA27INDEX] = 0x00040081; // SW2 LEFT
  IOMUX->SECCFG.PINCM[PA26INDEX] = 0x00040081; // SW3 DOWN
  // PA25 (SW4 RIGHT) not configured — conflicts with any ADC init
  IOMUX->SECCFG.PINCM[PA24INDEX] = 0x00040081; // SW5 BTN1
}
// return current state of switches (SW4/bit1 always masked to 0)
uint32_t Switch_In(void){
  return ((GPIOA->DIN31_0 >> 24) & SWITCH_MASK);
}

// Returns bits that just transitioned LOW→HIGH since the last call.
// Internally debounces by requiring the same reading twice (~10ms apart).
uint32_t Switch_InEdge(void){
  static uint32_t last = 0;
  static uint32_t candidate = 0;
  static uint32_t candidateCount = 0;
  uint32_t now = Switch_In();
  // simple two-sample debounce: must be stable for 2 consecutive calls
  if(now == candidate){
    candidateCount++;
  } else {
    candidate = now;
    candidateCount = 0;
  }
  uint32_t edge = 0;
  if(candidateCount >= 2){   // stable for ≥2 calls (~10ms at 200ms loop, immediate in tight loop)
    edge = now & ~last;      // bits that are HIGH now but were LOW before
    last = now;
    candidateCount = 0;
  }
  return edge;
}
