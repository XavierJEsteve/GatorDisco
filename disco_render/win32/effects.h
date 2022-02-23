#ifndef EFFECTS_H
#define EFFECTS_H

#include "Effects/Echo.h"

typedef struct{
    //inputs
    int effectType;
    float input;
    /*
    0:  off
    1:  ECHO
    */
    float param1;
    float param2;
    /*
    ECHO: echo time and echo volume
    */
    //internal variables

    //outputs
    float* output;
} Effects;
void updateEffects(Effects* fx);

#endif