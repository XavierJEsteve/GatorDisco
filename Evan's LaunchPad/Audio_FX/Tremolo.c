#include "Tremolo.h"

#pragma DATA_SECTION(PitchBuffer, "ramgs1");

#define TREMOLOBUFFERSIZE 4096
#define TREMOLOBUFFERSIZE_MASK 4095

static int16 TremoloBuffer[TREMOLOBUFFERSIZE]; //buffer to hold audio samples
static float32 TremoloRate = 0.5; //rate of tremolo between 0.0-1.0
static float32 TremoloDepth = 0.5; // depth of tremolo between 0.0-1.0
static float32 t = 0.0;     // current value of t for sine calculations



void updateTremolo(float32 time, float32 rate, float32 depth)
{
    if(time > 0.085)
        t  = (48000 * 0.085);
    else
       t  = (48000 * time);
    
    if(rate > 1.0)
        TremoloRate = 1.0;
    else if(rate < 0.0)
        TremoloRate = 0.0;
    else
        TremoloRate = rate;
        
    if(depth > 1.0)
        TremoloDepth = 1.0;
    else if(depth < 0.0)
        TremoloDepth = 0.0;
    else
        TremoloDepth = rate;
}

int16 processTremolo(int16 sampleIn)
{
    int16 bufferdata;

    static int16 tremoloHead = 0;
    static int16 tremoloTail = 0;
    static int16 tremoloPointer = 0;
    
    tremoloPointer = (tremoloTail - t) & TREMOLOBUFFERSIZE_MASK;

    bufferdata = TremoloBuffer[tremoloPointer];

    tremFactor = (float32)(1.0 - (TremoloDepth * (0.5 * sinf(t) + 0.5)));

    t += (TremoloRate * 0.085);
    
    if (t > 6.28318531)
        t -= 6.28318531;

    TremoloBuffer[tremoloHead] = tremFactor;          //send to buffer

    tremoloHead = ((tremoloHead + 1) & TREMOLOBUFFERSIZE_MASK);       //increment head head= head + 2 & BUFFERSIZE(0x7FFFF)
    tremoloTail = (tremoloTail + 1) & TREMOLOBUFFERSIZE_MASK;         // increment tail pointer

    return tremFactor;
}