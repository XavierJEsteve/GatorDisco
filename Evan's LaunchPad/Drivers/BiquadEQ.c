/*
 * BiquadEQ.c
 *
 *  Created on: Jan 18, 2022
 *      Author: rives
 */

#include "BiquadEQ.h"

Biquad* initializeBiquads()
{

/*
    function: generates the 7 biquads with pre-generated parameters

    return value: returns a pointer to an array of biquads
*/


    static Biquad EQ[7];
    //set up the different center frequencies of each band
    EQ[0].fCenter = 300.0;
    EQ[1].fCenter = 500.0;
    EQ[2].fCenter = 1000.0;
    EQ[3].fCenter = 2000.0;
    EQ[4].fCenter = 4000.0;
    EQ[5].fCenter = 8000.0;
    EQ[6].fCenter = 16000.0;


    EQ[0].Fs = 48000;
    EQ[1].Fs = 48000;
    EQ[2].Fs = 48000;
    EQ[3].Fs = 48000;
    EQ[4].Fs = 48000;
    EQ[5].Fs = 48000;
    EQ[6].Fs = 48000;

    //initialize default parameters and coefficients
    for(int i = 0; i < 7; i++)
    {
        updateParameters(&EQ[i], 0.0, EQ[i].fCenter, 0.707);
    }

    return EQ;
}

void updateParameters(Biquad* Biquad, float32 dbGain, float32 fCenter, float32 Q)
{
    /*
      Inputs:
        Biquad* : a pointer to a biquad which will have its parameters updated
        dbGain  : a value between -15.0 to + 15 dB. Controls whether we boost or cut frequencies
        fCenter : a value between certain ranges that is predefined and is handled on the R.PI
        Q       : a value between 0.1 to 10. Controls the bandwidth of the biquad

      Output: Biquad with updated parameters
    */

    Biquad->dbGain = dbGain;
    Biquad->fCenter = fCenter;
    Biquad->Q = Q;

    //re-calculate intermediate variables for filter coefficients
    Biquad->A = powf(10, dbGain/40.0);           //controls gain
    Biquad->w0 = PI2 * (float32)(fCenter/(Biquad->Fs));   //controls center frequency
    Biquad->cosParam = cosf(Biquad->w0);
    Biquad->sinParam = sinf(Biquad->w0);
    Biquad->alpha = (Biquad->sinParam/(2*Q));           //controls Q

    //update the coefficients for the given biquad/freqband
    calculateCoefficients(Biquad);
}

void calculateCoefficients(Biquad* Biquad)
{

    /*
      Inputs:
        Biquad* : a pointer to a biquad which will have its coefficients updated

      Output: Biquad with updated coefficients

      Description:
        b0 = 1 + a*A
        b1 = -2cos(w0)
        b2 = 1 - a*A
        a0 = 1+(alpha/A)
        a1 same as b1, -2cos(w0)
        a2 = 1-(alpha/A)

        IMPORTANT**: Function normalizes by dividing every coefficient by a0
    */

    Biquad->b0 = 1 + Biquad->alpha*Biquad->A;       //b0 = 1 + a*A
    Biquad->b1 = -2*Biquad->cosParam;               //b1 = -2cos(w0)
    Biquad->b2 = 1 - Biquad->alpha*Biquad->A;       //b2 = 1 - a*A
    Biquad->a0 = 1 + (Biquad->alpha/Biquad->A);     //a0 = 1+(alpha/A)
    Biquad->a1 = Biquad->b1;                        //a1 same as b1, -2cos(w0)
    Biquad->a2 = 1 - (Biquad->alpha/Biquad->A);     //a2 = 1-(alpha/A)

    //Normalizing
    Biquad->b0 = (Biquad->b0)/(Biquad->a0);
    Biquad->b1 = (Biquad->b1)/(Biquad->a0);
    Biquad->b2 = (Biquad->b2)/(Biquad->a0);
    Biquad->a1 = (Biquad->a1)/(Biquad->a0);
    Biquad->a2 = (Biquad->a2)/(Biquad->a0);

}

int16 processBiquads(Biquad* EQ, int16 sampleIn)
{
    /*
      Inputs:
        Biquad** : an array of biquads which we will cascade out results through

      Output: sampleOut

      Description:
        utilizing Transposed Direct Form 2, the output of 1 biquad feeds into the input of the next

    */
    float32 biquadInput = (float32)sampleIn;
    float32 biquadOutput = 0.0;

    for(int i = 0; i < 7; i++)
    {
        biquadOutput = (EQ[i].b0)*biquadInput + EQ[i].z1;
        EQ[i].z1 = (EQ[i].b1)*biquadInput - (EQ[i].a1)*biquadOutput + EQ[i].z2;
        EQ[i].z2 = (EQ[i].b2)*biquadInput - (EQ[i].a2)*biquadOutput;

        biquadInput = biquadOutput;

    }

    return (int16)biquadOutput;

}
