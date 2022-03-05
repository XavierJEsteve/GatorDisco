/*
 * BitCrush.h
 *
 *  Created on: Jan 27, 2022 08:51AM
 *      Author: Evan rives
 */

#ifndef BITCRUSH_H_
#define BITCRUSH_H_

float ProcessBitCrush(float input);

float ProcessSampleRateReduction(float sampleIn);

void updateBitDepth(float value);

void updateSampleRate(float value);

#endif /* BITCRUSH_H_ */