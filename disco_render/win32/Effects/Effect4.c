/*
    Possible Effects: Chorus, Flanger, Phaser
*/

#include "Effect4.h"
#include <stdio.h>

static float phaseBuffer[4096];
static int PHASEBUFFERSIZE_MASK = 4095;
static unsigned int p = 4095;          //default max phase
static int feedback = 0 ; //default half phase volume
static int printCounter = 0;


void updateEffect4Params(float time, float feedbackLine)
{
    //return;
    //update phase shift with time
    if(time*0.085 > 0.085)
        p  = (48000 * 0.085);
    else
        p  = (48000 * time * 0.085);

    if(0 < feedbackLine*5 && feedbackLine*5 < 5)
        feedback = (int)(5*feedbackLine);
    if(printCounter == 0){
        printf("Time: %f, feedback: %d\n\n", time, feedback);
    }
}

/*
float cascade(float liveData, float buffData, float shift, int fback, int head, int tail, int myPointer)
{
    int n = fback;
    if (n > 0)
    {
        return cascade(liveData, buffData, shift, n-1);
    }
    else
    {
        return liveData/fback + buffData/fback;
    }
}
*/

float processEffect4(float sampleIn)
{
    //printf("processing effect 4\n");

    float computedPhase;
    float bufferdata;

    static int phaseHead = 0;
    static int phaseTail = 0;
    static int phasePointer = 0;

    phasePointer = (phaseTail - p) & PHASEBUFFERSIZE_MASK;

    bufferdata = phaseBuffer[phasePointer];
    
    computedPhase = bufferdata;
    //computedPhase = cascade(sampleIn, bufferdata, p, feedback, phaseHead, phaseTail, phasePointer);
    
    phaseBuffer[phaseHead] = computedPhase;          //send to buffer

    phaseHead = ((phaseHead + 1) & PHASEBUFFERSIZE_MASK);       //increment head head= head + 2 & BUFFERSIZE(0x7FFFF)
    phaseTail = (phaseTail + 1) & PHASEBUFFERSIZE_MASK;         // increment tail pointer

    printCounter++;
    if(printCounter == 10000){
        printf("computed phase: %f\n", computedPhase);
        printCounter = 0;
    }
    return computedPhase;
    //return sampleIn;
}