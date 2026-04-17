/*
 * Switch.h
 *
 *  Created on: Nov 5, 2023
 *      Author: jonat
 */

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

// initialize Joystick
void Joystick_Init(void);

// return current state of joystick
uint32_t Joystick_ReadX(void);
uint32_t Joystick_ReadY(void);
uint32_t Joystick_ReadClick(void);


#endif /* JOYSTICK_H_ */
