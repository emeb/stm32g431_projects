/*
 * ST7789.c - interface routines for ST7789 LCD.
 * shamelessly ganked from Adafruit_ST7789 library
 * 08-12-19 E. Brombaugh
 */

#include "st7789.h"
#include "shared_spi.h"
#include "printf.h"

/* ----------------------- I/O definitions ----------------------- */
#define ST7789_CS_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define ST7789_CS_GPIO_PORT GPIOB
#define ST7789_CS_PIN GPIO_PIN_12

#define ST7789_DC_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define ST7789_DC_GPIO_PORT GPIOB
#define ST7789_DC_PIN GPIO_PIN_2

#define ST7789_RST_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define ST7789_RST_GPIO_PORT GPIOB
#define ST7789_RST_PIN GPIO_PIN_11

#define ST7789_BL_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define ST7789_BL_GPIO_PORT GPIOB
#define ST7789_BL_PIN GPIO_PIN_1

#define ST7789_CS_LOW()    (ST7789_CS_GPIO_PORT->BSRR=(ST7789_CS_PIN<<16))
#define ST7789_CS_HIGH()   (ST7789_CS_GPIO_PORT->BSRR=ST7789_CS_PIN)
#define ST7789_DC_CMD()    (ST7789_DC_GPIO_PORT->BSRR=(ST7789_DC_PIN<<16))
#define ST7789_DC_DATA()   (ST7789_DC_GPIO_PORT->BSRR=ST7789_DC_PIN)
#define ST7789_RST_LOW()   (ST7789_RST_GPIO_PORT->BSRR=(ST7789_RST_PIN<<16))
#define ST7789_RST_HIGH()  (ST7789_RST_GPIO_PORT->BSRR=ST7789_RST_PIN)
#define ST7789_BL_LOW()    (ST7789_BL_GPIO_PORT->BSRR=(ST7789_BL_PIN<<16))
#define ST7789_BL_HIGH()   (ST7789_BL_GPIO_PORT->BSRR=ST7789_BL_PIN)

#define ST_CMD            0x100
#define ST_CMD_DELAY      0x200
#define ST_CMD_END        0x400

#define ST77XX_NOP        0x00
#define ST77XX_SWRESET    0x01
#define ST77XX_RDDID      0x04
#define ST77XX_RDDST      0x09

#define ST77XX_SLPIN      0x10
#define ST77XX_SLPOUT     0x11
#define ST77XX_PTLON      0x12
#define ST77XX_NORON      0x13

#define ST77XX_INVOFF     0x20
#define ST77XX_INVON      0x21
#define ST77XX_DISPOFF    0x28
#define ST77XX_DISPON     0x29
#define ST77XX_CASET      0x2A
#define ST77XX_RASET      0x2B
#define ST77XX_RAMWR      0x2C
#define ST77XX_RAMRD      0x2E

#define ST77XX_PTLAR      0x30
#define ST77XX_COLMOD     0x3A
#define ST77XX_MADCTL     0x36

#define ST77XX_MADCTL_MY  0x80
#define ST77XX_MADCTL_MX  0x40
#define ST77XX_MADCTL_MV  0x20
#define ST77XX_MADCTL_ML  0x10
#define ST77XX_MADCTL_RGB 0x08
#define ST77XX_MADCTL_MH  0x04


#define ST77XX_RDID1      0xDA
#define ST77XX_RDID2      0xDB
#define ST77XX_RDID3      0xDC
#define ST77XX_RDID4      0xDD

/* uncomment this to enable DMA */
#define USE_DMA

