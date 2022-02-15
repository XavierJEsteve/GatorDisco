/*
 * Effects_SPI.c
 *
 *  Created on: Feb 15, 2022
 *      Author: Evan Rives
 *
 *      Testing using SPI to control Pitchshift parameter
 *
 *
 */


#include <F28x_Project.h>
#include <math.h>
#include "Drivers/SPIB.h"
#include "Drivers/InitAIC23.h"
#include "Audio_FX/PitchShift.h"
#include "Drivers/SPIDriver.h"


interrupt void Spi_RxINTB_ISR(void);
interrupt void Mcbsp_RxINTB_ISR(void);

int16 sample_L = 0;
int16 sample_R = 0;
int16 sampleIn = 0;
int16 sampleOut = 0;

float32 PitchStep = 0;
volatile Uint16 pitchenable = 0;
volatile Uint16 pitchflag = 0;

Uint16 SPIdata = 0;

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
    SPIB_Slave_Init();  // Initializes the SPI B port to be in slave mode
    GPIO_INIT();        // Initialize Switches and Buttons on CODEC
    InitSPIA();         // Initialize SPIA module for Communication with CODEC
    InitAIC23();        // Initialize CODEC (Currently running DSP mode)
    InitMcBSPb();       // Initalize McbspB (Currently running DSP mode)



    // enabling interrupt in PIE TABLE
    EALLOW;
    PieVectTable.MCBSPB_RX_INT = &Mcbsp_RxINTB_ISR; // Assign RX_Int 1 ISR to PIE vector table
    PieVectTable.SPIB_RX_INT = &Spi_RxINTB_ISR;
    PieCtrlRegs.PIEIER6.bit.INTx7 = 1; //McBSP
    PieCtrlRegs.PIEIER6.bit.INTx3 = 1; //SPIB
    IER |= M_INT6;      // Enable INT6 in CPU
    EALLOW;
    EnableInterrupts(); // Enable PIE and CPU interrupts

    EALLOW;
    GpioDataRegs.GPADAT.all |= 0xFFFFFFFF;



    while(1)
    {

        if(pitchflag == 1)
        {
            GpioDataRegs.GPADAT.bit.GPIO7 = 1;
            sampleOut = processPitchShift(sampleIn);
            pitchflag = 0;
            GpioDataRegs.GPADAT.bit.GPIO7 = 0;
        }



    }

    return 0;

}


interrupt void Mcbsp_RxINTB_ISR(void)
{
    //will have multiple flags to set depending on the enable flag in the SPI interrupt
    if(pitchenable == 1)
        pitchflag = 1;

    sample_L = McbspbRegs.DRR2.all; // store high word of left channel
    sample_R = McbspbRegs.DRR1.all;

    sampleIn = (sample_L + sample_R) >> 1;

    McbspbRegs.DXR2.all = (int16)sampleOut; // send out data
    McbspbRegs.DXR1.all = (int16)sampleOut;// send out data
    sampleOut = sampleIn; // if no effects are on then just do simple input/output
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
}

interrupt void Spi_RxINTB_ISR(void){

    SPIdata = SpibRegs.SPIRXBUF & 255;
/*    spiData[spiDataPointer] = data;
    spiDataPointer++;
    spiDataPointer %= 100;*/

    Uint16 EffectSel;

    EffectSel = (SPIdata >> 4) & 0x3; //Shift by 4 to isolate the 3 bits which choose the effect we run (MSB will be zero for turning off, 1 for turning on effect)

    //====================================CONTROLS WHAT EFFECT WILL BE UPDATED=============================================================

    if(EffectSel == 0) //add an OR later to add an additional condition which the eq can be updated by. Saves having to repeat bits we send
        //EQ EFFECT
        GpioDataRegs.GPADAT.bit.GPIO7 = 0;

    else if(EffectSel == 1)
    {
        //BITCRUSH EFFECT (will probably be a toggle)
        GpioDataRegs.GPADAT.bit.GPIO7 = 0;
    }
    else if(EffectSel == 2)
    {
        //PITCHSHIFT EFFECT

        if(SPIdata >> 7 == 1)
        {
            pitchenable = 1;
            PitchStep = (1.5f * (float)((SPIdata & 0x0F)/15.0f)) + 0.5f;
            updatePitch(PitchStep);
        }
        else
            pitchenable = 0;

    }
    else if(EffectSel == 3)
    {
        //SAMPLE RATE REDUCTIION EFFECT
        GpioDataRegs.GPADAT.bit.GPIO7 = 0;
    }
    else if(EffectSel == 4)
    {
        //ECHO EFFECT
        GpioDataRegs.GPADAT.bit.GPIO7 = 0;

    }
    else
    {
        GpioDataRegs.GPADAT.bit.GPIO7 = 0;
    }


    SpibRegs.SPISTS.bit.INT_FLAG = 0;
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
