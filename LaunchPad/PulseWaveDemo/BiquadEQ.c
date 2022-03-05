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
    function: generates the 10 biquads with pre-generated parameters

    return value: returns a pointer to an array of biquads
*/


    static Biquad EQ[10];
    //set up the different center frequencies of each band
    EQ[0].fCenter = 32.0;
    EQ[1].fCenter = 64.0;
    EQ[2].fCenter = 125.0;
    EQ[3].fCenter = 250.0;
    EQ[4].fCenter = 500.0;
    EQ[5].fCenter = 1000.0;
    EQ[6].fCenter = 2000.0;
    EQ[7].fCenter = 4000.0;
    EQ[8].fCenter = 8000.0;
    EQ[9].fCenter = 16000.0;


    EQ[0].Fs = 48000;
    EQ[1].Fs = 48000;
    EQ[2].Fs = 48000;
    EQ[3].Fs = 48000;
    EQ[4].Fs = 48000;
    EQ[5].Fs = 48000;
    EQ[6].Fs = 48000;
    EQ[7].Fs = 48000;
    EQ[8].Fs = 48000;
    EQ[9].Fs = 48000;

    EQ[0].low = 22;
    EQ[1].low = 44;
    EQ[2].low = 88;
    EQ[3].low = 177;
    EQ[4].low = 355;
    EQ[5].low = 710;
    EQ[6].low = 1420;
    EQ[7].low = 2840;
    EQ[8].low = 5680;
    EQ[9].low = 11360;

    EQ[0].high = 44;
    EQ[1].high = 88;
    EQ[2].high = 177;
    EQ[3].high = 355;
    EQ[4].high = 710;
    EQ[5].high = 1420;
    EQ[6].high = 2840;
    EQ[7].high = 5680;
    EQ[8].high = 11360;
    EQ[9].high = 22720;

    //initialize default parameters and coefficients
    for(int i = 0; i < 10; i++)
    {
        updateParameters(&EQ[i], 0.0, EQ[i].fCenter, 1.414);
    }

    return EQ;
}

void updateParameters(Biquad* Biquad, float dbGain, float fCenter, float Q)
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
    Biquad->w0 = PI2 * (float)(fCenter/(Biquad->Fs));   //controls center frequency
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

float processBiquads(Biquad* EQ, float sampleIn)
{
    /*
      Inputs:
        Biquad** : an array of biquads which we will cascade out results through

      Output: sampleOut

      Description:
        utilizing Transposed Direct Form 2, the output of 1 biquad feeds into the input of the next

    */
    sampleIn *= 10000;
    float biquadInput = sampleIn;
    float biquadOutput = 0.0;

    for(int i = 0; i < 10; i++)
    {
        if(i == 8){
            biquadOutput = (EQ[i].b0)*biquadInput + EQ[i].z1;
            EQ[i].z1 = (EQ[i].b1)*biquadInput - (EQ[i].a1)*biquadOutput + EQ[i].z2;
            EQ[i].z2 = (EQ[i].b2)*biquadInput - (EQ[i].a2)*biquadOutput;
            biquadInput = biquadOutput;
        }
        else{
            biquadOutput = (EQ[i].b0)*biquadInput + EQ[i].z1;
            EQ[i].z1 = (EQ[i].b1)*biquadInput - (EQ[i].a1)*biquadOutput + EQ[i].z2;
            EQ[i].z2 = (EQ[i].b2)*biquadInput - (EQ[i].a2)*biquadOutput;
            biquadInput = biquadOutput;
        }

    }

    return biquadOutput / 10000;

}

void resetEQ(Biquad* EQ)
{
        EQ[0].fCenter = 32.0;
        EQ[1].fCenter = 64.0;
        EQ[2].fCenter = 125.0;
        EQ[3].fCenter = 250.0;
        EQ[4].fCenter = 500.0;
        EQ[5].fCenter = 1000.0;
        EQ[6].fCenter = 2000.0;
        EQ[7].fCenter = 4000.0;
        EQ[8].fCenter = 8000.0;
        EQ[9].fCenter = 16000.0;


        EQ[0].Fs = 48000;
        EQ[1].Fs = 48000;
        EQ[2].Fs = 48000;
        EQ[3].Fs = 48000;
        EQ[4].Fs = 48000;
        EQ[5].Fs = 48000;
        EQ[6].Fs = 48000;
        EQ[7].Fs = 48000;
        EQ[8].Fs = 48000;
        EQ[9].Fs = 48000;

        EQ[0].low = 22;
        EQ[1].low = 44;
        EQ[2].low = 88;
        EQ[3].low = 177;
        EQ[4].low = 355;
        EQ[5].low = 710;
        EQ[6].low = 1420;
        EQ[7].low = 2840;
        EQ[8].low = 5680;
        EQ[9].low = 11360;

        EQ[0].high = 44;
        EQ[1].high = 88;
        EQ[2].high = 177;
        EQ[3].high = 355;
        EQ[4].high = 710;
        EQ[5].high = 1420;
        EQ[6].high = 2840;
        EQ[7].high = 5680;
        EQ[8].high = 11360;
        EQ[9].high = 22720;

        //initialize default parameters and coefficients
        for(int i = 0; i < 10; i++)
        {
            updateParameters(&EQ[i], 0.0, EQ[i].fCenter, 1.414);
        }
}
