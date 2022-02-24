/*
    Possible Effects: Chorus, Flanger, Phaser
*/

#include "Effect4.h"

static float effect4param1;
static float effect4param2;


void updateEffect4Params(float param1, float param2)
{
    effect4param1 = param1;
    effect4param2 = param2;
}

float processEffect4(float sampleIn)
{
    return sampleIn;
}