#include "effects.h"

void updateEffects(Effects* fx){
    if(fx->effectType == 0){ // no effects
        *fx->output = fx->input;
    }
    else if(fx->effectType == 1){ // ECHO
        updateEchoParams(0.085*fx->param1, fx->param2);
        *fx->output = processEcho(fx->input);
    }
    else if(fx->effectType == 2){ // BIT CRUSH
        updateBitDepth(fx->param1);
        *fx->output = ProcessBitCrush(fx->input);
    }
    else if(fx->effectType == 3){ //SAMPLE RATE REDUCTION
        updateSampleRate(fx->param1);
        *fx->output = ProcessSampleRateReduction(fx->input);
    }
}