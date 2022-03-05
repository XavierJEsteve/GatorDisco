#include "AIC23.h"
#include "SPIB.h"
#include <F2837xD_device.h>
#include <F28x_Project.h>
#include <math.h>
#include "BiquadEQ.h"
#include "synth.h"
#include "spiConstants.h"

interrupt void Mcbspb_RxINTB_ISR(void);
interrupt void Spi_RxINTB_ISR(void);

#define SAMPLE_RATE 48000
#define STREAM_BUFFER_SIZE 1
#define MAX_ATTACK_TIME 3
#define MAX_DECAY_TIME 5
#define MAX_VALUE 16384;
#define PI 3.14159


typedef struct{
    bool black;
    float frequency;
    bool pressed;
    int xPos;
} Key;
typedef struct{
    int x;
    int y;
    bool* keyPressed;
    int* key;
} Input;
typedef struct{
    int byte;
    int module;
    int param;
} SpiHandler;

Input masterInput;
Synth synth;
SpiHandler spiHandler;


bool ping = false;
int16 ping_buffer[STREAM_BUFFER_SIZE];
int16 pong_buffer[STREAM_BUFFER_SIZE];
int samplePointer = 0;
bool updatingFilters = false;
float sample_duration;

void updateSignal(int16* signal){
    for(int i = 0; i < STREAM_BUFFER_SIZE; i++){
        float val = updateSynth(&synth);
        signal[i] = (int16)val;
    }
}

void processSpiInput(int byte){

    //spi_buffer[0] = byte;
    //wiringPiSPIDataRW(CHANNEL, spi_buffer, 1);
    if(spiHandler.byte == 0 && (byte >> 7) == 1){
        spiHandler.module = (byte >> 4) & 7;
        spiHandler.param = byte & 15;
        spiHandler.byte++;
    }
    else if(byte >> 7 == 0 && spiHandler.byte > 0){
        if(spiHandler.module == 0){ // oscillator
            if(spiHandler.param == 0){ // oscSelect
                synth.osc.oscType = byte;
                synth.osc.phase = 0;
                synth.osc.phase2 = 0;
            }
            else if(spiHandler.param == 1) // oscParam1
            synth.osc.param1 = (float)byte / 128;
            else if(spiHandler.param == 2) // oscParam2
            synth.osc.param2 = (float)byte / 128;
            spiHandler.byte = 0;
        }
        else if(spiHandler.module == 1){ // keyboard
            if(spiHandler.param == 0) //select key
            synth.keys.key = byte;
            else if(spiHandler.param == 1) //select octave
            synth.keys.octave = (float)byte / 128;
            spiHandler.byte = 0;
        }
        else if(spiHandler.module == 2){ // LFO
            if(spiHandler.param == 0) // lfo speed
            synth.lfo.speed = (float)byte / 128;
            else if(spiHandler.param == 1) // lfo value
            synth.lfo.val = (float)byte / 128;
            else if(spiHandler.param == 2){ // lfo target
                synth.lfo.target = byte;
                synth.lfo.output = synth.lfo.targets[synth.lfo.target];
            }
            spiHandler.byte = 0;
        }
        else if(spiHandler.module == 3){ // envelope
            if(spiHandler.param == 0) // attack
            synth.env.attack = (float)byte / 128;
            else if(spiHandler.param == 1) // decay
            synth.env.decay = (float)byte / 128;
            else if(spiHandler.param == 2) // sustain
            synth.env.sustain = (float)byte / 128;
            else if(spiHandler.param == 3) // release
            synth.env.release = (float)byte / 128;
            else if(spiHandler.param == 4) // gate
            synth.env.gate = byte;
            spiHandler.byte = 0;
        }
        else if(spiHandler.module == 4){ // effects
            if(spiHandler.param == SPI_FX_SEL)
            synth.fx.effectType = byte;
            else if(spiHandler.param == SPI_FX_PARAM1)
            synth.fx.param1 = (float)byte / 128;
            else if(spiHandler.param == SPI_FX_PARAM2)
            synth.fx.param2 = (float)byte / 128;
            spiHandler.byte = 0;
        }
        else if(spiHandler.module == 5){ // EQ
            // fCenter update
            if(spiHandler.byte == 1){
                synth.filter.fCenter = (float)byte / 128;
                spiHandler.byte++;
            }
            // gain update
            else if(spiHandler.byte == 2){
                synth.filter.gain = (float)byte / 128;
                spiHandler.byte++;
            }
            // qFactor update
            else if(spiHandler.byte == 3){
                synth.filter.qFactor = (float)byte / 128;
                //update filter
                float Fcenter = (synth.filter.EQ[spiHandler.param].low * synth.filter.fCenter) + synth.filter.EQ[spiHandler.param].low;
                float Gain = (30 * synth.filter.gain) + -15.0;
                float Q = (9.9f * synth.filter.qFactor) + 0.1f;
                synth.filter.updateFlag = true;
                updateParameters(&synth.filter.EQ[spiHandler.param],Gain,Fcenter,Q);
                synth.filter.updateFlag = false;
                spiHandler.byte = 0;
            }
        }
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

    initSynth(&synth);
    //filter init
    updateSignal(ping_buffer);
    updateSignal(pong_buffer);
    while(1)
    {
        if(samplePointer == STREAM_BUFFER_SIZE){
            //UpdateAudioStream(synthStream, buffer, STREAM_BUFFER_SIZE);
            //processInput();
            if(ping){
                updateSignal(ping_buffer);
                ping = false;
            }
            else{
                updateSignal(pong_buffer);
                ping = true;
            }
            samplePointer = 0;
        }
    }
    return 0;
}

int16 maxData = 0;
interrupt void Mcbspb_RxINTB_ISR(void)
{
    EALLOW;
    int16 LeftData = McbspbRegs.DRR2.all;
    int16 RightData = McbspbRegs.DRR1.all;
    synth.filter.input = ((float)LeftData + (float)RightData)/2;
    int16 data;
    //codecEvent = true;
    if(ping){
        data = ping_buffer[samplePointer];
        //McbspbRegs.DXR2.all = ping_buffer[samplePointer];
        //McbspbRegs.DXR1.all = ping_buffer[samplePointer];
    }
    else{
        data = pong_buffer[samplePointer];
        //McbspbRegs.DXR2.all = pong_buffer[samplePointer];
        //McbspbRegs.DXR1.all = pong_buffer[samplePointer];
    }

        if(data > maxData){
                maxData = data;
            }
        McbspbRegs.DXR2.all = data;
        McbspbRegs.DXR1.all = data;

    samplePointer++;
    EALLOW;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
}
Uint16 spiData[100];
int spiDataPointer = 0;
interrupt void Spi_RxINTB_ISR(void){
    EALLOW;
    Uint16 data = SpibRegs.SPIRXBUF & 255;
    processSpiInput(data);
    spiData[spiDataPointer] = data;
    spiDataPointer++;
    spiDataPointer %= 100;
    SpibRegs.SPISTS.bit.INT_FLAG = 0;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;
}
