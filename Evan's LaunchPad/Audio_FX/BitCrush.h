/*
 * BitCrush.h
 *
 *  Created on: Jan 27, 2022 08:51AM
 *      Author: Evan rives
 */

#ifndef BITCRUSH_H_
#define BITCRUSH_H_

#include <F28x_Project.h>

int16 ProcessBitCrush(int16 sampleIn);

int16 ProcessSampleRateReduction(int16 sampleIn);

void updateBitDepth(Uint16 value);

void updateSampleRate(Uint16 value);

#endif /* BITCRUSH_H_ */
