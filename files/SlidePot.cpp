/* SlidePot.cpp
 * Students put your names here
 * Modified: put the date here
 * 12-bit ADC input on ADC1 channel 5, PB18
 */
#include <ti/devices/msp/msp.h>
#include "../inc/Clock.h"
#include "../inc/SlidePot.h"
#define ADCVREF_VDDA 0x000
#define ADCVREF_INT  0x200


void SlidePot::Init(void){
// write code to initialize ADC1 channel 5, PB18
// Your measurement will be connected to PB18
// 12-bit mode, 0 to 3.3V, right justified
// software trigger, no averaging
    ADC1->ULLMEM.GPRCM.RSTCTL = 0xB1000003;
    ADC1->ULLMEM.GPRCM.PWREN  = 0x26000001;
    Clock_Delay(24);
    ADC1->ULLMEM.GPRCM.CLKCFG = 0xA9000000;
    ADC1->ULLMEM.CLKFREQ       = 7;
    ADC1->ULLMEM.CTL0          = 0x03010000;
    ADC1->ULLMEM.CTL1          = 0x00000000;
    ADC1->ULLMEM.CTL2          = 0x00000000;
    ADC1->ULLMEM.MEMCTL[0]     = 5;           // channel 5 = PB18
    ADC1->ULLMEM.SCOMP0        = 0;
    ADC1->ULLMEM.CPU_INT.IMASK = 0;           // no interrupt
  // write this
}

uint32_t SlidePot::In(void){
  // write code to sample ADC1 channel 5, PB18 once
  // return digital result (0 to 4095)
    ADC1->ULLMEM.CTL0 |= 0x00000001;         // enable conversions
    ADC1->ULLMEM.CTL1 |= 0x00000100;         // start ADC
    uint32_t volatile delay = ADC1->ULLMEM.STATUS; // let ADC start
    while((ADC1->ULLMEM.STATUS & 0x01) == 0x01){}  // wait for completion
    return ADC1->ULLMEM.MEMRES[0];           // 12-bit result
  
}


// constructor, invoked on creation of class
// m and b are linear calibration coefficents
SlidePot::SlidePot(uint32_t m, uint32_t b){
   // write this, runs on creation
   slope = m;
   offset = b;
   data = 0;
   flag = 0;
   distance = 0;
}

void SlidePot::Save(uint32_t n){
    // write this
    data = n;
    flag = 1;
}
uint32_t SlidePot::Convert(uint32_t n){
  n *= slope;
  n = (n>>12);
  n = n+offset;
  return n; // replace this with solution
}
// do not use this function in final lab solution
// it is added just to show you how SLOW floating point in on a Cortex M0+
float SlidePot::FloatConvert(uint32_t input){
  return 0.00048828125*input -0.0001812345;
}

void SlidePot::Sync(void){
  // write this
  while(flag == 0)
  {
  }
  
  flag = 0;

    // wait for semaphore, then clear semaphore
}


uint32_t SlidePot::Distance(void){  // return distance value (0 to 2000), 0.001cm
   distance = Convert(data);
   return distance;
}

