# Gator Disco
## By Xavier Esteve, Luke Jones, and Evan Rives
### This is a **prototype** for our 2021-2022 Senior Design capstone project

This application is currently comprised of a pre-compiled raylib library and main.c source file to run the synthesizer, located in disco_render.

The Project involves the GUI handling the control signals, which is sent to the DSP launchpad for audio processing. The DSP board operates at 200MHz and includes a audio codec with 16-bit ADC/DAC. 

Our Alpha build includes the R.PI running the GUI and sending control signals to the DSP via SPI and generating notes via PWM and being able to manipulate the audio by controlling an 7-band equalizer. 

There is also a web server framework built on Django that can be found in disco_server.

In order to run the applications, one should seperately start the gui application by executing main.c.

The application can be built by running 'make main.c PLATFORM=PLATFORM_RPI' from disco_render. This will use the static libraylib library and raylib.h file to generate an executable.
* In the abscence of an existing library file, the Makefile currently expects raylib source code to be located at /home/pi/raylib. Raylib can be cloned from https://github.com/raysan5/raylib.
*  For desktop, Raylib PREFIX
* PLATFORM=PLATFORM_RPI can be withheld in the case this is not being built on a Raspberry Pi. The makefile will default to PLATFORM=PLATFORM_DESKTOP instead.
 


The synthesizer should now be present on the raspberry pi. It can be closed at any time by killing the proicess or pressing the "escape" key.
The Webserver will be accessible from any machine on the same network following these steps:
1. VVerify ip address of the pi by checking 'ip a' or ipconfig on windows.
2. Change directy to disco_server/
3. $ pip3 install -r requiremnets.txt
4. $ python3 manage.py runserver 0.0.0.0:<PORT#> 
6. Django should report if the service is running.
7. Visit the ip address and port of the pi from another machine. An example may be 10.192.192.63:<PORT#>

  
  
In the place of make, cmake can also be used to build dependencies cross-platform. It will even install raylib if it is found to be missing.
1.	Install CMake
  a.	https://cmake.org/download/
  b.	Add it to your path

2.	Create sub directory for building files underneath your existing source code. 
  i.	mkdir build # Create a build directory
  ii.	cd build && cmake .. # Build from that directory so the build files are in one place
  iii.	cmake --build . # Build the project
4.	Run the new executable

To be able to run and compile C code onto the Lauchpad, please read CCS_C_Project_Creation_V2020.pdf
