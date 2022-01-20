/*
 * BiquadEQ.h
 *
 *  Created on: Jan 18, 2022
 *      Author: rives
 */

#ifndef BIQUADEQ_H_
#define BIQUADEQ_H_

#include <F28x_Project.h>
#include <math.h>
#include <stdlib.h>

#define PI2 6.283185307179

typedef struct
{
    float32 dbGain; //keep between +/- 15dB
    float32 Q;    //Q controls bandwidth
    float32 fCenter; //vary center frequency

    float32 alpha;
    float32 A;    //this will control the gain
    float32 w0;
    float32 cosParam; //cos(w0)
    float32 sinParam; //sin(w0)

    Uint16 Fs;

    //coefficients
    float32 b0,b1,b2,a0,a1,a2;

    //delays
    float32 z1,z2;


}Biquad;


Biquad* initializeBiquads();
void updateParameters(Biquad* Biquad, float32 dbGain, float32 fCenter, float32 Q); //will send a pointer
void calculateCoefficients(Biquad* Biquad);
int16 processBiquads(Biquad* EQ, int16 sampleIn);

/*
 ranges of the different freq bands
 250 <- 300-> 400
 400 <- 500 -> 750
 750 <- 1000 -> 1500
 1500 <- 2000 -> 3000
 3000 <- 4000 -> 6000
 6000 <- 8000 -> 12k
 12k <- 16k -> 20k
*/

#endif /* BIQUADEQ_H_ */
