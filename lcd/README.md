# STM32G431 Nucleo LCD demo
This is a simple graphic demo for the STM32G431 Nucleo board using some
raycasting code that was published at

https://lodev.org/cgtutor/raycasting.html

I've modified the original to work with my own embedded graphics library
for the ST7789 IPS LCD, as well as some buttons for navigation.

## prerequisites
To build this you will need the following tools

* gcc-arm-none-eabi 7-2018-q2 or newer
* make
* openocd 2020-04-11 or newer
* ffmpeg version 4.1.5 or newer (for texture mangling)

The LCD demo also requires an ST7789 IPS LCD connected to the G431 Nucleo,
as well as some pushbuttons for navigation.

## Building

	git clone https://github.com/emeb/stm32g431_projects.git
	cd stm32g431_projects/lcd
	make

This should build the LCD raycaster project

## Programming

Hook up the G431 Nucleo via USB and run this command

	make flash
	
That will load the code into the Nucleo board and it should run.

