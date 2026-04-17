/* UART2.cpp
 * Your name
 * Data:
 * PA22 UART2 Rx from other microcontroller PA8 IR output<br>
 */


#include <ti/devices/msp/msp.h>
#include "UART2.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/FIFO2.h"

uint32_t LostData;
uint32_t RxCounter;
Queue FIFO2;

// power Domain PD0
// for 80MHz bus clock, UART2 clock is ULPCLK 40MHz
// initialize UART2 for 2375 baud rate
// no transmit, interrupt on receive timeout
void UART2_Init(void){
    // RSTCLR to GPIOA and UART2 peripherals
   // write this
   LostData = 0;
   GPIOA->GPRCM.RSTCTL = 0xB1000003;
   GPIOA->GPRCM.PWREN = 0x26000001;
   Clock_Delay(24);

   UART2->GPRCM.RSTCTL = 0xB1000003;
   UART2->GPRCM.PWREN = 0x26000001;
   Clock_Delay(24);

   IOMUX->SECCFG.PINCM[PA22INDEX] = 0x00040082;

   UART2->CLKSEL = 0x08;
   UART2->CLKDIV = 0x00;

   UART2->CTL0 &= ~0x01;
   UART2->CTL0 = 0x00020018;

   UART2->IBRD = 1052;
   UART2->FBRD = 40;


   UART2->IFLS  = 0x400;
   UART2->CPU_INT.IMASK = 0x40;
   UART2->LCRH = 0x00000070;
   UART2->CTL0 |= 0x01;

   NVIC_SetPriority(UART2_INT_IRQn, 2);
   NVIC_EnableIRQ(UART2_INT_IRQn);
}
//------------UART2_InChar------------
// Get new serial port receive data from FIFO2
// Input: none
// Output: Return 0 if the FIFO2 is empty
//         Return nonzero data from the FIFO1 if available
char UART2_InChar(void){char out;
// write this
while(FIFO2.Get(&out) == false)
{
}
  return out;
}

extern "C" void UART2_IRQHandler(void);
void UART2_IRQHandler(void){ uint32_t status; char letter;
  status = UART2->CPU_INT.IIDX; // reading clears bit in RTOUT
  if(status == 0x07){   // 0x01 receive timeout
    GPIOB->DOUTTGL31_0 = BLUE; // toggle PB22 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = BLUE; // toggle PB22 (minimally intrusive debugging)
    // read all data, putting in FIFO
    // finish writing this
    while((UART2->STAT&0x4) == 0)
    {
      char test = UART2->RXDATA;
      if(FIFO2.Put(test)== false)
      {
        LostData++;
      }
    }
    RxCounter++;
    GPIOB->DOUTTGL31_0 = BLUE; // toggle PB22 (minimally intrusive debugging)
  }
}