/* ----------------------- private variables ----------------------- */
// Initialization command sequence
const static uint16_t
  initlst[] = {
    ST77XX_SWRESET | ST_CMD,        //  1: Software reset, no args, w/delay
    ST_CMD_DELAY | 150,             //  150 ms delay
    ST77XX_SLPOUT | ST_CMD ,        //  2: Out of sleep mode, no args, w/delay
	ST_CMD_DELAY | 500,             //  500 ms delay
    ST77XX_COLMOD | ST_CMD ,        //  3: Set color mode
      0x55,                         //     16-bit color
	ST_CMD_DELAY | 10,              //     10 ms delay
    ST77XX_MADCTL | ST_CMD ,        //  4: Mem access ctrl (directions), 1 arg:
      0x00,                         //     Row/col addr, bottom-top refresh
    ST77XX_CASET | ST_CMD  ,        //  5: Column addr set, 4 args, no delay:
      0x00,
      0,                            //     XSTART = 0
      0,
      240,                          //     XEND = 240
    ST77XX_RASET | ST_CMD  ,        //  6: Row addr set, 4 args, no delay:
      0x00,
      0,                            //     YSTART = 0
      320>>8,
      320&0xff,                     //     YEND = 320
    ST77XX_INVON | ST_CMD  ,        //  7: hack
    ST_CMD_DELAY | 10,              //  10 ms
    ST77XX_NORON | ST_CMD  ,        //  8: Normal display on, no args, w/delay
    ST_CMD_DELAY | 10,              //  10 ms delay
    ST77XX_DISPON | ST_CMD ,        //  9: Main screen turn on, no args, delay
    ST_CMD_DELAY | 500,             //  500 ms delay
	ST_CMD_END                      //  END OF LIST
};

/* SPI port handle */
SPI_HandleTypeDef SpiHandle;

#ifdef USE_DMA
/* uncomment this to use non-blocking DMA */
#define NONBLOCKING_DMA

#ifdef NONBLOCKING_DMA
/* Lock for nonblocking DMA */
volatile uint8_t st7789_dma_busy;
#endif
#endif

/* LCD state */
uint8_t rowstart, colstart;
uint16_t _width, _height, rotation;

/* ----------------------- Private functions ----------------------- */
/*
 * Initialize SPI interface to LCD
 */
void ST7789_SPI_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable CS pin Clock */
	ST7789_CS_GPIO_CLK_ENABLE();
	
	/* Enable CS pin for output */
	GPIO_InitStructure.Pin =  ST7789_CS_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL ;
	HAL_GPIO_Init(ST7789_CS_GPIO_PORT, &GPIO_InitStructure);
	
	/* Enable DC pin Clock */
	ST7789_DC_GPIO_CLK_ENABLE();
	
	/* Enable DC pin for output */
	GPIO_InitStructure.Pin =  ST7789_DC_PIN;
	HAL_GPIO_Init(ST7789_DC_GPIO_PORT, &GPIO_InitStructure);
	
	/* Enable RST pin Clock */
	ST7789_RST_GPIO_CLK_ENABLE();
	
	/* Enable RST pin for output */
	ST7789_RST_HIGH();
	GPIO_InitStructure.Pin =  ST7789_RST_PIN;
	HAL_GPIO_Init(ST7789_RST_GPIO_PORT, &GPIO_InitStructure);
	
	/* Enable BL pin Clock */
	ST7789_BL_GPIO_CLK_ENABLE();
	
	/* Enable BL pin for output */
	ST7789_BL_HIGH();
	GPIO_InitStructure.Pin =  ST7789_BL_PIN;
	HAL_GPIO_Init(ST7789_BL_GPIO_PORT, &GPIO_InitStructure);
	
#ifdef USE_DMA
#ifdef NONBLOCKING_DMA
	/* Lock for nonblocking DMA */
	st7789_dma_busy = 0;
	
	/* enable IRQ for end of SPI DMA */
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
#endif
#endif

	/* init external SPI */
	Shared_SPI_Init();
}

/*
 * send single byte via SPI - cmd or data depends on bit 8
 */
void ST7789_write_byte(uint16_t dat)
{
	if((dat & ST_CMD) == ST_CMD)
		ST7789_DC_CMD();
	else
		ST7789_DC_DATA();

	ST7789_CS_LOW();

    Shared_SPI_WriteByte(dat&0xff);
    
	ST7789_CS_HIGH();
}

