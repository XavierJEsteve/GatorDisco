/*
 * BiquadEQ.h
 *
 *  Created on: Jan 18, 2022
 *      Author: rives
 */

#ifndef BIQUADEQ_H_
#define BIQUADEQ_H_

#include <math.h>
#include <stdlib.h>

#define PI2 6.283185307179

typedef struct
{
    float dbGain; //keep between +/- 15dB
    float Q;    //Q controls bandwidth
    float fCenter; //vary center frequency
    float low;    //holds the Lower limit of the freq band in Hz
    float high;   //holds the upper limit of the freq band in Hz

    float alpha;
    float A;    //this will control the gain
    float w0;
    float cosParam; //cos(w0)
    float sinParam; //sin(w0)

    int Fs;

    //coefficients
    float b0,b1,b2,a0,a1,a2;

    //delays
    float z1,z2;


}Biquad;


Biquad* initializeBiquads();
void resetEQ(Biquad* Biquad);
void updateParameters(Biquad* Biquad, float dbGain, float fCenter, float Q); //will send a pointer
void calculateCoefficients(Biquad* Biquad);
float processBiquads(Biquad* EQ, float sampleIn);

/*
 ranges of the different freq bands
 22 <- 32 -> 44
 44 <- 64 -> 88
 88 <-  125 -> 177
 177 <- 250 -> 355
 355 <- 500 -> 710
 710 <- 1000 -> 1420
 1420 <- 2000 -> 2840
 2840 <- 4000 -> 5680
 5680 <- 8000 -> 11360
 11360 <- 16000 -> 22720

*/

#endif /* BIQUADEQ_H_ */
