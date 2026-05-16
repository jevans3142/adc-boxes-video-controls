<img width="280" align="right" src="https://raw.githubusercontent.com/CHTJonas/roombooking/master/public/logo-long-black.svg?sanitize=true">

# ADC Theatre - Control Boxes Video Switchers

This repository hosts the hardware (in the form of KiCad schematics, PCB layouts, 3D print files, and metalwork cutting outines) and 
firmware (in the form of C code) of the [ADC Theatre's](https://www.adctheatre.com) Control Boxes Video Switchers. These are a pair of control panels mounted in each control box which 
provides remote control of a Blackmagic Design Videohub SDI switcher via Ethernet.

## Features
* Each panel controls a single video output on the switcher, with six sources 
* Panels have steel panel fronts and backs, with a 3D printed case around them
* An additional steel plate mounts the assembly to a VESA mount behind a screen to drop the panel neatly under the screen

## Configuration

Although some things such as number of buttons etc are fixed in hardware, some aspects can be customised via a text file 
on a microSD card read at boot. This provides the ADC with the ability to reconfigure for example which sources are availible on the panel without having to recompile firmware (as long as the physical buttons are relabeled of course!)

Items which can be configured: 
* Which router inputs are used as sources for each of the destinations 
* IP address of the router


## Compilation
The microcontroller used is an ESP32 on an Olimex ESP32-PoE-ISO board. After standard installation of the esp-idf FreeRTOS toolchain, currently building on v5.1.1 as a stable version with the configuration included in the src folder (ie. when building do not run the idf.py set-target esp32 command as directed in the esp-idf Getting Started instructions to set up the default build config - just go straight to idf.py build)

## Hardware
Each panel requires the following components: 
* A Olimex ESP32-PoE-ISO board
* A custom PCB 'main-board' to break out the button/LED connections
* A 3D printed case, which various heat-fit standoffs need to be sunk into to mount the main-board PCB and front/back panels
* A front panel consisting of a laser cut steel panel with buttons/LEDs mounted in; this also has a label strip. A 3D printed version is in the repo, but there is an off-the-shelf part that can be used too 
* A back panel with another 3D printed part used as an SD card cover
* A VESA mounting steel plate 

## Copyright
Copyright (c) 2021-2024 John Evans and contributors.
The ADC Control Boxes Video Switcher hardware and firmware is released under Version 3 of the GNU General Public License.
See the LICENSE file for full details.
