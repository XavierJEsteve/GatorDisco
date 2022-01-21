/*
 * UARTDriver.c
 *
 *  Created on: Nov 19, 2021
 *      Author: rives
 */


#include "UARTDRiver.h"



void UART_INIT()
{

    Uint16 tmp;

    //SysCtrlRegs.LOSPCP.bit.LSPCLK = 1;
    //SysCtrlRegs.PCLKCR0.bit.SCIBENCLK = 1;
    GpioCtrlRegs.GPBGMUX1.bit.GPIO42 = 3;
    GpioCtrlRegs.GPBGMUX1.bit.GPIO43 = 3;
    GpioCtrlRegs.GPBMUX1.bit.GPIO42 = 3;
    GpioCtrlRegs.GPBMUX1.bit.GPIO43 = 3;
    ScibRegs.SCICCR.all = 7;

    tmp = tmp & 0x000A;
    SciaRegs.SCIHBAUD.bit.BAUD = 0xA;


    SciaRegs.SCILBAUD.bit.BAUD = 0x2A;

    SciaRegs.SCICCR.bit.SCICHAR = 7;
    SciaRegs.SCICCR.bit.STOPBITS = 0;


    SciaRegs.SCICTL1.bit.RXENA = 1;
    SciaRegs.SCICTL1.bit.TXENA = 1;
    DELAY_US(5000);
    SciaRegs.SCICTL1.bit.SWRESET = 1;

    //
    // Configure SCIA for echoback.
    //

    //SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 9600, (7 | 0 | 0)); //8-bit cahr length, 1 stop bit, no parity bit


}

char IN_CHAR()
{

}
