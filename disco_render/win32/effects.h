#ifndef EFFECTS_H
#define EFFECTS_H

#include "Effects/Echo.h"
#include "Effects/BitCrush.h"
#include "Effects/PitchShift.h"
#include "Effects/Effect5.h"

#define NUM_EFFECTS 6
typedef struct{
    //inputs
    int effectType;
    float input;
    /*
    0:  off
    1:  ECHO
    2:  Bit Crush
    3:  Sample Rate Reduction
    4:  Effect 4
    5:  Effect 5
    */
    float param1;
    float param2;
    /*
    ECHO:       echo time and echo volume
    Bit Crush:  bit crush factor
    SRR:        Sample Rate Reduction factor
    Effect4:    ???
    Effect5:    ???
    */
    //internal variables

    //outputs
    float* output;
} Effects;
void updateEffects(Effects* fx);

#endif