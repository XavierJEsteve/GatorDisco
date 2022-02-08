/*
 * SPI_SLAVE_test.c
 *
 *  Created on: Jan 13, 2022
 *      Author: Evan rives
 *
 *      This is program is to test slave mode functionality on the launchpad
 *
 *      program will recieve data from the DAD board and store in variable.
 *
 *      Expressions window will update to show characters successfully received
 *
 *      Launchpad will send back 0x22 to verify transmit success.
 *
 */



#include <F28x_Project.h>
#include <math.h>
#include "Drivers/SPIDriver.h"
#include "Drivers/InitAIC23.h"


void GPIO_INIT();

Uint16 DataReceived = 0;
Uint16 DataReceived2 = 0;
Uint16 DataCombined = 0;
int main(void)
{

    InitSysCtrl(); // Set SYSCLK to 200 MHz, disables watchdog timer, turns on peripheral clocks

    DINT; // Disable CPU interrupts on startup

    // Init PIE
    InitPieCtrl();      // Initialize PIE -> disable PIE and clear all PIE registers
    IER = 0x0000;       // Clear CPU interrupt register
    IFR = 0x0000;       // Clear CPU interrupt flag register
    InitPieVectTable(); // Initialize PIE vector table to known state
    EALLOW;             // EALLOW for rest of program (unless later function disables it)
    GPIO_INIT();        // Initialize Switches and Buttons on CODEC
    SPI_SLAVEINIT();     // SPI B port used





    // enabling interrupt in PIE TABLE
/*
    EALLOW;
    PieVectTable.MCBSPB_RX_INT = &Mcbsp_RxINTB_ISR; // Assign RX_Int 1 ISR to PIE vector table
    PieCtrlRegs.PIEIER6.bit.INTx7 = 1;
    IER |= M_INT6;      // Enable INT6 in CPU


    EnableInterrupts(); // Enable PIE and CPU interrupts
*/

    EALLOW;
    GpioDataRegs.GPADAT.all |= 0xFFFFFFFF;



/*
    GPIO63 SPISIMOB 55
    GPIO64 SPISOMIB 54
    GPIO65 SPICLKB  47
    GPIO66 SPISTEB  59
*/

    while(1)
    {
        DataReceived = SpiBReceive(0x24 << 8);
        DataReceived2 = SpiBReceive(0x13 << 8);
        DataCombined = (DataReceived << 8) + DataReceived2;
    }

    return 0;

}


void GPIO_INIT()
{
    EALLOW;
    //GPIO for pins 0-15
    GpioCtrlRegs.GPAGMUX1.all = 0x00000000;
    GpioCtrlRegs.GPAMUX1.all = 0x00000000;

    GpioCtrlRegs.GPAGMUX2.bit.GPIO16 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 0;

    //make switches inputs
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO9 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO10 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO11 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO14 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO15 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO16 = 0;

    //MAKE LEDs OUTPUTS
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO1 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO2 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO4 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO5 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO6 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;

    //enable pull up resistors
    GpioCtrlRegs.GPAPUD.bit.GPIO8 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO9 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO10 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO11 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO14 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO15 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 0;
}





