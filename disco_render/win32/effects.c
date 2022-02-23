#include "effects.h"

void updateEffects(Effects* fx){
    if(fx->effectType == 0){ // no effects
        *fx->output = fx->input;
    }
    else if(fx->effectType == 1){ // ECHO
        updateEchoParams(0.085*fx->param1, fx->param2);
        *fx->output = processEcho(fx->input);
    }
}