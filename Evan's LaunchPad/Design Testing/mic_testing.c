/*
 * mic_testing.c
 *
 *  Created on: Jan 27, 2022 1:10 PM
 *      Author: Evan Rives
 */


/*
 * BitCrush_Testing.c
 *
 *  Created on: Jan 27, 2022 09:27 AM
 *      Author: Evan Rives
 *
 *      Test program to see BitCrush Audio effect
 *      reduces the bit depth of the sampled audio
 *      should add neat distortion to audio
 *
 *      Testing sample rate reduction as well which adds
 *
 */



#include <F28x_Project.h>
#include <math.h>
#include "Drivers/SPIDriver.h"
#include "Drivers/InitAIC23.h"

interrupt void Mcbsp_RxINTB_ISR(void);

void GPIO_INIT();

#define PI2 6.283185307179

int16 sample_L = 0;
int16 sample_R = 0;

int16 sampleIn = 0;
int16 sampleOut = 0;

Uint16 Switches = 0;





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
    InitSPIA();         // Initialize SPIA module for Communication with CODEC
    InitAIC23_MIC();        // Initialize CODEC (Currently running DSP mode)
    InitMcBSPb();       // Initalize McbspB (Currently running DSP mode)



    // enabling interrupt in PIE TABLE
    EALLOW;
    PieVectTable.MCBSPB_RX_INT = &Mcbsp_RxINTB_ISR; // Assign RX_Int 1 ISR to PIE vector table
    PieCtrlRegs.PIEIER6.bit.INTx7 = 1;
    IER |= M_INT6;      // Enable INT6 in CPU
    EALLOW;
    EnableInterrupts(); // Enable PIE and CPU interrupts

    EALLOW;
    GpioDataRegs.GPADAT.all |= 0xFFFFFFFF;



    while(1)
    {


    }

    return 0;

}


interrupt void Mcbsp_RxINTB_ISR(void)
{
    //8.43us

    GpioDataRegs.GPADAT.bit.GPIO7 = 1;
    sample_L = McbspbRegs.DRR2.all; // store high word of left channel
    sample_R = McbspbRegs.DRR1.all;

    sampleIn = (sample_L + sample_R) >> 1;

    McbspbRegs.DXR2.all = sampleIn; // send out data
    McbspbRegs.DXR1.all = sampleIn;// send out data
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
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
