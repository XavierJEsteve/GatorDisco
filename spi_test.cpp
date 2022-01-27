******************************************************************************/

#include <iostream>
#include <errno.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

// channel is the wiringPi name for the chip select (or chip enable) pin.
// Set this to 0 or 1, depending on how it's connected.
static const int CHANNEL = 0;

int main()
{
   int fd, result;
   unsigned char buffer[100];

   // CHANNEL insicates chip select,
   // 500000 indicates bus speed.
   fd = wiringPiSPISetup(CHANNEL, 500000);


   while(1)
   {
   	buffer[0] = 128;
	result = wiringPiSPIDataRW(CHANNEL, buffer, 1);	
	buffer[0] = 0;
	result = wiringPiSPIDataRW(CHANNEL, buffer, 1);
	delay(1000);
	buffer[0]
   }
}
