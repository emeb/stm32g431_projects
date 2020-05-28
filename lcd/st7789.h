/*
 * ST7789.h - interface routines for ST7789 LCD.
 * shamelessly ganked from Adafruit_ST7789 library
 * 08-12-19 E. Brombaugh
 */


#ifndef __ST7789__
#define __ST7789__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"

// dimensions
#define ST7789_TFTWIDTH 240
#define ST7789_TFTHEIGHT 240

// Color definitions
#define	ST7789_BLACK   0x0000
#define	ST7789_BLUE    0x001F
#define	ST7789_RED     0xF800
#define	ST7789_GREEN   0x07E0
#define ST7789_CYAN    0x07FF
#define ST7789_MAGENTA 0xF81F
#define ST7789_YELLOW  0xFFE0  
#define ST7789_WHITE   0xFFFF

#define ST7789_HALFCOLOR(x) ((((x))>>1)&(0x7BEF))

extern uint16_t _width, _height;

void ST7789_init(void);
void ST7789_setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7789_fillScreen(uint16_t color);
void ST7789_drawPixel(int16_t x, int16_t y, uint16_t color);
void ST7789_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void ST7789_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void ST7789_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
	uint16_t color);
uint16_t ST7789_Color565(uint32_t rgb24);
void ST7789_bitblt(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buf);
void ST7789_drawchar(int16_t x, int16_t y, uint8_t chr, 
	uint16_t fg, uint16_t bg);
void ST7789_drawstr(int16_t x, int16_t y, char *str,
	uint16_t fg, uint16_t bg);
void ST7789_setRotation(uint8_t m);

#ifdef __cplusplus
}
#endif

#endif
