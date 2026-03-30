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
  ADC1->ULLMEM.GPRCM.RSTCTL = 0xB1000003;   //reset
  ADC1->ULLMEM.GPRCM. PWREN = 0x26000001;   //active
  Clock Delay (24) ;                        //wait
  ADC1->ULLMEM.GPRCM.CLKCFG = 0xA9000000;
  ADC1->ULLMEM.CLKFREQ = 7;
  ADC1->ULLMEM.CTLO = 0x03010000;
  ADC1->ULLMEM.CTL1 = 0x00000000;
  ADC1->ULLMEM.CTL2 = 0x00000000;
  ADC1->ULLMEM.MEMCTL[0] = 5;
  ADC1->ULLMEM.SCOMPO = 0;
  // write this
}
uint32_t SlidePot::In(void){
  // write code to sample ADC1 channel 5, PB18 once
  // return digital result (0 to 4095)
  ADC1->ULLMEM.CTLO |= 0x00000001;
  ADC1->ULLMEM.CTL1 |= 0x00000100;
  uint32 t volatile delay=ADC1->ULLMEM.STATUS;
  while((ADC1->ULLMEM.STATUS&0x01) == 0x01) {}
  return ADC1->ULLMEM.MEMRES [0];
}


// constructor, invoked on creation of class
// m and b are linear calibration coefficents
SlidePot::SlidePot(uint32_t m, uint32_t b){
   // write this, runs on creation
}

void SlidePot::Save(uint32_t n){
    // write this
}
uint32_t SlidePot::Convert(uint32_t n){
  return 42; // replace this with solution
}
// do not use this function in final lab solution
// it is added just to show you how SLOW floating point in on a Cortex M0+
float SlidePot::FloatConvert(uint32_t input){
  return 0.00048828125*input -0.0001812345;
}

void SlidePot::Sync(void){
  // write this

    // wait for semaphore, then clear semaphore
}


uint32_t SlidePot::Distance(void){  // return distance value (0 to 2000), 0.001cm
   return 42; // replace this with solution
}

