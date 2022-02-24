/*
    synth.h
    created on: Feb 15, 2022
    Author: Luke Jones
*/
#ifndef SYNTH_H
#define SYNTH_H

#include <stdbool.h>
#include "effects.h"

#define MAX_ATTACK_TIME 3
#define MAX_DECAY_TIME 5
#define NUM_OSCILLATORS 4
#define NUM_LFO_TARGETS 3
#define SAMPLE_RATE 48000
#define MAX_LFO_SPEED 100

typedef struct{
    //inputs
    int key;
    float octave;
    float lfo_input;
    //internal variables
    float freq_table[85];
    bool midiKeys;
    //outputs
    float* frequency;
} Keyboard;
void updateKeyboard(Keyboard* keys);
typedef struct{
    //inputs
    int oscType;
    /*
    0:  PULSE WAVE
    1:  SAWTOOTH
    2:  FM
    */
    float frequency;
    float param1;
    float param2;
    /*
    PULSE WAVE: threshold
    SAWTOOTH:   detune
    */
    float lfo_input;
    //internal variables
    float phase;
    float phase2;
    //outputs
    float* output;
} Oscillator;
void updateOscillator(Oscillator* osc);
typedef struct{
    //inputs
    float speed; // 0 to 1
    float val;
    int target;
    //internal variables
    float phase;
    float* targets[5];
    //output
    float* output;
} LFO;
void updateLFO(LFO* lfo);
typedef struct{
    //inputs
    float attack;
    float decay;
    float sustain;
    float release;
    float oscOutput;
    bool gate;
    float lfo_input;
    //internal variables
    bool decayPhase;
    float amplitude;
    //outputs
    float* output;
} Envelope;
void updateEnvelope(Envelope* env);
typedef struct{
    float highPass;
    float cutoff;
} Filter;
typedef struct{
    Keyboard keys;
    Oscillator osc;
    LFO lfo;
    Envelope env;
    Effects fx;
    float output;
} Synth;

void initSynth(Synth* synth);
float updateSynth(Synth* synth);

#endif