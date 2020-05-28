/*
 * gfx.c - high-level graphics library
 * 03-12-20 E. Brombaugh
 */

#include "gfx.h"
#include "font_8x8.h"

const GFX_COLOR gfx_colortab[16] =
{
	GFX_BLACK,
	GFX_BLUE,
	GFX_GREEN,
	GFX_CYAN,
	GFX_RED,
	GFX_MAGENTA,
	GFX_YELLOW,
	GFX_DGRAY,
	GFX_LGRAY,
	GFX_PINK,
	GFX_LYELLOW,
	GFX_LGREEN,
	GFX_LBLUE,
	GFX_LPURPLE,
	GFX_WHITE,
};

GFX_COLOR forecolor, backcolor;
uint8_t txtsz;
uint16_t gfx_chrbuff[2][64];
uint8_t gfx_chrbuffidx;

/*
 * abs() helper function for line drawing
 */
int16_t gfx_abs(int16_t x)
{
	return (x<0) ? -x : x;
}

/*
 * swap() helper function for line drawing
 */
void gfx_swap(int16_t *z0, int16_t *z1)
{
	int16_t temp = *z0;
	*z0 = *z1;
	*z1 = temp;
}

/*
 * initialize display
 */
void gfx_init(void)
{	
	ST7789_init();
	ST7789_setRotation(2);
	forecolor = ST7789_Color565(GFX_WHITE);
	backcolor = ST7789_Color565(GFX_BLACK);
	txtsz = 1;
	gfx_chrbuffidx = 0;
	gfx_clrscreen();
}

void gfx_set_forecolor(GFX_COLOR color)
{
	forecolor = ST7789_Color565(color);
}

void gfx_set_backcolor(GFX_COLOR color)
{
	backcolor = ST7789_Color565(color);
}

/*
 * fill whole screen with background
 */
void gfx_clrscreen(void)
{
	ST7789_fillScreen(backcolor);
}

/*
 * draw a pixel with foreground
 */
void gfx_setpixel(GFX_POINT pixel)
{
	ST7789_drawPixel(pixel.x, pixel.y, forecolor);
}

/*
 * draw a pixel with background
 */
void gfx_clrpixel(GFX_POINT pixel)
{
	ST7789_drawPixel(pixel.x, pixel.y, backcolor);
}

/*
 * fill a rectangle with color
 */
void gfx_colorrect(GFX_RECT *rect, GFX_COLOR color)
{
	/* check for inversion */
	if(rect->x0 > rect->x1)
		gfx_swap(&rect->x0, &rect->x1);
	if(rect->y0 > rect->y1)
		gfx_swap(&rect->y0, &rect->y1);
	
	ST7789_fillRect(rect->x0, rect->y0, rect->x1-rect->x0+1, rect->y1-rect->y0+1,
		forecolor);
}

/*
 * fill a rectangle with foreground
 */
void gfx_fillrect(GFX_RECT *rect)
{
	gfx_colorrect(rect, forecolor);
}

/*
 * fill a rectangle with background
 */
void gfx_clrrect(GFX_RECT *rect)
{
	gfx_colorrect(rect, backcolor);
}

/*
 * draw a horizontal line
 */
void gfx_drawhline(GFX_POINT start, int16_t w)
{
	ST7789_drawFastHLine(start.x, start.y, w, forecolor);
}

/*
 * draw a vertical line
 */
void gfx_drawvline(GFX_POINT start, int16_t h)
{
	ST7789_drawFastVLine(start.x, start.y, h, forecolor);
}

/*
 * draw a rectangle outline
 */
void gfx_drawrect(GFX_RECT *rect)
{
	/* check for inversion */
	if(rect->x0 > rect->x1)
		gfx_swap(&rect->x0, &rect->x1);
	if(rect->y0 > rect->y1)
		gfx_swap(&rect->y0, &rect->y1);
	
	ST7789_drawFastHLine(rect->x0, rect->y0, rect->x1-rect->x0+1, forecolor);
	ST7789_drawFastHLine(rect->x0, rect->y1, rect->x1-rect->x0+1, forecolor);
	ST7789_drawFastVLine(rect->x0, rect->y0, rect->y1-rect->y0+1, forecolor);
	ST7789_drawFastVLine(rect->x1, rect->y0, rect->y1-rect->y0+1, forecolor);
}

