# CS4222/CS5222 Wireless Networking
### Group 1

This repository contains the code for the CS4222/CS5222 project for AY22/23. The project requirements can be found [here](https://weiserlab.github.io/ambuj/cs4222_project).

The code has been tested to compile on Ubuntu CLI 22.04 with Contiki OS and the ARM GCC compiler. Installation steps are provided below.

The project makes use of the Texas Instruments SensorTag microcontroller. UniFlash is required to flash the SensorTag with the program. RealTerm is used to visualise outputs from the SensorTag via the serial port.

## Setup
1. Install Contiki OS in your chosen repository using the following command in a bash terminal in Ubuntu.
```
git clone https://github.com/contiki-ng/contiki-ng.git --recursive
```
Subsequently, clone this repository into the `contiki-ng/examples/` directory.

2. Install the ARM GCC compiler using the following command in a bash terminal in Ubuntu.
```
sudo apt-get install gcc-arm-none-eabi
```
If the above command produces an error, follow the alternate commands provided below.
```
sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install gcc-arm-none-eabi
sudo apt-get install gdb-arm-none-eabi
```

3. Install UniFlash. Both the [online version](https://dev.ti.com/uniflash/#!/) or the [offline version](https://www.ti.com/tool/download/UNIFLASH) will work.

4. Install [RealTerm](https://sourceforge.net/projects/realterm/) to visualise outputs from the SensorTag via the serial port.

## Running a program
1. Go to your local directory containing this repository. Compile the program using the following command in a bash terminal in Ubuntu.
```
sudo make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 PORT=/dev/ttyACM0 <FILENAME ONLY of file to be compiled>
```
2. Using UniFlash, flash the compiled file of filetype `.cc26x0-cc13x0` onto the SensorTag.
3. Use RealTerm to visualise outputs from the SensorTag via the serial port. 

## Running Task 1
This task requires 2 SensorTags, one of which will be Node A and the other Node B. Flash each SensorTag with the corresponding `task1_<node ID>.cc26x0-cc13x0` image.

## Running Task 2
This task requires 2-4 SensorTags. Let 1 of the SensorTags be the transmitter, and the other 1-3 SensorTags be the receivers. Flash the transmitter with the `task2_light_sensor.cc26x0-cc13x0` image, and flash the receivers with the `task2_receiver.cc26x0-cc13x0` iamge.
