/*
 * PitchShift.c
 *
 *  Created on: Feb 3, 2022
 *      Author: Evan Rives 08:57 AM
 *
 *      A Time-domain pitch shifting implementation
 *
 *      Utilizing a buffer, we write to the buffer at a constant rate
 *      and using fractionals we are able to read from the buffer at a different rate
 *      producing a shift in the pitch without affecting length of audio
 *
 *      When the one pointer gets closer to overlapping the other pointer, we crossfade too prevent glitches in audio
 *
 *
 */
#include "PitchShift.h"

#define WRAP_SHIFT 4
#define OVERLAP 80 //how much overlap before we crossfade and switch read pointer to reduce clicks
#define PITCHBUFFERSIZE 4096
#define PITCHBUFFERSIZE_MASK 4095

static int PitchBuffer[PITCHBUFFERSIZE]; //buffer to hold audio samples
static float PitchStep = 1.0; //This will range between 0.1 and 2.0


void updatePitch(float step)
{
    if(step > 8.0)
        PitchStep = 8.0;
    else if(step < 0.5)
        PitchStep = 0.5;
    else
        PitchStep = step;

}

float processPitchShift(float sampleIn)
{
    /*
     * Write is always updated at a constant rate
     * read utilizes float32 because we are going to typecast to Uint16 when accessing buffer
     * This will truncate the fractional and use the integer portion, but will update index when
     * fractional adds new integer
     * ie: PitchBuffer[(Uint16)42.6] == PitchBuffer[42] => add 0.6 for next iteration
     *     PitchBuffer[(Uint16)43.2] == PitchBuffer[43] => add 0.6 for next iteration
     *     PitchBuffer[(Uint16)43.8] == PitchBuffer[43] => etc.
     * */
    int sampleInt = 10000*sampleIn;
    static int PitchWrite = 0;
    static float PitchRead = 0;
    static float PitchRead2 = PITCHBUFFERSIZE/2;
    static unsigned int index = 0;
    static unsigned int index2 = 0;
    static unsigned int diff = 0;
    int sampleOut, sample1, sample2;
    int result;
    static unsigned int diff_comp;
    float w, crossfade;
    //w = 0.90f - 0.10f * (1-cosf(PI2*PitchWrite / ((PITCHBUFFERSIZE - 1)))); //a hanning window to reduce clicks

    //write to buffer
    PitchBuffer[PitchWrite] = sampleInt;

    index = (unsigned int)PitchRead;
    index = index & PITCHBUFFERSIZE_MASK; //Masking for wrapping
    index2 = (unsigned int)PitchRead2;
    index2 = index2 & PITCHBUFFERSIZE_MASK; //Masking for wrapping

    //add our fractional step to the PitchRead pointer
    PitchRead += PitchStep;
    PitchRead2 += PitchStep;
    if(PitchRead > 4096.0)
        PitchRead = 0.0;
    if(PitchRead2 > 4096.0)
        PitchRead2 = 0.0;

    // Grab a sample
    sample1 = PitchBuffer[index];
    sample2 = PitchBuffer[index2];


    //the difference between read1 and write pointer, determines if we crossfade
    diff = ( PitchWrite - index);
    if(diff <= OVERLAP && diff >= 0)
    {
        //crossfade
        crossfade = (float)((PitchWrite - index)/OVERLAP);
    }
    else if(PitchWrite - index == 0)
        crossfade = 0.0f;

    //the difference between read2 and write pointer, determines if we crossfade
    diff = (PitchWrite - index2);
    if(diff <= OVERLAP && diff >= 0)
    {
        //crossfade
        crossfade = 1.0f - (float)((PitchWrite - index2)/OVERLAP);
    }
    else if(PitchWrite - index2 == 0)
        crossfade = 1.0f;

    //sum up crossfade
    sampleOut = (float)(sample1*crossfade) + (float)(sample2*(1-crossfade));

    // Increment write position : check if position has gotten bigger than buffer size
    PitchWrite = (PitchWrite + 1) & PITCHBUFFERSIZE_MASK;

    return (float)sampleOut/10000;


}