/*
 * draw arbitrary line
 */
void gfx_drawline(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	int16_t steep;
	int16_t deltax, deltay, error, ystep, x, y;
	
	/* flip sense 45deg to keep error calc in range */
	steep = (gfx_abs(y1 - y0) > gfx_abs(x1 - x0));
	
	if(steep)
	{
		gfx_swap(&x0, &y0);
		gfx_swap(&x1, &y1);
	}
	
	/* run low->high */
	if(x0 > x1)
	{
		gfx_swap(&x0, &x1);
		gfx_swap(&y0, &y1);
	}
	
	/* set up loop initial conditions */
	deltax = x1 - x0;
	deltay = gfx_abs(y1 - y0);
	error = deltax/2;
	y = y0;
	if(y0 < y1)
		ystep = 1;
	else
		ystep = -1;
    	
	/* loop x */
	for(x=x0;x<=x1;x++)
	{
		/* plot point */
		if(steep)
			/* flip point & plot */
			ST7789_drawPixel(y, x, forecolor);
		else
			/* just plot */
			ST7789_drawPixel(x, y, forecolor);
		
		/* update error */
		error = error - deltay;
		
		/* update y */
		if(error < 0)
		{
			y = y + ystep;
			error = error + deltax;
		}
	}
}

/*
 * draw an empty circle
 * note - mode 2 doesn't work well due to redrawing some points 
 */
void gfx_drawcircle(int16_t x, int16_t y, int16_t radius)
{
    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    do
    {
        ST7789_drawPixel(x - x_pos, y + y_pos, forecolor);
        ST7789_drawPixel(x + x_pos, y + y_pos, forecolor);
        ST7789_drawPixel(x + x_pos, y - y_pos, forecolor);
        ST7789_drawPixel(x - x_pos, y - y_pos, forecolor);
        e2 = err;
        if(e2 <= y_pos)
        {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos)
            {
                e2 = 0;
            }
        }
        if(e2 > x_pos)
        {
            err += ++x_pos * 2 + 1;
        }
    }
    while(x_pos <= 0);
}

/*
 * draw a filled circle
 * note - mode 2 doesn't work well due to redrawing some lines 
 */
void gfx_fillcircle(int16_t x, int16_t y, int16_t radius)
{
    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    do
    {
        ST7789_drawPixel(x - x_pos, y + y_pos, forecolor);
        ST7789_drawPixel(x + x_pos, y + y_pos, forecolor);
        ST7789_drawPixel(x + x_pos, y - y_pos, forecolor);
        ST7789_drawPixel(x - x_pos, y - y_pos, forecolor);
        ST7789_drawFastHLine(x + x_pos, y + y_pos, 2 * (-x_pos) + 1, forecolor);
        ST7789_drawFastHLine(x + x_pos, y - y_pos, 2 * (-x_pos) + 1, forecolor);
        e2 = err;
        if(e2 <= y_pos)
        {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos)
            {
                e2 = 0;
            }
        }
        if(e2 > x_pos)
        {
            err += ++x_pos * 2 + 1;
        }
    }
    while(x_pos <= 0);
}

/*
 * set size of text
 */
void gfx_set_txtscale(uint8_t scale)
{
	txtsz = scale;
}

/*
 * Draw character direct to the display at 1x scale
 */
void gfx_drawchar_1x(int16_t x, int16_t y, uint8_t chr)
{
    int16_t xt, yt;
	uint16_t i, j;
	uint8_t d;
	uint16_t *gptr = gfx_chrbuff[gfx_chrbuffidx];
	
    yt = y;
    
	/* convert font bitmap to colored glyph */
	for(i=0;i<8;i++)
	{
		d = fontdata[(chr<<3)+i];
        xt = x;
		for(j=0;j<8;j++)
		{
			// set pixel
			*gptr++ = (d&0x80) ? forecolor : backcolor;
			
			// next bit
			d <<= 1;
            
            /* update x counter and clip x */
            xt++;
            if(xt>(_width-1))
                break;
		}
        
        /* update y counter and clip y */
        yt++;
        if(yt>(_height-1))
            break;
	}
    
    /* compute actual dimensions */
    xt -= x;
    yt -= y;
    
    /* render to LCD */
	ST7789_bitblt(x, y, xt, yt, gfx_chrbuff[gfx_chrbuffidx]);
	gfx_chrbuffidx ^= 1;
}

