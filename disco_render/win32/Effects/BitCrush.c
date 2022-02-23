/*
 * BitCrush.c
 *
 *  Created on: Jan 27, 2022 08:51AM
 *      Author: rives
 */

#include "BitCrush.h"
#include <math.h>

static unsigned int bitDepth = 16;
static int sampleRatio = 1;
static int sampleStep = 0;




float ProcessBitCrush(float input)
{
    //we shift right by (16-bitDepth)
    //we then shift back left to maintain original magnitude
    int sampleIn = (int)256*128*input;
    int sampleOut;
    int n = 16-bitDepth;


    sampleOut = (sampleIn >> n);
    sampleOut <<= n;

    return (float)sampleOut/(256*128);
}


void updateBitDepth(float input)
{
    int value = (int)(16*input) + 1;
    if(value < 1)
        bitDepth = 1;
    else if(value > 16)
        bitDepth = 16;
    else
        bitDepth = value;
}

void updateSampleRate(float input)
{
    //value of 1 - 16
    int value = (int)(16*input) + 1;
    if(value < 1)
        sampleRatio = 1;
    else if(value > 16)
        sampleRatio = 16;
    else
        sampleRatio = value;
}

float ProcessSampleRateReduction(float sampleIn)
{
    static float accumulator = 0;
    static float buffer = 0;
    float sampleOut = 0;

    if(sampleStep == 0) //updates output every Nth samples
    {
        // Save current sample
        accumulator += sampleIn;
        // Calculate the average of prev N samples to output
        buffer = accumulator / sampleRatio;
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