/*
 * Echo.c
 *
 *  Created on: Jan 27, 2022 12:28 PM
 *      Author: Evan Rives
 */

#include "Echo.h"

#pragma DATA_SECTION(echoBuffer, "ramgs0");

static int16 echoBuffer[4096];
static int16 ECHOBUFFERSIZE_MASK = 4095;
static Uint16 p = 4095;          //default max echo
static float32 echoRatio = 0.5; //default half echo volume


void updateEchoParams(float32 time, float32 ratio)
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

int16 processEcho(int16 sampleIn)
{


    /*
     * Stores sample in echo buffer for later processing
     * allows user to adjust the volume of echo with parameter ratio
     * max echo time possible is 85ms: y[n-p] where p = echoTime * sampling Freq
     */
    int16 computedEcho;
    int16 bufferdata;

    static int16 echoHead = 0;
    static int16 echoTail = 0;
    static int16 echoPointer = 0;

    echoPointer = (echoTail - p) & ECHOBUFFERSIZE_MASK;

    bufferdata = echoBuffer[echoPointer];

    computedEcho = (float32)((1-echoRatio)*sampleIn + echoRatio*bufferdata);

    echoBuffer[echoHead] = computedEcho;          //send to buffer

    echoHead = ((echoHead + 1) & ECHOBUFFERSIZE_MASK);       //increment head head= head + 2 & BUFFERSIZE(0x7FFFF)
    echoTail = (echoTail + 1) & ECHOBUFFERSIZE_MASK;         // increment tail pointer

    return computedEcho;

}
