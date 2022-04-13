/*
 * PitchShift.h
 *
 *  Created on: Feb 3, 2022
 *      Author: Evan Rives
 */

#ifndef AUDIO_FX_PITCHSHIFT_H_
#define AUDIO_FX_PITCHSHIFT_H_

#include <math.h>

#define PI2 6.283185307179

void updatePitch(float step);

float processPitchShift(float sampleIn);

#endif /* AUDIO_FX_PITCHSHIFT_H_ */