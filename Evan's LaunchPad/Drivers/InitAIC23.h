/*
 * InitAIC23.h
 *
 *  Created on: Oct 13, 2021
 *      Author: rives
 */

#ifndef DRIVERS_INITAIC23_H_
#define DRIVERS_INITAIC23_H_


#include <stdint.h>
#include <F28x_Project.h>
#include "AIC23.h"

/***************** Defines ***************/
#define CodecSPI_CLK_PULS {EALLOW; GpioDataRegs.GPASET.bit.GPIO18 = 1; GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;}
#define CodecSPI_CS_LOW {EALLOW; GpioDataRegs.GPACLEAR.bit.GPIO19 = 1;}
#define CodecSPI_CS_HIGH {EALLOW; GpioDataRegs.GPASET.bit.GPIO19 = 1;}

/***************** User Functions *****************/
void InitMcBSPb();
void InitSPIA();
void InitAIC23();
void SpiTransmit(uint16_t data);



#endif /* DRIVERS_INITAIC23_H_ */
