// StringConversion.s
// Student names: change this to your names or look very silly
// Last modification date: change this to the last modification date or look very silly
// Runs on any Cortex M0
// ECE319K lab 6 number to string conversion
//
// You write udivby10 and Dec2String
     .data
     .align 2
// no globals allowed for Lab 6
    .global OutChar    // virtual output device
    .global OutDec     // your Lab 6 function
    .global Test_udivby10

    .text
    .align 2
// **test of udivby10**
// since udivby10 is not AAPCS compliant, we must test it in assembly
Test_udivby10:
    PUSH {LR}

    MOVS R0,#123
    BL   udivby10
// put a breakpoint here
// R0 should equal 12 (0x0C)
// R1 should equal 3

    LDR R0,=16389
    BL   udivby10
// put a breakpoint here
// R0 should equal 1234 (0x4D2)
// R1 should equal 5

    MOVS R0,#0
    BL   udivby10
// put a breakpoint here
// R0 should equal 0
// R1 should equal 0
    POP {PC}

// ****************************************************
// divisor=10
// Inputs: R0 is 16-bit dividend
// quotient*10 + remainder = dividend
// Output: R0 is 16-bit quotient=dividend/10
//         R1 is 16-bit remainder=dividend%10 (modulus)
// not AAPCS compliant because it returns two values
udivby10:
   PUSH {LR}
// write this
    LDR  R1, =0xCCCD
    MOVS R2, R1
    MULS R2, R2, R0
    LSRS R2, R2, #19
    MOVS R3, #10
    MULS R3, R3, R2
    SUBS R3, R0, R3
    MOVS R0, R2
    MOVS R1, R3
   POP  {PC}

  
//-----------------------OutDec-----------------------
// Convert a 16-bit number into unsigned decimal format
// Call the function OutChar to output each character
// You will call OutChar 1 to 5 times
// OutChar does not do actual output, OutChar does virtual output used by the grader
// Input: R0 (call by value) 16-bit unsigned number
// Output: none
// Invariables: This function must not permanently modify registers R4 to R11
.equ count, 0 //binding
OutDec:
  PUSH {R4-R7,LR}
   PUSH {R4} //allocation
   MOV R7, SP
   MOVS R4, count
   STR R4, [R7]
// write this
 
MOV R6, R0

CMP R0, #0
BNE LOOP1
MOVS R0, 0x30
BL OutChar
B DONE

LOOP1:
BL udivby10
MOV R4, R0
ADDS R1, 0x30
PUSH{R1}
LDR R5,[R7]
ADDS R5, #1
STR R5, [R7]
CMP R0, #0
BNE LOOP1

PRINT:
CMP R5, #0
BEQ DONE
POP {R0}
BL OutChar
LDR R5, [R7]
SUBS R5, #1
STR R5, [R7]
B PRINT
 
 DONE:
   POP {R4}
   POP  {R4-R7,PC}
// * * * * * * * * End of OutDec * * * * * * * *

// ECE319H recursive version
// Call the function OutChar to output each character
// You will call OutChar 1 to 5 times
// Input: R0 (call by value) 16-bit unsigned number
// Output: none
// Invariables: This function must not permanently modify registers R4 to R11
.equ space, 0//binding
OutDec2:
   PUSH {R5, LR}
   PUSH {R1}//allocation
// write this
CMP R0, #0
BNE NEWCHAR
MOVS R0, 0x30
BL OutChar
B DONE2

NEWCHAR:
BL udivby10
STR R1, [SP, #space]//access
CMP R0, #0
BEQ PRINT2
BL OutDec2

PRINT2:
LDR R0, [SP, #space]//access
ADDS R0, 0x30
BL OutChar

DONE2:
POP{R1}//deallocate
POP  {R5, PC}

     .end
