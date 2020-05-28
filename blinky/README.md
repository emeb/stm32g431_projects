# STM32G431 Nucleo LED
This is a basic LED blinky for starting with the STM32G431 Nucleo board

## prerequisites
To build this you will need the following tools

* gcc-arm-none-eabi 7-2018-q2 or newer
* make
* openocd 2020-04-11 or newer

## Building

	git clone https://github.com/emeb/stm32g431_projects.git
	cd stm32g431_projects/blinky
	make

This should build the blinky project

## Programming

Hook up the G431 Nucleo via USB and run this command

	make flash
	
That will load the code into the Nucleo board and it should run.

