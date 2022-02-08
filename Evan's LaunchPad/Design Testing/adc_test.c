/*
 * adc_test.c
 *
 *  Created on: Jan 12, 2022
 *      Author: Evan Rives
 *
 *  This program is to test ADCA channels 0,1,2 for control of
 *  parameters Fceneter, Q, and Gain for Equalizer
 *
 */



#include <F28x_Project.h>
#include <math.h>
#include "Drivers/SPIDriver.h"
#include "Drivers/InitAIC23.h"

//interrupt void Mcbsp_RxINTB_ISR(void);
interrupt void time1_isr(void);

void GPIO_INIT();
void InitTimer1(void);
void InitAdca(void);

#define PI2 6.2832


//biquad variable parameters
float32 dbGain = 0.0; //keep between +/- 15dB
float32 alpha = 0.0;
float32 A = 0.0;    //this will control the gain (I think)
float32 w0 = 0.0;
float32 cosParam = 0.0; //cos(w0)
float32 sinParam = 0.0; //sin(w0)
float32 Q = 0.707;    //vary between 0.1 and 10. Bigger Q == narrow width
float32 fCenter = 2000.0; //(keep between 10 and 1000 Hz)
Uint16 Fs = 48000;

int16 sampleIn = 0;
int16 sampleOut = 0;

//filter coefficients
float32 b0,b1,b2,a0,a1,a2 = 0.0;

//delay samples
int16 xd1,xd2,yd1,yd2 = 0;

//adc data
Uint16 adcDataCh0, adcDataCh1,adcDataCh2 = 0;
float32 cFreqRatio, gainRatio, qRatio = 0.0;


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
    InitTimer1();       // Initialize CPU timer 1
    InitAdca();         // Initialize ADC A channel 0,1,2

/*
    InitSPIA();         // Initialize SPIA module for Communication with CODEC
    InitAIC23();        // Initialize CODEC (Currently running DSP mode)
    InitMcBSPb();       // Initalize McbspB (Currently running DSP mode)
*/



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






    //calculate default biquad parameters
    A = powf(10.0,dbGain/40.0);  //dBGain of 0, no gain, basic sound in/out
    w0 = PI2 * (float32)(fCenter/Fs);
    cosParam = cosf(w0);
    sinParam = sinf(w0);
    alpha = (sinParam/(2*Q));


    //coeffiecients will be placed in a struct corresponding to the frequency band
    //coeffiecients will change between freq bands but the sample data needed will not
    //a function could be created to calculate output of 1 band and pass thorugh next band(coeffiecients)
    //static variable could keep track of what filter coeffiecients to use by controlling the index of array of structs

    while(1)
    {

        //get ratio of each channel to control biquad parameters
        cFreqRatio = (adcDataCh0 / 4095.0);
        gainRatio = (adcDataCh1/ 4095.0);
        qRatio = (adcDataCh2/ 4095.0);

        //get range of parameters and multiply by ratio to get value to add to low end
        // i.e : 1000Hz - 10Hz = 990 Hz  ==> 990Hz * (0.5 ratio) = 495 ==> 495Hz + 10Hz = 505Hz
        fCenter = ((990 * cFreqRatio) + 10);
        dbGain  = ((30 * gainRatio) + -15);
        Q       = ((9.9 * qRatio) + 0.1);

    }

    return 0;

}

void InitAdca(void) {
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6;                                 // Set ADCCLK to SYSCLK/4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE); // Initializes ADCA to 12-bit and single-ended mode. Performs internal calibration
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;                                 // Powers up ADC
    DELAY_US(1000);                                                    // Delay to allow ADC to power up

    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;                                 // Sets SOC0 to channel 0 -> pin ADCINA0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 14;                                // Sets sample and hold window -> must be at least 1 ADC clock long
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL= 2;                                // Set the adc to trigger on Timer1 interrupt

    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 1;                                 // Sets SOC0 to channel 0 -> pin ADCINA1
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = 14;                                // Sets sample and hold window -> must be at least 1 ADC clock long
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL= 2;                                // Set the adc to trigger on Timer1 interrupt

    AdcaRegs.ADCSOC2CTL.bit.CHSEL = 2;                                 // Sets SOC0 to channel 0 -> pin ADCINA0
    AdcaRegs.ADCSOC2CTL.bit.ACQPS = 14;                                // Sets sample and hold window -> must be at least 1 ADC clock long
    AdcaRegs.ADCSOC2CTL.bit.TRIGSEL= 2;                                // Set the adc to trigger on Timer1 interrupt
}

void InitTimer1(void) {
    InitCpuTimers();                            // Initialize all timers to known state
    ConfigCpuTimer(&CpuTimer1, 200, 100000);    // Configure CPU timer 1. 200 -> SYSCLK in MHz, 100,000 -> period in usec. NOTE: Does NOT start timer
    PieVectTable.TIMER1_INT = &time1_isr;      // Assign timer 1 ISR to PIE vector table
    IER |= M_INT13;                             // Enable INT13 in CPU
    EnableInterrupts();                         // Enable PIE and CPU interrupts
    CpuTimer1.RegsAddr->TCR.bit.TSS = 0;        // Start timer 1
}



interrupt void time1_isr(void) {

    //AdcaRegs.ADCSOCFRC1.all = 0x1;          //0x07 == Force conversion on channel 0,1,2 | 0x03 == conversion on channel 0,1

    adcDataCh0 = AdcaResultRegs.ADCRESULT0;    // Read ADC result into global variable
    adcDataCh1 = AdcaResultRegs.ADCRESULT1;
    adcDataCh2 = AdcaResultRegs.ADCRESULT2;
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