/* ----------------------- Public functions ----------------------- */
// Initialization for ST7789R red tab screens
void ST7789_init(void)
{
	// init the SPI port
	ST7789_SPI_Init();
	
	// default settings
	colstart = 0;
	rowstart = 0;
	_width  = ST7789_TFTWIDTH;
	_height = ST7789_TFTHEIGHT;
	rotation = 0;
	
	// Reset it
	HAL_Delay(5);
	ST7789_RST_LOW();
	HAL_Delay(10);
	ST7789_RST_HIGH();
	HAL_Delay(5);

	// Send init command list
	uint16_t *addr = (uint16_t *)initlst, ms;
	while(*addr != ST_CMD_END)
	{
		if((*addr & ST_CMD_DELAY) != ST_CMD_DELAY)
			ST7789_write_byte(*addr++);
		else
		{
			ms = (*addr++)&0x1ff;        // strip delay time (ms)
			HAL_Delay(ms);
		}	
	}
}

// opens a window into display mem for bitblt
void ST7789_setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
	uint8_t tx_buf[4];
	uint16_t sum;
	
	ST7789_write_byte(ST77XX_CASET | ST_CMD); // Column addr set
	sum = x0+colstart;
	tx_buf[0] = sum>>8;
	tx_buf[1] = sum&0xff;
	sum = x1+colstart;
	tx_buf[2] = sum>>8;
	tx_buf[3] = sum&0xff;
	ST7789_DC_DATA();
	ST7789_CS_LOW();
	Shared_SPI_Blocking_PIO_WriteBytes(tx_buf, 4);
	ST7789_CS_HIGH();
	
	ST7789_write_byte(ST77XX_RASET | ST_CMD); // Row addr set
	sum = y0+rowstart;
	tx_buf[0] = sum>>8;
	tx_buf[1] = sum&0xff;
	sum = y1+rowstart;
	tx_buf[2] = sum>>8;
	tx_buf[3] = sum&0xff;
	ST7789_DC_DATA();
	ST7789_CS_LOW();
	Shared_SPI_Blocking_PIO_WriteBytes(tx_buf, 4);
	ST7789_CS_HIGH();

	ST7789_write_byte(ST77XX_RAMWR | ST_CMD); // write to RAM
}

// fill screen w/ single color
void ST7789_fillScreen(uint16_t color)
{
	ST7789_fillRect(0, 0, ST7789_TFTWIDTH, ST7789_TFTHEIGHT, color);
}

// draw single pixel
void ST7789_drawPixel(int16_t x, int16_t y, uint16_t color)
{

	if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;

	ST7789_setAddrWindow(x,y,x+1,y+1);

	ST7789_DC_DATA();
	ST7789_CS_LOW();

	Shared_SPI_WriteWord(color);
	
	ST7789_CS_HIGH();
}

// fast vert line
void ST7789_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	// Rudimentary clipping
	if((x >= _width) || (y >= _height)) return;
	if((y+h-1) >= _height) h = _height-y;
	ST7789_setAddrWindow(x, y, x, y+h-1);

	ST7789_DC_DATA();
	ST7789_CS_LOW();
	
	Shared_SPI_Blocking_PIO_WriteWord(color, h);
	
	ST7789_CS_HIGH();
}

// fast horiz line
void ST7789_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	// Rudimentary clipping
	if((x >= _width) || (y >= _height)) return;
	if((x+w-1) >= _width)  w = _width-x;
	ST7789_setAddrWindow(x, y, x+w-1, y);

	ST7789_DC_DATA();
	ST7789_CS_LOW();
	
	Shared_SPI_Blocking_PIO_WriteWord(color, w);
	
	ST7789_CS_HIGH();
}

// fill a rectangle
void ST7789_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
	uint16_t color)
{
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	if((x + w - 1) >= _width)  w = _width  - x;
	if((y + h - 1) >= _height) h = _height - y;
	
	ST7789_setAddrWindow(x, y, x+w-1, y+h-1);

	/* prep tos end data */
	ST7789_DC_DATA();
	ST7789_CS_LOW();
	
	/* faster version keeps pipes full */ 
	Shared_SPI_Blocking_PIO_WriteWord(color, h*w);

	ST7789_CS_HIGH();
}

