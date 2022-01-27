/*
 * BitCrush.c
 *
 *  Created on: Jan 27, 2022 08:51AM
 *      Author: rives
 */

#include "BitCrush.h"
#include <math.h>

static Uint16 bitDepth = 16;
static int16 sampleRatio = 1;
static int16 sampleStep = 0;




int16 ProcessBitCrush(int16 sampleIn)
{
    //we shift right by (16-bitDepth)
    //we then shift back left to maintain original magnitude

    int sampleOut;
    Uint16 n = 16-bitDepth;


    sampleOut = (sampleIn >> n);
    sampleOut <<= n;

    return sampleOut;
}


void updateBitDepth(Uint16 value)
{
    if(value < 1)
        bitDepth = 1;
    else if(value > 16)
        bitDepth = 16;
    else
        bitDepth = value;
}

void updateSampleRate(Uint16 value)
{
    //value of 1 - 16
    sampleRatio = value;
}

int16 ProcessSampleRateReduction(int16 sampleIn)
{
    static int32 accumulator = 0;
    static int16 buffer = 0;
    int16 sampleOut = 0;

    if(sampleStep == 0) //updates output every Nth samples
    {
        // Save current sample
        accumulator += sampleIn;
        // Calculate the average of prev N samples to output
        buffer = accumulator / (int32)sampleRatio;
        sampleOut = buffer;
        accumulator = 0;
    }
    else
    {
        // Use previously saved sample
        accumulator += sampleIn;
        sampleOut = buffer;

    }

    if(++sampleStep >= sampleRatio)
        sampleStep = 0;

    return sampleOut;

}
