/*
    Possible Effects: Chorus, Flanger, Phaser
*/

#include "Effect4.h"
#include <stdio.h>
#include <math.h>

#define SAMPLE_RATE 48000
#define PI 3.14159265358979323846f
#define MIN_FREQ 0.2
#define MAX_FREQ 5
#define BUFFER_SIZE 4096

static float inputBuffer[BUFFER_SIZE];
static float outputBuffer[BUFFER_SIZE];

static float phase = 0;
static float frequency = 0;
static float mix = 0;

static int bufferPointer = 0;
static int printCounter = 0;

void updateEffect4Params(float freq, float delayMix)
{
    mix = delayMix * 0.5;
    frequency = MIN_FREQ + (MAX_FREQ - MIN_FREQ) * freq;
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
    printCounter++;
    //determine delay value
    //printf("determine delay value\n");
    phase += frequency / SAMPLE_RATE;
    if(phase > 1) phase -= 1;
    float sin_value = sinf(phase * 2 * PI);
    //original delay = SAMPLE_RATE / w0 (25 hz, 50 hz, 100 hz, etc.)
    int w0 = 50;
    int delay = SAMPLE_RATE/w0 + sin_value*10;
    
    //store current input
    //printf("determine delay value\n");
    inputBuffer[bufferPointer] = sampleIn;

    //increment pointer
    bufferPointer++;
    bufferPointer %= BUFFER_SIZE;

    //obtain previous input
    int prevPointer = bufferPointer - delay;
    if(prevPointer < 0) prevPointer += BUFFER_SIZE;
    float prevInput = inputBuffer[prevPointer];

    //obtain previous output
    float prevOutput = outputBuffer[prevPointer];

    // y(k)=dx(k)−x(k−1)+dy(k−1) ---- for each stage, d = 0.5, substitute -1 for calculated delay value

    float output = mix*sampleIn - prevInput + mix*prevOutput;
    
    if(output > 1 || output < -1){
        printf("output: %f\nprevOutput: %f\nprevInput: %f\nmix: %f\n",output,prevOutput,prevInput,mix);
    }
    
    outputBuffer[bufferPointer] = output;
    return output;
}