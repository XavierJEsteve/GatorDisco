/*
 * SPIDriver.h
 *
 *  Created on: Sep 30, 2021
 *      Author: rives
 */

#ifndef SPIDRIVER_H_
#define SPIDRIVER_H_
#include <F28x_Project.h>
//=================================================================================================
void SPI_INITA(Uint16 SPI_CLKDIVIDER); //used by codec, InitAIC23.c
void SPI_INITB(Uint16 SPI_CLKDIVIDER);
void SPI_SLAVEINIT();                   //will use SPIB, as SPIA is used by codec



Uint16 SpiATransmit(Uint16 data);
Uint16 SpiBTransmit(Uint16 data);
Uint16 SpiBReceive(Uint16 data);

void SpiWrite(Uint16 data);

Uint16 SpiRead(int address);

//==================================================================================================


#endif /* SPIDRIVER_H_ */
