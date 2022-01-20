#include "AIC23.h"
#include "SPIB.h"
#include <F2837xD_device.h>
#include <F28x_Project.h>
#include <math.h>
#include "BiquadEQ.h"

interrupt void Mcbspb_RxINTB_ISR(void);
interrupt void Spi_RxINTB_ISR(void);

#define SAMPLE_RATE 48000
#define STREAM_BUFFER_SIZE 1
#define MAX_ATTACK_TIME 3
#define MAX_DECAY_TIME 5
#define MAX_VALUE 1000;
#define PI 3.14159
int updateSignalCounter = 0;
int bufferSwitchCounter = 0;
float* params[10];
int paramSelection = 0;
int keySelection = -1;
float frequencies[17];
typedef struct{
    float phase;
    float phaseStride;
    float frequency;
    float threshold;
    float octave;
    float octaveFactor;
    float PWM_phase;
    float PWM_phaseStride;
    float PWM_frequency;
    float PWM_val;
} Oscillator;
typedef struct{
    float attack;
    float decay;
    float sustain;
    float release;
    float amplitude;
    bool decayPhase;
} ADSR_Control;
typedef struct{
    bool black;
    float frequency;
    bool pressed;
    int xPos;
} Key;
typedef struct{
    bool keyPressed;
    int x;
    int y;
} Input;
typedef struct{
    float32 highPass;
    float32 cutoff;
    Biquad* biquads;
} Filter;

Oscillator osc;
ADSR_Control adsr;
Input masterInput;
Filter filter;

bool ping = false;
int16 ping_buffer[STREAM_BUFFER_SIZE];
int16 pong_buffer[STREAM_BUFFER_SIZE];
int samplePointer = 0;

float sample_duration;
float calculateAmp(){
    if(masterInput.keyPressed){
        if(!adsr.decayPhase){
            adsr.amplitude += 1.0/(SAMPLE_RATE*MAX_ATTACK_TIME*(adsr.attack+0.001));
            if(adsr.amplitude > 0.99){
                adsr.amplitude = 1;
                adsr.decayPhase = true;
            }
        }
        else{
            if(adsr.amplitude > adsr.sustain){
                adsr.amplitude -= (1.0-adsr.sustain)/(SAMPLE_RATE*MAX_ATTACK_TIME*(adsr.decay+0.001));
            }
        }
    }
    else{
        adsr.decayPhase = false;
        if(adsr.amplitude > 0){
            adsr.amplitude -= 1.0/(SAMPLE_RATE*MAX_ATTACK_TIME*(adsr.release+0.001));
        }
    }
    return adsr.amplitude;
}
void updateSignal(int16* signal, float sample_duration){

    if(osc.octave < 0.25){
        osc.octaveFactor = 0.25;
    }
    else if(osc.octave < 0.5){
        osc.octaveFactor = 0.5;
    }
    else if(osc.octave < 0.75){
        osc.octaveFactor = 1;
    }
    else{
        osc.octaveFactor = 2;
    }
    osc.PWM_phaseStride = osc.PWM_frequency*20 * sample_duration;
    osc.phaseStride = osc.frequency * sample_duration * osc.octaveFactor;
    for(int i = 0; i < STREAM_BUFFER_SIZE; i++){
        //pulse wave
        osc.phase += osc.phaseStride;
        if(osc.phase > 1) osc.phase -= 1;
        float sin_value = sinf(2.0f * PI * osc.phase);
        //PWM phase
        osc.PWM_phase += osc.PWM_phaseStride;
        if(osc.PWM_phase > 1) osc.PWM_phase -= 1;
        float PWM_sin_value = sinf(2.0f * PI * osc.PWM_phase)*osc.PWM_val;
        float threshold = 0.5*(osc.threshold + PWM_sin_value);

        if(sin_value > threshold){
            signal[i] = 0.5*MAX_VALUE;
        }
        else {
            signal[i] = -0.5*MAX_VALUE;
        }
        //FILTER
        signal[i] = processBiquads(filter.biquads, signal[i]);
        //ADSR
        signal[i] *= calculateAmp();
    }
    updateSignalCounter++;

}

void updateBiquads(){
    bool zeroOut = false;
    if(filter.highPass > 0.5){
        for(int i = 6; i >=0; i--){
            if(!zeroOut){
                if(filter.cutoff*20000 < filter.biquads[i].fCenter){
                    updateParameters(&filter.biquads[i],0,filter.biquads[i].fCenter,0.707);
                }
                else{
                    float32 fCenterLow = filter.biquads[i].fCenter;
                    float32 fCenterHigh;
                    if(i < 6) fCenterHigh = filter.biquads[i+1].fCenter;
                    else fCenterHigh = 20000;
                    float ratio = (fCenterHigh - filter.cutoff*20000)/(fCenterHigh - fCenterLow);
                    updateParameters(&filter.biquads[i],15*ratio-15,filter.biquads[i].fCenter,0.707);
                    zeroOut = true;
                }
            }
            else{
                updateParameters(&filter.biquads[i],-15.0,filter.biquads[i].fCenter,0.707);
            }
        }
    }
    else{
        for(int i = 0; i <7; i++){
            if(!zeroOut){
                if(filter.cutoff*20000 > filter.biquads[i].fCenter){
                    updateParameters(&filter.biquads[i],0,filter.biquads[i].fCenter,0.707);
                }
                else{
                    float32 fCenterHigh = filter.biquads[i].fCenter;
                    float32 fCenterLow;
                    if(i > 0) fCenterLow = filter.biquads[i-1].fCenter;
                    else fCenterLow = 0;
                    float ratio = (fCenterHigh - filter.cutoff*20000)/(fCenterHigh - fCenterLow);
                    updateParameters(&filter.biquads[i],-15*ratio,filter.biquads[i].fCenter,0.707);
                    zeroOut = true;
                }
            }
            else{
                updateParameters(&filter.biquads[i],-15.0,filter.biquads[i].fCenter,0.707);
            }
        }
    }
}
void getSliderParams(){
    osc.octave = 0.5;
    osc.threshold = 0.5;
    osc.PWM_frequency = 0.1;
    osc.PWM_val = 1;
    adsr.attack = 0.8;
    adsr.decay = 0;
    adsr.sustain = 1;
    adsr.release = 0;
}

