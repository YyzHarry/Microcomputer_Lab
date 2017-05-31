# Final Project of Microcomputer Lab

## Introduction
This is my final project for ___Experiments on Microcomputer and Interface Technology___ Course, EE, Peking University.

___An Elevator Control System___.
Implemented by __C51__.

The latest version is __3.1__.

## Demo
This picture is the final demo of the project.

### Version 1.0
Implemented Finite State Machine, with LED displaying the position of the elevator.

### Version 2.0
Added TFT module and Lcd1602 module, displaying with ___graphic interface___ on TFT.

### Version 3.0
Implemented ADC & DAC, realizing recording of "Beep tone" in every stair. Data is stored directly in XRAM, and has limited storage.

### Version 3.1
Based on version 3.0, recording and displaying voice data through SPI Flash (using __Record_in.c__ to write data into Flash).

## Environment
Locally tested by ___Keil___. ___C8051F020___ is the chip we use.