/*
    synth.c
    created on: Feb 15, 2022
    Author: Luke Jones
*/
#include "synth.h"
#include <math.h>
#include <stdio.h>
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
    *keys->frequency = keys->freq_table[keyIndex]*pow(1.5,keys->lfo_input);
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
    else if(osc-> oscType == 2){ // WAV FILE
        //update pitch shift factor
        updatePitch(osc->frequency/osc->wavFrequency);
        //printf("%f %f\n", osc->frequency,osc->wavFrequency);
        //updatePitch(1);
        //process pitch shift, write to output
        *osc->output = processPitchShift(osc->wavInput);
    }
    else if(osc-> oscType == 3){ // OSCILLATOR 4 (frequency modulation)
        //calculate modulator frequency
        float modFreq = osc->frequency * pow(8, osc->param1 + osc->lfo_input);
        osc->phase2 += modFreq / SAMPLE_RATE;
        if(osc->phase2 > 1) osc->phase2 -=1;
        //calculate modulation
        float freqModulation = sinf(2.0f * PI * osc->phase2) * osc->param2;
        //calculate modulated frequency
        float frequency = osc->frequency * pow(2, freqModulation);
        //update oscillator phase
        osc->phase += frequency / SAMPLE_RATE;
        if(osc->phase > 1) osc->phase -= 1;
        //write to output 
        *osc->output = sinf(2.0f * PI * osc->phase);
    }
    else if(osc-> oscType == 4){ // OSCILLATOR 5 (ring modulation)
        //calculate ring modulation frequency
        float modFreq = osc->frequency * pow(2, 2*osc->param1 + osc->lfo_input - 1);
        //calculate difference of frequencies
        float diffFreq = osc->frequency - modFreq;
        if(diffFreq < 0) diffFreq *= -1;
        float sumFreq = osc->frequency + modFreq;
        osc->phase += diffFreq / SAMPLE_RATE;
        if(osc->phase > 1) osc->phase -= 1;
        osc->phase2 += sumFreq / SAMPLE_RATE;
        if(osc->phase2 > 1) osc->phase2 -= 1;
        *osc->output = sinf(2.0f * PI * osc->phase) * sinf(2.0f * PI * osc->phase2);
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
    *env->output = env->amplitude * env->input * (env->lfo_input + 1);
}
void updateFilter(Filter* filter){
    float input;
    for(int i = 0; i < 4; i++){
        input += filter->input[i];
    }
    input /= 4;
    if(filter->updateFlag == false)
    *filter->output = processBiquads(filter->EQ, input);
    else
    *filter->output = input;
}
void initSynth(Synth* synth){
    synth->filter.EQ = initializeBiquads();
    //keyboard init
    //fill frequency table
    for(int i = 0; i < 4; i++){
        float tempFreq = 261.6/4;
        for(int j = 0; j < 85; j++){
            synth->keys[i].freq_table[j] = tempFreq;
            tempFreq *= 1.059463;
        }
        //midi keys is false for test program
        synth->keys[i].midiKeys = false;
        //connect keyboard to oscillator and envelope
        synth->keys[i].frequency = &synth->osc[i].frequency;
        //connect oscillator to envelope
        synth->osc[i].output = &synth->env[i].input;
        //connect envelope to filter
        synth->env[i].output = &synth->filter.input[i];
    }
    //connect filter to effects input
    synth->filter.output = &synth->fx.input;
    //connect effects unit to synth output
    synth->fx.output = &synth->output;
    //configure lfo targets
    synth->lfo.targets[0] = &synth->keys[0].lfo_input;
    synth->lfo.targets[1] = &synth->osc[0].lfo_input;
    synth->lfo.targets[2] = &synth->env[0].lfo_input;
    //init lfo target
    synth->lfo.target = 0;
    synth->lfo.output = synth->lfo.targets[0];
    //hard code effects parameters
}
float updateSynth(Synth* synth){
    updateLFO(&synth->lfo);
    for(int i = 0; i < 4; i++){
        //copy lfo connections
        if(i != 0){
            synth->osc[i].lfo_input = synth->osc[0].lfo_input;
            synth->env[i].lfo_input = synth->env[0].lfo_input;
            synth->keys[i].lfo_input = synth->keys[0].lfo_input;
        }
        updateKeyboard(&synth->keys[i]);
        updateOscillator(&synth->osc[i]);
        updateEnvelope(&synth->env[i]);
    }
    updateFilter(&synth->filter);
    updateEffects(&synth->fx);
    return synth->output;
}