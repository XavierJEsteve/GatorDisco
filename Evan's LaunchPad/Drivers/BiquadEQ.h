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
    float32 low;    //holds the Lower limit of the freq band in Hz
    float32 high;   //holds the upper limit of the freq band in Hz

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
void resetEQ(Biquad* Biquad);
void updateParameters(Biquad* Biquad, float32 dbGain, float32 fCenter, float32 Q); //will send a pointer
void calculateCoefficients(Biquad* Biquad);
int16 processBiquads(Biquad* EQ, int16 sampleIn);

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
