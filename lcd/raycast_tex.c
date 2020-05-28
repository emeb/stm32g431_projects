/*
 * raycast_tex.c - test textured raycasting on STM32G4
 * 04-12-20 E. Brombaugh
 * based on code from https://lodev.org/cgtutor/raycasting.html
 */

#include <math.h>
#include "raycast.h"
#include "gfx.h"
#include "printf.h"
#include "systick.h"
#include "textures.h"

#define screenWidth ST7789_TFTWIDTH
#define screenHeight (ST7789_TFTHEIGHT-8)
#define texWidth 64
#define texHeight 64
#define mapWidth 24
#define mapHeight 24

float32_t posX, posY;  //x and y start position
float32_t dirX, dirY; //initial direction vector
float32_t planeX, planeY; //the 2d raycaster version of camera plane

uint32_t time ; //time of current frame
uint32_t oldTime ; //time of previous frame

uint16_t ramtex[texWidth*texHeight];  // dynamic texture in RAM
uint8_t rt_offset;
uint16_t linebuf[2][ST7789_TFTHEIGHT];	// DMA buffers for vertical lines
uint8_t bufnum;
uint8_t keys;

/*
 * the world map
 */
const uint8_t worldMap[mapWidth][mapHeight]=
{
  {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,7,7,7,7,7,7,7},
  {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7},
  {4,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7},
  {4,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7},
  {4,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7},
  {4,0,4,0,0,0,0,5,5,5,5,5,5,5,5,5,7,7,0,7,7,7,7,7},
  {4,0,5,0,0,0,0,5,0,5,0,5,0,5,0,5,7,0,0,0,7,7,7,1},
  {4,0,6,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8},
  {4,0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,7,7,1},
  {4,0,8,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8},
  {4,0,0,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,7,7,7,1},
  {4,0,0,0,0,0,0,5,5,5,5,0,5,5,5,5,7,7,7,7,7,7,7,1},
  {6,6,6,6,6,6,6,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6},
  {8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
  {6,6,6,6,6,6,0,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6},
  {4,4,4,4,4,4,0,4,4,4,6,0,6,2,2,2,2,2,2,2,3,3,3,3},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,0,0,0,6,2,0,0,5,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2},
  {4,0,6,0,6,0,0,0,0,4,6,0,0,0,0,0,5,0,0,0,0,0,0,2},
  {4,0,0,5,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2},
  {4,0,6,0,6,0,0,0,0,4,6,0,6,2,0,0,5,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2},
  {4,4,4,4,4,4,4,4,4,4,1,1,1,2,2,2,2,2,2,3,3,3,3,3}
};

/*
 * colormap for flay raycast
 */
const uint16_t colormap[] =
{
	0, 0,
	ST7789_RED, ST7789_HALFCOLOR(ST7789_RED),
	ST7789_GREEN, ST7789_HALFCOLOR(ST7789_GREEN),
	ST7789_BLUE, ST7789_HALFCOLOR(ST7789_BLUE),
	ST7789_WHITE, ST7789_HALFCOLOR(ST7789_WHITE),
	ST7789_YELLOW, ST7789_HALFCOLOR(ST7789_YELLOW),
};

enum key_map
{
	SDLK_LEFT,
	SDLK_UP,
	SDLK_RIGHT,
	SDLK_DOWN,
};

/*
 * key input helper
 */
void readKeys(void)
{
	keys = Systick_getbuttons();
}

/*
 * key check helper
 */
uint8_t keyDown(uint8_t key)
{
	return (keys & (1<<key));
}

/*
 * set up raycasting
 */
