/*
 * Regalia-MitraEQ.c
 *
 *  Created on: Jan 18, 2022
 *      Author: Evan rives
 *
 *      Regalia-Mitra EQ is a filter structure where we utilize an all pass filter
 *      to achieve low frequency performance
 *
 *
 */



#include <F28x_Project.h>
#include <math.h>
#include "Drivers/SPIDriver.h"
#include "Drivers/InitAIC23.h"

interrupt void Mcbsp_RxINTB_ISR(void);
interrupt void Timer1_isr(void);

void GPIO_INIT();
void InitTimer1(void);
void InitAdca(void);

#define PI2 6.283185307179

int16 sample_L = 0;
int16 sample_R = 0;


//biquad variable parameters
float32 dbGain = 0.0; //keep between +/- 15dB
float32 K = 0.0;
float32 a = 0.0;    //this will control the gain (I think)
float32 B = 0.0;
float32 b = 0.0;;

float32 Q = 1.414;    //Q will be static, this covers one octave
float32 fCenter = 5100.0; //(keep between 10 and 1000 Hz)
Uint16 Fs = 48000;

int16 sampleIn = 0;
float32 sampleOut, APFOut = 0.0;

//filter coefficients
float32 b0,b1,b2,a0,a1,a2 = 0.0;

//delay samples
float32 xd1,xd2,yd1,yd2,z1,z2 = 0;

//adc data
Uint16 adcDataCh0, adcDataCh1,adcDataCh2 = 0;
float32 cFreqRatio, gainRatio, qRatio = 0.0;
Uint16 ParamUpdate = 0;


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
    InitAdca();         // Initialize ADC A channel 0,1,2
    GPIO_INIT();        // Initialize Switches and Buttons on CODEC
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






    //calculate default biquad parameters
    B = fCenter / Q;
    K = powf(10, dbGain/20.0);           //controls gain
    a = (1 - tanf((float32)(M_PI * B/Fs))/(1+tanf((float32)(M_PI * B/Fs))));
    b =  -1*cosf((float32)((PI2*fCenter)/Fs));   //controls center frequency



    //coeffiecients will be placed in a struct corresponding to the frequency band
    //coeffiecients will change between freq bands but the sample data needed will not
    //a function could be created to calculate output of 1 band and pass thorugh next band(coeffiecients)
    //static variable could keep track of what filter coeffiecients to use by controlling the index of array of structs

    while(1)
    {

        if(ParamUpdate == 1)
        {
            //get ratio of each channel to control biquad parameters
            GpioDataRegs.GPADAT.bit.GPIO7 = 1; //pin 79
            cFreqRatio = (adcDataCh0 / 4095.0);
            gainRatio = (adcDataCh1/ 4095.0);
            qRatio = (adcDataCh2/ 4095.0);

            //get range of parameters and multiply by ratio to get value to add to low end
            // i.e : 1000Hz - 10Hz = 990 Hz  ==> 990Hz * (0.5 ratio) = 495 ==> 495Hz + 10Hz = 505Hz
            fCenter = ((4250 * cFreqRatio) + 250);
            dbGain  = ((30 * gainRatio) + -15);
            Q       = ((9.9 * qRatio) + 0.1);

            //these values along with the coefficients will be placed in a struct ideally (possibly previous samples as well)
            //re-calculate variables for filter coeffiecients
            B = fCenter / Q;

            K = powf(10, dbGain/20.0);           //controls gain
            a = (1 - tanf((float32)(M_PI * B/Fs))/(1+tanf((float32)(M_PI * B/Fs))));
            b =  -1*cosf((float32)((PI2*fCenter)/Fs));   //controls center frequency

            //might do small size 2 arrays to keep track of previous outputs????
            b0 = ((1 + a + K - K*a)) * 0.5;
            b1 = (b + b*a);
            b2 = (1 + a - K + K*a) * 0.5;
            a1 = b1;           //a1 same as b1
            a2 = a;

            ParamUpdate = 0;
            GpioDataRegs.GPADAT.bit.GPIO7 = 0; //pin 79
        }


        //DIRECT FORM 1 DIFFERENCE EQUATION
        //y[n]= (b0/a0)*x[n] + (b1/a0)x[n-1] + (b2/a0)x[n-2] - (a1/a0)y[n-1] - (a2/a0)y[n-2]

        APFOut = (b0)*sampleIn + (b1)*xd1 + (b2)*xd2 - (a1)*yd1 - (a2)*yd2;

        //shift the samples
        xd2 = xd1;
        xd1 = sampleIn;
        yd2 = yd1;
        yd1 = APFOut;



        //TRANSPOSED DIRECT FORM 2
        // y[n] = a[0]*x[n] + d[0];
        // d[0] = a[1]*x[n] - b[1]*y[n] + d[1];
        // d[1] = a[2]*x[n] - b[2]*y[n] + d[2];

/*        APFOut = (b0)*sampleIn + z1;
        z1 = (b1)*sampleIn - (a1)*APFOut + z2;
        z2 = (b2)*sampleIn - (a2)*APFOut;*/

        sampleOut = 0.5*(sampleIn + APFOut) + (float32)(K/2)*(sampleIn - APFOut);

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
    ConfigCpuTimer(&CpuTimer1, 200, 500000);    // Configure CPU timer 1. 200 -> SYSCLK in MHz, 100,000 -> period in usec. NOTE: Does NOT start timer
    PieVectTable.TIMER1_INT = &Timer1_isr;      // Assign timer 1 ISR to PIE vector table
    IER |= M_INT13;                             // Enable INT13 in CPU
    EnableInterrupts();                         // Enable PIE and CPU interrupts
    CpuTimer1.RegsAddr->TCR.bit.TSS = 0;        // Start timer 1
}

interrupt void Mcbsp_RxINTB_ISR(void)
{
    //8.43us
    //GpioDataRegs.GPADAT.bit.GPIO7 = 1; //pin 79
    sample_L = McbspbRegs.DRR2.all; // store high word of left channel
    sample_R = McbspbRegs.DRR1.all;

    sampleIn = (sample_L + sample_R) >> 1;

    McbspbRegs.DXR2.all = (int16)sampleOut; // send out data
    McbspbRegs.DXR1.all = (int16)sampleOut;// send out data
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
}

interrupt void Timer1_isr(void) {

    //AdcaRegs.ADCSOCFRC1.all = 0x1;          //0x07 == Force conversion on channel 0,1,2 | 0x03 == conversion on channel 0,1

    ParamUpdate = 1;
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
