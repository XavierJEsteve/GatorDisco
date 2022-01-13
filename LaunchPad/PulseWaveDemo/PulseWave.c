#include "AIC23.h"
#include <F2837xD_device.h>
#include <F28x_Project.h>
#include <SRAMDriver.h>
#include <OneToOneI2CDriver.h>
#include <math.h>

interrupt void Mcbspb_RxINTB_ISR(void);

#define SAMPLE_RATE 32000
#define STREAM_BUFFER_SIZE 1
#define MAX_ATTACK_TIME 3
#define MAX_DECAY_TIME 5
#define MAX_VALUE 1000;
#define PI 3.14159
int updateSignalCounter = 0;
int bufferSwitchCounter = 0;
float* params[10];
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

Oscillator osc;
ADSR_Control adsr;
Input masterInput;
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
        signal[i] *= calculateAmp();
    }
    updateSignalCounter++;

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
    osc.frequency = 200;
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
}
int main() {
    //LaunchPad init functions
    InitSysCtrl();
        EALLOW;
        InitMcBSPb_2();
        EALLOW;
        InitSPIA();
        InitAIC23_2();
        EALLOW;
        I2C_O2O_Master_Init(0x27, 200, 100);    //Initialize LCD
        EALLOW;
        PieVectTable.MCBSPB_RX_INT = &Mcbspb_RxINTB_ISR;
        DINT;
        PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
        PieCtrlRegs.PIEIER6.bit.INTx7 = 1;
        EALLOW;
        IER |= 32;
        EINT;
        EALLOW;


    //synth init functions
    masterInput.keyPressed = true;
    initOscADSR();
    //buildKeys();
    getSliderParams();
    updateSignal(ping_buffer, sample_duration);
    updateSignal(pong_buffer, sample_duration);
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