void raycast_init(void)
{
	int32_t x, y;
	
	posX = 22.0F;
	posY = 11.5F;
	dirX = -1.0F;
	dirY = 0.0F;
	planeX = 0.0F;
	planeY = 0.66F;
	time = HAL_GetTick();
	keys = 0;
	bufnum = 0;
	rt_offset = 0;
	
	/* create textures */
	for(x = 0; x < texWidth; x++)
	{
		int32_t xorcolor, ycolor, xycolor;
		
		for(y = 0; y < texHeight; y++)
		{
			xorcolor = (x * 256 / texWidth) ^ (y * 256 / texHeight);
			ycolor = y * 256 / texHeight;
			xycolor = y * 128 / texHeight + x * 128 / texWidth;
			
			switch(4)
			{
				case 0:
					//flat red texture with black cross
					ramtex[texWidth * y + x] =
						ST7789_Color565(65536 * 254 * (x != y && x != texWidth - y));
					break;
				
				case 1:
					//sloped greyscale
					ramtex[texWidth * y + x] =
						ST7789_Color565(xycolor + 256 * xycolor + 65536 * xycolor);
					break;
				
				case 2:
					//sloped yellow gradient
					ramtex[texWidth * y + x] = 
						ST7789_Color565(256 * xycolor + 65536 * xycolor);
					break;
				
				case 3:
					//xor greyscale
					ramtex[texWidth * y + x] =
						ST7789_Color565(xorcolor + 256 * xorcolor + 65536 * xorcolor);
					break;
				
				case 4:
					//xor green
					ramtex[texWidth * y + x] =
						ST7789_Color565(256 * xorcolor);
					break;
				
				case 5:
					//red bricks
					ramtex[texWidth * y + x] =
						ST7789_Color565(65536 * 192 * (x % 16 && y % 16));
					break;
				
				case 6:
					//red gradient
					ramtex[texWidth * y + x] =
						ST7789_Color565(65536 * ycolor);
					break;
				
				case 7:
					//flat grey texture
					ramtex[texWidth * y + x] =
						ST7789_Color565(128 + 256 * 128 + 65536 * 128);
					break;
			}
		}
	}
}

/*
 * one frame of raycast
 */
