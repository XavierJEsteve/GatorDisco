#ifndef EFFECTS_H
#define EFFECTS_H

#include "Effects/Echo.h"
#include "Effects/BitCrush.h"
#define NUM_EFFECTS 4
typedef struct{
    //inputs
    int effectType;
    float input;
    /*
    0:  off
    1:  ECHO
    2:  Bit Crush
    3:  Sample Rate Reduction
    */
    float param1;
    float param2;
    /*
    ECHO:       echo time and echo volume
    Bit Crush:  bit crush factor
    SRR:        Sample Rate Reduction factor
    */
    //internal variables

    //outputs
    float* output;
} Effects;
void updateEffects(Effects* fx);

#endif