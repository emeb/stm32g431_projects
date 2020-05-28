/*
 * gfx.h - high-level graphics library
 * 03-12-20 E. Brombaugh
 */

#ifndef __gfx__
#define __gfx__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "st7789.h"

// Color definitions
#define	GFX_BLACK   0x00000000
#define	GFX_BLUE    0x000000FF
#define	GFX_GREEN   0x0000FF00
#define GFX_CYAN    0x0000FFFF
#define	GFX_RED     0x00FF0000
#define GFX_MAGENTA 0x00FF00FF
#define GFX_YELLOW  0x00FFFF00  
#define GFX_DGRAY   0x00505050
#define GFX_LGRAY   0x00A0A0A0
#define GFX_PINK    0x00F8A0F8
#define GFX_LORANGE 0x00F8A000
#define GFX_LYELLOW 0x00F8FCA0
#define GFX_LGREEN  0x0000FCA0
#define GFX_LBLUE   0x0000A0F8
#define GFX_LPURPLE 0x00F850F8
#define GFX_WHITE   0x00FFFFFF

typedef uint32_t GFX_COLOR;

typedef struct
{
	int16_t x;
	int16_t y;
} GFX_POINT;

typedef struct
{
	int16_t x0;
	int16_t y0;
	int16_t x1;
	int16_t y1;
} GFX_RECT;

extern const GFX_COLOR gfx_colortab[];

void gfx_init(void);
void gfx_set_forecolor(GFX_COLOR color);
void gfx_set_backcolor(GFX_COLOR color);
void gfx_clrscreen(void);
void gfx_setpixel(GFX_POINT pixel);
void gfx_clrpixel(GFX_POINT pixel);
void gfx_fillrect(GFX_RECT *rect);
void gfx_clrrect(GFX_RECT *rect);
void gfx_drawhline(GFX_POINT start, int16_t w);
void gfx_drawvline(GFX_POINT start, int16_t h);
void gfx_drawrect(GFX_RECT *rect);
void gfx_drawline(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void gfx_drawcircle(int16_t x, int16_t y, int16_t radius);
void gfx_fillcircle(int16_t x, int16_t y, int16_t radius);
void gfx_set_txtscale(uint8_t scale);
void gfx_drawchar(int16_t x, int16_t y, uint8_t chr);
void gfx_drawstr(int16_t x, int16_t y, char *str);
GFX_COLOR gfx_hsv2rgb(uint8_t hsv[]);

#endif
