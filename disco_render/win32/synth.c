/*
    synth.c
    created on: Feb 15, 2022
    Author: Luke Jones
*/
#include "synth.h"
#include <math.h>
#define PI 3.14159

void updateKeyboard(Keyboard* keys){
    int keyIndex = keys->key;
    if(!keys->midiKeys){
        int octaveFactor = 0;
        if(keys->octave > 0.75){
            octaveFactor = 3;
        }
        else if(keys->octave > 0.5){
            octaveFactor = 2;
        }
        else if(keys->octave > 0.25){
            octaveFactor = 1;
        }
        keyIndex += octaveFactor*12;
    }
    //printf("table freq: %f\n",keys->freq_table[keyIndex]);
    *keys->frequency = keys->freq_table[keyIndex]*pow(2,keys->lfo_input);
    //printf("real freq: %f\n", *keys->frequency);
}
void updateOscillator(Oscillator* osc){
    //update phase
    if(osc->oscType == 0){ //pulse wave
        osc->phase += osc->frequency / SAMPLE_RATE;
        if(osc->phase > 1) osc->phase -= 1;
        float sin_value = sinf(2.0f * PI * osc->phase);
        //determine threshold
        float threshold = osc->param1;
        threshold = 0.5*(threshold + osc->lfo_input);
        if(sin_value > threshold){
            *osc->output = 0.5;
        }
        else {
            *osc->output = -0.5;
        }
    }
    else if(osc->oscType == 1){ //sawtooth
        //two seperate oscillators
        float frequency1 = osc->frequency * pow(1.01, osc->param1 + osc->lfo_input);
        float frequency2 = osc->frequency * pow(1.01, -1*(osc->param1 + osc->lfo_input));
        osc->phase += frequency1 / SAMPLE_RATE;
        if(osc->phase > 1) osc->phase -= 1;
        osc->phase2 += frequency2 / SAMPLE_RATE;
        if(osc->phase2 > 1) osc->phase2 -= 1;
        *osc->output = osc->phase + osc->phase2 - 1;
    }
}
void updateLFO(LFO* lfo){
    //update phase
    lfo->phase += MAX_LFO_SPEED * lfo->speed / SAMPLE_RATE;
    if(lfo->phase > 1) lfo->phase -= 1;
    *lfo->output = 0;
    lfo->output = lfo->targets[lfo->target];
    *lfo->output = sinf(2.0f * PI * lfo->phase) * lfo->val;
}
void updateEnvelope(Envelope* env){
    if(env->gate){
        if(!env->decayPhase){
            env->amplitude += 1.0/(SAMPLE_RATE*MAX_ATTACK_TIME*(env->attack+0.001));
            if(env->amplitude > 0.99){
                env->amplitude = 1;
                env->decayPhase = true;
            }
        }
        else{
            if(env->amplitude > env->sustain){
                env->amplitude -= (1.0-env->sustain)/(SAMPLE_RATE*MAX_ATTACK_TIME*(env->decay+0.001));
            }
        }
    }
    else{
        env->decayPhase = false;
        if(env->amplitude > 0){
            env->amplitude -= 1.0/(SAMPLE_RATE*MAX_ATTACK_TIME*(env->release+0.001));
        }
    }
    *env->output = env->amplitude * env->oscOutput * (env->lfo_input + 1);
}
void initSynth(Synth* synth){
    //keyboard init
    //fill frequency table
    float tempFreq = 261.6/4;
    for(int i = 0; i < 85; i++){
        synth->keys.freq_table[i] = tempFreq;
        tempFreq *= 1.059463;
    }
    //midi keys is false for test program
    synth->keys.midiKeys = false;
    //connect keyboard to oscillator and envelope
    synth->keys.frequency = &synth->osc.frequency;
    //connect oscillator to envelope
    synth->osc.output = &synth->env.oscOutput;
    //connect envelope to effects input
    synth->env.output = &synth->fx.input;
    //connect effects unit to synth output
    synth->fx.output = &synth->output;
    //configure lfo targets
    synth->lfo.targets[0] = &synth->keys.lfo_input;
    synth->lfo.targets[1] = &synth->osc.lfo_input;
    synth->lfo.targets[2] = &synth->env.lfo_input;
    //init lfo target
    synth->lfo.target = 0;
    synth->lfo.output = synth->lfo.targets[0];
    //hard code effects parameters
}
float updateSynth(Synth* synth){
    updateLFO(&synth->lfo);
    updateKeyboard(&synth->keys);
    updateOscillator(&synth->osc);
    updateEnvelope(&synth->env);
    updateEffects(&synth->fx);
    return synth->output;
}