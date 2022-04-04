/*
    Possible Effects: Chorus, Flanger, Phaser
*/

#include "Effect5.h"

static float effect5param1;
static float effect5param2;


void updateEffect5Params(float param1, float param2)
{
    effect5param1 = param1;
    effect5param2 = param2;
}

float processEffect5(float sampleIn)
{
    return sampleIn;
}