void raycast_calc(void)
{
	int16_t x, w, y, h, mapX, mapY, stepX, stepY, hit, side, lineHeight;
	int16_t drawStart, drawEnd;
	float32_t cameraX, rayDirX, rayDirY;
	float32_t sideDistX, sideDistY; //length of ray from one x or y-side to next x or y-side
	float32_t deltaDistX, deltaDistY, perpWallDist;
	uint16_t color;
	int8_t texNum, texX, texY;
	float32_t wallX, step, texPos;
	uint32_t fps;
	char textbuf[30];
	float32_t frameTime, moveSpeed, rotSpeed, oldDirX, oldPlaneX;
		
	w = screenWidth;
	h = screenHeight;
	side = 0;
	
    for(x=0;x<w;x++)
    {
		//calculate ray position and direction
		cameraX = 2.0F *(float32_t)x / (float32_t)w - 1.0F; //x-coordinate in camera space
		rayDirX = dirX + planeX * cameraX;
		rayDirY = dirY + planeY * cameraX;
		
		//which box of the map we're in
		mapX = (int16_t)floorf(posX);
		mapY = (int16_t)floorf(posY);

		//length of ray from one x or y-side to next x or y-side
		deltaDistX = fabsf(1.0F / rayDirX);
		deltaDistY = fabsf(1.0F / rayDirY);

		//calculate step and initial sideDist
		if(rayDirX < 0)
		{
			stepX = -1;
			sideDistX = (posX - mapX) * deltaDistX;
		}
		else
		{
			stepX = 1;
			sideDistX = (mapX + 1.0 - posX) * deltaDistX;
		}
		if(rayDirY < 0)
		{
			stepY = -1;
			sideDistY = (posY - mapY) * deltaDistY;
		}
		else
		{
			stepY = 1;
			sideDistY = (mapY + 1.0 - posY) * deltaDistY;
		}
		
		//perform DDA
		hit = 0;
		while (hit == 0)
		{
			//jump to next map square, OR in x-direction, OR in y-direction
			if(sideDistX < sideDistY)
			{
				sideDistX += deltaDistX;
				mapX += stepX;
				side = 0; //was a NS or a EW wall hit?
			}
			else
			{
				sideDistY += deltaDistY;
				mapY += stepY;
				side = 1;
			}
			
			//Check if ray has hit a wall
			if(worldMap[mapX][mapY] > 0)
				hit = 1;
		}
		
		//Calculate distance projected on camera direction (Euclidean distance will give fisheye effect!)
		if(side == 0)
			perpWallDist = (mapX - posX + (1 - stepX) / 2) / rayDirX;
		else
			perpWallDist = (mapY - posY + (1 - stepY) / 2) / rayDirY;

		//Calculate height of line to draw on screen
		lineHeight = (int16_t)floorf(h / perpWallDist);

		//calculate lowest and highest pixel to fill in current stripe
		drawStart = -lineHeight / 2 + h / 2;
		if(drawStart < 0)
			drawStart = 0;
		if(drawStart >= h)
			drawStart = h-1;
		drawEnd = lineHeight / 2 + h / 2;
		if(drawEnd >= h)
			drawEnd = h - 1;
		if(drawEnd < 0)
			drawEnd = 0;
		
		//texturing calculations
		texNum = (worldMap[mapX][mapY]-1)%8; //1 subtracted from it so that texture 0 can be used!

		//calculate value of wallX where exactly the wall was hit
		if(side == 0)
			wallX = posY + perpWallDist * rayDirY;
		else
			wallX = posX + perpWallDist * rayDirX;
		wallX -= floorf((wallX));

		//x coordinate on the texture
		texX = (int8_t)floorf(wallX * (float32_t)texWidth);
		if((side == 0) && (rayDirX > 0))
			texX = texWidth - texX - 1;
		if((side == 1) && (rayDirY < 0))
			texX = texWidth - texX - 1;

		// TODO: an integer-only bresenham or DDA like algorithm could make the texture coordinate stepping faster
		// How much to increase the texture coordinate per screen pixel
		step = 1.0 * texHeight / lineHeight;
		
		// Sky
		if((bufnum>1)||(drawStart>=h))
		{
			printf("eep! %d %d\n\r", bufnum, drawStart);
		}
		for(y=0;y<drawStart;y++)
			linebuf[bufnum][y] = __REVSH(ST7789_CYAN);
		
		// Starting texture coordinate
		texPos = (drawStart - h / 2 + lineHeight / 2) * step;
		for(y = drawStart; y < drawEnd; y++)
		{
			// Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
			texY = (int8_t)floorf(texPos) & (texHeight - 1);
			texPos += step;
			
			// get texture from flash or RAM
			if(texNum < 7)
				color = texture[texNum][texHeight * texY + texX];
			else
				color = ramtex[texHeight * texY + (texX + rt_offset)%texWidth];
			
			//make color darker for y-sides: R, G and B byte each divided through two with a "shift" and an "and"
			if(side == 1)
				color =  __REVSH(ST7789_HALFCOLOR( __REVSH(color)));
			
			linebuf[bufnum][y] = color;
		}
		
		// floor
		for(y=drawEnd;y<h;y++)
			linebuf[bufnum][y] = 0;
		ST7789_bitblt(x, 0, 1, h, linebuf[bufnum]);

		bufnum ^= 1;
    }
	
	/* update animation offset */
	rt_offset = (rt_offset + 1)%64;
	
    //timing for input and FPS counter
    oldTime = time;
    time = HAL_GetTick();
    fps = 1000 / (time - oldTime); //FPS counter
    sprintf(textbuf, "%3d", fps);
    gfx_drawstr(0, 232, textbuf);

    //speed modifiers
	frameTime = 1.0F / (float32_t)fps;
	moveSpeed = frameTime * 5.0F; //the constant value is in squares/second
    rotSpeed = frameTime * 3.0F; //the constant value is in radians/second
	
	/* check actions */
    readKeys();
    //move forward if no wall in front of you
    if(keyDown(SDLK_UP))
    {
      if(worldMap[(int16_t)floorf(posX + dirX * moveSpeed)][(int16_t)floorf(posY)] == 0)
		  posX += dirX * moveSpeed;
      if(worldMap[(int16_t)floorf(posX)][(int16_t)floorf(posY + dirY * moveSpeed)] == 0)
		  posY += dirY * moveSpeed;
    }
	
    //move backwards if no wall behind you
    if(keyDown(SDLK_DOWN))
    {
      if(worldMap[(int16_t)floorf(posX - dirX * moveSpeed)][(int16_t)floorf(posY)] == 0)
		  posX -= dirX * moveSpeed;
      if(worldMap[(int16_t)floorf(posX)][(int16_t)floorf(posY - dirY * moveSpeed)] == 0)
		  posY -= dirY * moveSpeed;
    }
	
    //rotate to the right
    if(keyDown(SDLK_RIGHT))
    {
      //both camera direction and camera plane must be rotated
      oldDirX = dirX;
      dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
      dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
      oldPlaneX = planeX;
      planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
      planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
    }
	
    //rotate to the left
    if(keyDown(SDLK_LEFT))
    {
      //both camera direction and camera plane must be rotated
      oldDirX = dirX;
      dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
      dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
      oldPlaneX = planeX;
      planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
      planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
    }
}
