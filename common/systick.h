/*
 * systick.h - g431 nucleo system 1ms tick setup
 */

#ifndef __systick__
#define __systick__

#include "main.h"

void Systick_Init(void);
uint8_t Systick_getbuttons(void);

#endif
