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

   int spiData = 0;
   while(1)
   {
   	buffer[0] = spiData;
      spiData++;
      buffer[1] = spiData;
      spiData++;
      result = wiringPiSPIDataRW(CHANNEL, buffer, 2);
   }
}
