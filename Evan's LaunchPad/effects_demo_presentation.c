/*
 * effects_demo_presentation.c
 *
 *  Created on: Feb 7, 2022
 *      Author: rives
 */






#include <F28x_Project.h>
#include <math.h>
#include "Drivers/SPIDriver.h"
#include "Drivers/InitAIC23.h"
#include "Audio_FX/PitchShift.h"
#include "Audio_FX/BitCrush.h"
#include "Audio_FX/Echo.h"

interrupt void Mcbsp_RxINTB_ISR(void);
interrupt void Timer1_isr(void);

void GPIO_INIT();
void InitTimer1(void);
void InitAdca(void);

#define PI2 6.283185307179

int16 sample_L = 0;
int16 sample_R = 0;

int16 sampleIn = 0;
int16 sampleOut = 0;

Uint16 Switches = 0;

volatile Uint16 UpdateParams = 0;
Uint16 CurrentEffect = 0;

volatile Uint16 adcDataCh0, adcDataCh1, adcDataCh2 = 0;

//AUDIO EFFECT PARAMETERS
float32 pitchstep = 1.0;
Uint16 sampleRatio = 0;
float32 time = 0.0;
float32 ratio = 0.5;

//AUDIO EFFECT FLAGS
volatile Uint16 SRReduction = 0;
volatile Uint16 bitCrush = 0;
volatile Uint16 PitchShift = 0;
volatile Uint16 Echo = 0;


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
    InitAdca();         // Initialize ADC A channel 0,1,2
    InitSPIA();         // Initialize SPIA module for Communication with CODEC
    InitAIC23();        // Initialize CODEC (Currently running DSP mode)
    InitMcBSPb();       // Initalize McbspB (Currently running DSP mode)



    // enabling interrupt in PIE TABLE
   EALLOW;
   PieVectTable.MCBSPB_RX_INT = &Mcbsp_RxINTB_ISR; // Assign RX_Int 1 ISR to PIE vector table
   PieCtrlRegs.PIEIER6.bit.INTx7 = 1;
   IER |= M_INT6;      // Enable INT6 in CPU
   EALLOW;
   InitTimer1();       // Initialize CPU timer 1
   //EnableInterrupts(); // Enable PIE and CPU interrupts

   EALLOW;
   GpioDataRegs.GPADAT.all |= 0xFFFFFFFF;



    while(1)
    {
        CurrentEffect = (GpioDataRegs.GPADAT.bit.GPIO15 << 2) + (GpioDataRegs.GPADAT.bit.GPIO14 << 1) + GpioDataRegs.GPADAT.bit.GPIO11;

        // add an if statement which prevents effect from starting.
        //prevents using old adc data on new effect which may have odd effects

        if(GpioDataRegs.GPADAT.bit.GPIO16 == 1)
        {
            if(bitCrush == 1)
            {
                //bitcrushing
                //value of 1 - 16 bits

                updateBitDepth(sampleRatio);
                sampleOut = ProcessBitCrush(sampleIn);
                bitCrush = 0;

            }
            else if(SRReduction == 1)
            {
                //sample rate reduction
                //value of 1 - 16

                updateSampleRate(sampleRatio);
                sampleOut = ProcessSampleRateReduction(sampleIn);
                SRReduction = 0;

            }
            else if(Echo == 1)
            {
                //echo
                //

                updateEchoParams(time, ratio);
                sampleOut = processEcho(sampleIn);
                Echo = 0;

            }
            else if(PitchShift == 1)
            {
                //pitchShifting
                updatePitch(pitchstep);
                sampleOut = processPitchShift(sampleIn);
                PitchShift = 0;
            }

        }


    }

    return 0;

}

interrupt void Timer1_isr(void) {

    //AdcaRegs.ADCSOCFRC1.all = 0x1;          //0x07 == Force conversion on channel 0,1,2 | 0x03 == conversion on channel 0,1

    adcDataCh0 = AdcaResultRegs.ADCRESULT0;    // Read ADC result into global variable
    adcDataCh1 = AdcaResultRegs.ADCRESULT1;
    adcDataCh2 = AdcaResultRegs.ADCRESULT2;

    sampleRatio = (15 * (adcDataCh0/4095.0)) + 1;
    time = ((0.055f * (float32)(adcDataCh0/4095.0) + 0.030f));
    ratio  = (float32)(adcDataCh1/4095.0);
    pitchstep = (1.5*(adcDataCh0 / 4095.0)) + 0.5f;

}


interrupt void Mcbsp_RxINTB_ISR(void)
{
    //8.43us
    if(CurrentEffect == 0)
        bitCrush = 1;
    else if(CurrentEffect == 1)
        SRReduction = 1;
    else if(CurrentEffect == 2)
        Echo = 1;
    else if(CurrentEffect == 3)
        PitchShift = 1;

    sample_L = McbspbRegs.DRR2.all; // store high word of left channel
    sample_R = McbspbRegs.DRR1.all;

    sampleIn = (sample_L + sample_R) >> 1;

    McbspbRegs.DXR2.all = (int16)sampleOut; // send out data
    McbspbRegs.DXR1.all = (int16)sampleOut;// send out data
    sampleOut = sampleIn;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
}

void InitAdca(void) {
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6;                                 // Set ADCCLK to SYSCLK/4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE); // Initializes ADCA to 12-bit and single-ended mode. Performs internal calibration
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;                                 // Powers up ADC
    DELAY_US(1000);                                                    // Delay to allow ADC to power up

    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;                                 // Sets SOC0 to channel 0 -> pin ADCINA0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 23;                                // Sets sample and hold window -> must be at least 1 ADC clock long
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL= 2;                                // Set the adc to trigger on Timer1 interrupt

    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 1;                                 // Sets SOC0 to channel 0 -> pin ADCINA1
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = 23;                                // Sets sample and hold window -> must be at least 1 ADC clock long
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL= 2;                                // Set the adc to trigger on Timer1 interrupt

    AdcaRegs.ADCSOC2CTL.bit.CHSEL = 2;                                 // Sets SOC0 to channel 0 -> pin ADCINA0
    AdcaRegs.ADCSOC2CTL.bit.ACQPS = 23;                                // Sets sample and hold window -> must be at least 1 ADC clock long
    AdcaRegs.ADCSOC2CTL.bit.TRIGSEL= 2;                                // Set the adc to trigger on Timer1 interrupt
}

void InitTimer1(void) {
    InitCpuTimers();                            // Initialize all timers to known state
    ConfigCpuTimer(&CpuTimer1, 200, 500000);    // Configure CPU timer 1. 200 -> SYSCLK in MHz, 100,000 -> period in usec. NOTE: Does NOT start timer
    PieVectTable.TIMER1_INT = &Timer1_isr;      // Assign timer 1 ISR to PIE vector table
    IER |= M_INT13;                             // Enable INT13 in CPU
    EnableInterrupts();                         // Enable PIE and CPU interrupts
    CpuTimer1.RegsAddr->TCR.bit.TSS = 0;        // Start timer 1
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
