/*
 * systick.c - g431 nucleo system 1ms tick setup
 */

#include "systick.h"

/*
 * Initialize the breakout board LED
 */
void Systick_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIO A Clock */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	
	/* Enable buttons for output */
	GPIO_InitStructure.Pin =  GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStructure.Pull = GPIO_PULLUP ;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*
 * get inputs
 */
uint8_t Systick_getbuttons(void)
{
	return (~(GPIOA->IDR >> 6))&0xf;
}

/*
 * needed by HAL
 */
void SysTick_Handler(void)
{
  HAL_IncTick();
}


