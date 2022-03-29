/*
 * PitchShift.h
 *
 *  Created on: Feb 3, 2022
 *      Author: Evan Rives
 */

#ifndef AUDIO_FX_PITCHSHIFT_H_
#define AUDIO_FX_PITCHSHIFT_H_

#include <F28x_Project.h>
#include <math.h>

#define PI2 6.283185307179

void updatePitch(float32 step);

int16 processPitchShift(int16 sampleIn);

#endif /* AUDIO_FX_PITCHSHIFT_H_ */
