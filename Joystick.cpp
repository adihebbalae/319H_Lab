/*
 * Joystick.cpp
 *
 *  Created on: Nov 5, 2023
 *      Author:
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
#include "../inc/Clock.h"
// LaunchPad.h defines all the indices into the PINCM table

void Joystick_Init(void) {
    // Configure IOMUX for ADC inputs (PB17, PB19) - analog mode
    IOMUX->SECCFG.PINCM[PB17INDEX] = (uint32_t)0x00000000;
    IOMUX->SECCFG.PINCM[PB19INDEX] = (uint32_t)0x00000000;
    IOMUX->SECCFG.PINCM[PB20INDEX] = (uint32_t)0x00050081;

    ADC1->ULLMEM.GPRCM.RSTCTL = 0xB1000003;
    ADC1->ULLMEM.GPRCM.PWREN  = 0x26000001;
    Clock_Delay(24);
    ADC1->ULLMEM.GPRCM.CLKCFG = 0xA9000000;
    ADC1->ULLMEM.CLKFREQ       = 7;
    ADC1->ULLMEM.CTL0          = 0x03010000;
    ADC1->ULLMEM.CTL1          = 0x00000000;  // single channel, ENDADD=0
    ADC1->ULLMEM.CTL2          = 0x00000000;
    ADC1->ULLMEM.MEMCTL[0]     = 6;   // default to ch6 (PB19)
    ADC1->ULLMEM.SCOMP0        = 0;
    ADC1->ULLMEM.CPU_INT.IMASK = 0;

}

// return current state of joystick
uint32_t Joystick_ReadX(void) {
    ADC1->ULLMEM.CTL0 &= ~0x00000001;  // clear ENC to unlock config
    ADC1->ULLMEM.MEMCTL[0] = 4;        // ch6 = PB19 (left/right)
    ADC1->ULLMEM.CTL0 |= 0x00000001;   // ENC=1
    ADC1->ULLMEM.CTL1 |= 0x00000100;   // start
    uint32_t volatile delay = ADC1->ULLMEM.STATUS;
    while((ADC1->ULLMEM.STATUS & 0x01) == 0x01) {}
    return ADC1->ULLMEM.MEMRES[0];
}

uint32_t Joystick_ReadY(void) {
    ADC1->ULLMEM.CTL0 &= ~0x00000001;  // clear ENC to unlock config
    ADC1->ULLMEM.MEMCTL[0] = 6;        // ch4 = PB17 (up/down)
    ADC1->ULLMEM.CTL0 |= 0x00000001;   // ENC=1
    ADC1->ULLMEM.CTL1 |= 0x00000100;   // start
    uint32_t volatile delay = ADC1->ULLMEM.STATUS;
    while((ADC1->ULLMEM.STATUS & 0x01) == 0x01) {}
    return ADC1->ULLMEM.MEMRES[0];
}

uint32_t Joystick_ReadClick(void) {
    // Read click button - active LOW
    uint32_t pin = GPIOB->DIN31_0;
    return ((pin >> 20) & 0x01);             // 0 if pressed, 1 if released
}
