/*
 * SRAMDriver.h
 *
 *  Created on: Sep 30, 2021
 *      Author: rives
 */

#ifndef SRAMDRIVER_H_
#define SRAMDRIVER_H_

#include <F28x_Project.h>
#include "SPIDriver.h"

void SRAMWrite(Uint16 data,long int address);

Uint16 SRAMRead(long int address);

void SRAMZero();

#endif /* SRAMDRIVER_H_ */
