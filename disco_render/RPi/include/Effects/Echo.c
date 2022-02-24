/*
 * Echo.c
 *
 *  Created on: Jan 27, 2022 12:28 PM
 *      Author: Evan Rives
 */

#include "Echo.h"
#include <stdio.h>

static float echoBuffer[4096];
static int ECHOBUFFERSIZE_MASK = 4095;
static unsigned int p = 4095;          //default max echo
static float echoRatio = 0.5; //default half echo volume


void updateEchoParams(float time, float ratio)
{
    /*
     * Updates the echo time and volume
     * time is in ms
     */

    if(time > 0.085)
        p  = (48000 * 0.085);
    else
        p  = (48000 * time);

    if(ratio > 1.0)
        echoRatio = 1.0;
    else
        echoRatio = ratio;


}

float processEcho(float sampleIn)
{


    /*
     * Stores sample in echo buffer for later processing
     * allows user to adjust the volume of echo with parameter ratio
     * max echo time possible is 85ms: y[n-p] where p = echoTime * sampling Freq
     */
    //printf("effects output: %f\n", sampleIn);
    float computedEcho;
    float bufferdata;

    static int echoHead = 0;
    static int echoTail = 0;
    static int echoPointer = 0;

    echoPointer = (echoTail - p) & ECHOBUFFERSIZE_MASK;

    bufferdata = echoBuffer[echoPointer];

    computedEcho = ((1-echoRatio)*sampleIn + echoRatio*bufferdata);

    echoBuffer[echoHead] = computedEcho;          //send to buffer

    echoHead = ((echoHead + 1) & ECHOBUFFERSIZE_MASK);       //increment head head= head + 2 & BUFFERSIZE(0x7FFFF)
    echoTail = (echoTail + 1) & ECHOBUFFERSIZE_MASK;         // increment tail pointer

    return computedEcho;

}