// Pass 8-bit (each) R,G,B in 32-bit get back 16-bit packed color
uint16_t ST7789_Color565(uint32_t rgb24)
{
	return __REVSH((((rgb24>>16) & 0xF8) << 8) | (((rgb24>>8) & 0xFC) << 3) | ((rgb24 & 0xF8) >> 3));
}

// bitblt a region to the display
void ST7789_bitblt(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buf)
{
#ifdef USE_DMA
#ifdef NONBLOCKING_DMA
	/* nonblocking DMA waits for DMA to finish then proceeds & returns */
	while(st7789_dma_busy);
		
	ST7789_setAddrWindow(x, y, x+w-1, y+h-1);
	
	ST7789_DC_DATA();
	ST7789_CS_LOW();
    
    /* start DMA */
	st7789_dma_busy = 1;
    Shared_SPI_start_DMA_WriteBytes((uint8_t *)buf, 2*w*h);
#else
	/* blocking DMA just does start and end sequentiall */
	ST7789_setAddrWindow(x, y, x+w-1, y+h-1);
	
	ST7789_DC_DATA();
	ST7789_CS_LOW();
    
    /* start DMA */
    Shared_SPI_start_DMA_WriteBytes((uint8_t *)buf, 2*w*h);
    
    /* end DMA */
    Shared_SPI_end_DMA_WriteBytes();
	
	ST7789_CS_HIGH();
#endif
#else
	/* PIO */
	ST7789_setAddrWindow(x, y, x+w-1, y+h-1);
	
	ST7789_DC_DATA();
	ST7789_CS_LOW();
    
	/* PIO buffer send */
	Shared_SPI_Blocking_PIO_WriteBytes((uint8_t *)buf, 2*w*h);
	
	ST7789_CS_HIGH();
#endif
}

// set orientation of display
void ST7789_setRotation(uint8_t m)
{
	ST7789_write_byte(ST77XX_MADCTL | ST_CMD);
	rotation = m % 4; // can't be higher than 3
	switch (rotation)
	{
		case 0:
			ST7789_write_byte(ST77XX_MADCTL_MX | ST77XX_MADCTL_MY);
			_width  = ST7789_TFTWIDTH;
			_height = ST7789_TFTHEIGHT;
			rowstart = 80;
			colstart = 0;
			break;
		
		case 1:
			ST7789_write_byte(ST77XX_MADCTL_MY | ST77XX_MADCTL_MV);
			_width  = ST7789_TFTHEIGHT;
			_height = ST7789_TFTWIDTH;
			rowstart = 0;
			colstart = 80;
			break;
		
		case 2:
			ST7789_write_byte(0);
			_width  = ST7789_TFTWIDTH;
			_height = ST7789_TFTHEIGHT;
			rowstart = 0;
			colstart = 0;
			break;
		
		case 3:
			ST7789_write_byte(ST77XX_MADCTL_MX | ST77XX_MADCTL_MV);
			_width  = ST7789_TFTHEIGHT;
			_height = ST7789_TFTWIDTH;
			rowstart = 0;
			colstart = 0;
			break;
	}
}

#ifdef USE_DMA
#ifdef NONBLOCKING_DMA
/*
 * DMA IRQ handler for nonblocking DMA
 */
void DMA1_Channel1_IRQHandler(void)
{
	/* check for TC on our channel */
	if(__HAL_DMA_GET_FLAG(&hdma_spi, SPI_DMA_TCFLAG)==SPI_DMA_TCFLAG)
	{
		/* finish up DMA transfer */
		Shared_SPI_end_DMA_WriteBytes();
		
		/* allow next transfer */
		st7789_dma_busy = 0;
		ST7789_CS_HIGH();
	}
}
#endif
#endif