/*
 * Draw character direct to the display at higher scales
 */
void gfx_drawchar_xx(int16_t x, int16_t y, uint8_t chr)
{
    int16_t xt, yt;
	uint16_t i, j;
	uint8_t d;
	
    yt = y;
    
	/* convert font bitmap to colored glyph */
	for(i=0;i<8;i++)
	{
		d = fontdata[(chr<<3)+i];
        xt = x;
		for(j=0;j<8;j++)
		{
			ST7789_fillRect(xt, yt, txtsz, txtsz, (d&0x80) ? forecolor : backcolor);
			
			// next bit
			d <<= 1;
            
            /* update x counter and clip x */
            xt+=txtsz;
            if(xt>(_width-1))
                break;
		}
        
        /* update y counter and clip y */
        yt+=txtsz;
        if(yt>(_height-1))
            break;
	}
}

/*
 * Draw character direct to the display at 1x scale
 */
void gfx_drawchar(int16_t x, int16_t y, uint8_t chr)
{
	if(txtsz == 1)
		gfx_drawchar_1x(x, y, chr);
	else
		gfx_drawchar_xx(x, y, chr);
}

/*
 * draw a string to the display
 */
void gfx_drawstr(int16_t x, int16_t y, char *str)
{
	uint8_t c;
	
	/* loop over string */
	while((c=*str++))
	{
		gfx_drawchar(x, y, c);
		x+=8*txtsz;
	}
}

/*
 * Convert HSV triple to RGB triple
 * use algorithm from
 * http://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
 */
GFX_COLOR gfx_hsv2rgb(uint8_t hsv[])
{
	uint16_t C;
	int16_t Hprime, Cscl;
	uint8_t hs, X, m;
	uint8_t rgb[3] = {0,0,0};
	
	/* calcs are easy if v = 0 */
	if(hsv[2] == 0)
		return 0;
	
	/* C is the chroma component */
	C = ((uint16_t)hsv[1] * (uint16_t)hsv[2])>>8;
	
	/* Hprime is fixed point with range 0-5.99 representing hue sector */
	Hprime = (int16_t)hsv[0] * 6;
	
	/* get intermediate value X */
	Cscl = (Hprime%512)-256;
	Cscl = Cscl < 0 ? -Cscl : Cscl;
	Cscl = 256 - Cscl;
	X = ((uint16_t)C * Cscl)>>8;
	
	/* m is value offset */
	m = hsv[2] - C;
	
	/* get the hue sector (1 of 6) */
	hs = (Hprime)>>8;
	
	/* map by sector */
	switch(hs)
	{
		case 0:
			/* Red -> Yellow sector */
			rgb[0] = C + m;
			rgb[1] = X + m;
			rgb[2] = m;
			break;
		
		case 1:
			/* Yellow -> Green sector */
			rgb[0] = X + m;
			rgb[1] = C + m;
			rgb[2] = m;
			break;
		
		case 2:
			/* Green -> Cyan sector */
			rgb[0] = m;
			rgb[1] = C + m;
			rgb[2] = X + m;
			break;
		
		case 3:
			/* Cyan -> Blue sector */
			rgb[0] = m;
			rgb[1] = X + m;
			rgb[2] = C + m;
			break;
		
		case 4:
			/* Blue -> Magenta sector */
			rgb[0] = X + m;
			rgb[1] = m;
			rgb[2] = C + m;
			break;
		
		case 5:
			/* Magenta -> Red sector */
			rgb[0] = C + m;
			rgb[1] = m;
			rgb[2] = X + m;
			break;
	}
	
	/* pack the color */
	return (rgb[0]<<16) | (rgb[1]<<8) | rgb[2];
}

