/*
 * led.c - g431 nucleo LED setup
 */

#ifndef __led__
#define __led__

#include "stm32g4xx_hal.h"

void LEDInit(void);
void LEDOn(void);
void LEDOff(void);
void LEDToggle(void);

#endif