void initOscADSR(){
    osc.frequency = 600;
    osc.threshold = 0;
    osc.octave = 0;
    osc.PWM_phase = 0;
    osc.PWM_phaseStride = 0;
    osc.PWM_frequency = 0;
    osc.PWM_val = 0;
    sample_duration = (1.0f)/SAMPLE_RATE;
    osc.phase = 0;
    osc.phaseStride = osc.frequency * sample_duration;
    osc.phase = 0;
    osc.phaseStride = osc.frequency * sample_duration;
    adsr.amplitude = 0;
    adsr.attack = 0;
    adsr.decay = 0;
    adsr.release = 0;
    adsr.sustain = 0;
    //configure pointers to parameters
    params[0] = &osc.octave;
    params[1] = &osc.threshold;
    params[2] = &osc.PWM_frequency;
    params[3] = &osc.PWM_val;
    params[4] = &adsr.attack;
    params[5] = &adsr.decay;
    params[6] = &adsr.sustain;
    params[7] = &adsr.release;
    params[8] = &filter.cutoff;
    params[9] = &filter.highPass;
}
void buildKeys(){
    float tempFreq = 261.6;
    for(int i = 0; i < 17; i++){
        frequencies[i] = tempFreq;
        tempFreq *= 1.059463;
    }
}
int main() {
    //LaunchPad init functions
    InitSysCtrl();
        EALLOW;
        InitMcBSPb_2();
        EALLOW;
        InitSPIA();
        InitAIC23_2();
        SPIB_Slave_Init();
        EALLOW;
        PieVectTable.MCBSPB_RX_INT = &Mcbspb_RxINTB_ISR;
        PieVectTable.SPIB_RX_INT = &Spi_RxINTB_ISR;
        DINT;
        PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
        PieCtrlRegs.PIEIER6.bit.INTx7 = 1;
        PieCtrlRegs.PIEIER6.bit.INTx3 = 1;
        EALLOW;
        IER |= 32;
        EINT;
        EALLOW;


    //synth init functions
    masterInput.keyPressed = true;
    initOscADSR();
    buildKeys();
    getSliderParams();
    //filter init
    filter.biquads = initializeBiquads();
    filter.cutoff = 1;
    updateSignal(ping_buffer, sample_duration);
    updateSignal(pong_buffer, sample_duration);
    filter.highPass = 0.7;
    filter.cutoff = 0.6;
    updateBiquads();
    while(1)
    {
        if(samplePointer == STREAM_BUFFER_SIZE){
            //UpdateAudioStream(synthStream, buffer, STREAM_BUFFER_SIZE);
            //processInput();
            if(ping){
                updateSignal(ping_buffer, sample_duration);
                ping = false;
            }
            else{
                updateSignal(pong_buffer, sample_duration);
                ping = true;
            }
            samplePointer = 0;
            bufferSwitchCounter++;
            if(bufferSwitchCounter == 100){
                updateSignalCounter -= bufferSwitchCounter;
            }
        }
    }
    return 0;
}

interrupt void Mcbspb_RxINTB_ISR(void)
{
    EALLOW;
    int16 LeftData = McbspbRegs.DRR2.all;
    int16 RightData = McbspbRegs.DRR1.all;
    //codecEvent = true;
    if(ping){
        McbspbRegs.DXR2.all = ping_buffer[samplePointer];
        McbspbRegs.DXR1.all = ping_buffer[samplePointer];
    }
    else{
        McbspbRegs.DXR2.all = pong_buffer[samplePointer];
        McbspbRegs.DXR1.all = pong_buffer[samplePointer];
    }
    samplePointer++;
    EALLOW;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
}
Uint16 spiData[100];
int spiDataPointer = 0;
interrupt void Spi_RxINTB_ISR(void){
    EALLOW;
    Uint16 data = SpibRegs.SPIRXBUF & 255;
        SpibRegs.SPITXBUF = data;
        spiData[spiDataPointer] = data;
        spiDataPointer++;
        spiDataPointer %= 100;
        if(data >> 7 == 1){
            paramSelection = data & 127;
        }
        else{
            if(paramSelection > 9){
                float val = (float)data / 128.0;
                *params[paramSelection - 2] = val;
                updateBiquads();
            }
            else if(paramSelection > 1){
                float val = (float)data / 128.0;
                *params[paramSelection - 2] = val;
            }
            else{
                if(paramSelection == 0){
                    if(data == 0){
                        masterInput.keyPressed = false;
                    }
                    else{
                        masterInput.keyPressed = true;
                    }
                }
                else if(paramSelection == 1){
                    if(data >= 0 && data < 17){
                        osc.frequency = frequencies[data];
                    }
                }
            }
        }
    SpibRegs.SPISTS.bit.INT_FLAG = 0;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
}
