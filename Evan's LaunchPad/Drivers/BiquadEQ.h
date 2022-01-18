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

#define PI2 6.283185307179

typedef struct
{
    float32 dbGain = 0.0; //keep between +/- 15dB
    float32 Q = 0.707;    //Q controls bandwidth
    float32 fCenter = 0.0; //vary center frequency

    float32 alpha = 0.0;
    float32 A = 0.0;    //this will control the gain
    float32 w0 = 0.0;
    float32 cosParam = 0.0; //cos(w0)
    float32 sinParam = 0.0; //sin(w0)

    Uint16 Fs = 48000;

    //coefficients
    float32 b0,b1,b2,a0,a1,a2 = 0.0;

    //delays
    float32 z1,z2 = 0.0;


}Biquad;


Biquad** initializeBiquads();
void updateParameters(Biquad* Biquad, float32 dbGain, float32 fCenter, float32 Q); //will send a pointer
void calculateCoefficients(Biquad* Biquad);
int16 processBiquads(Biquad** EQ);

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
