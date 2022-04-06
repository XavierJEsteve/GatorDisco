#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "include/raygui.h"
#include <math.h>
#include <errno.h>
#include "include/wiringPiSPI.h"
#include <unistd.h>
//#include <sys/soundcard.h>
#include <fcntl.h>
#include "synth.h"
#include "spiConstants.h"
// channel is the wiringPi name for the chip select (or chip enable) pin.
// Set this to 0 or 1, depending on how it's connected.
static const int CHANNEL = 0;

#define STREAM_BUFFER_SIZE 1024
#define SCREEN_HEIGHT 800
#define SCREEN_WIDTH 1280
#define WHITE_KEY_WIDTH 128
#define SLIDER_HEIGHT 150
#define SLIDER_WIDTH 30
#define SLIDER_VISIBLE_WIDTH 10
#define MAX_ATTACK_TIME 3
#define MAX_DECAY_TIME 5
#define NUM_SLIDERS 11
#define NUM_BUTTONS 5
#define MIDI_DEVICE "/dev/midi2"
#define SYNTH_MODE 0
#define EQ_MODE 1
#define NUM_EQ_BANDS 10

typedef struct{
    int byte;
    int module;
    int param;
} SpiHandler;
typedef struct{
    bool black;
    bool pressed;
    int xPos;
} Key;
typedef struct{
    float value;
    int xPos;
    int yPos;
    char* name;
    int param;
} Slider;
typedef struct{
    int xPos;
    int yPos;
    int width;
    int height;
    Color color;
    char* text;
    void (*buttonAction)(int input);
} Button;
typedef struct{
    int x;
    int y;
    int gatePointer;
    int keyPointer;
    bool keyPressed;
    int keyIndex;
} Input;
typedef struct{
    int xPos;
    int yPos;
    int width;
    int height;
    char* title;
    bool noTitle;
} GUI_Section;
typedef struct{
    float fCenter;
    float gain;
    float qFactor;
    float upperLimit;
    float lowerLimit;
} BandGUI;
char* oscNames[NUM_OSCILLATORS] = {"PULSE WAVE", "SAWTOOTH", "WAV FILE", "FREQ MOD", "RING MOD"};
char* oscParam1Names[NUM_OSCILLATORS] = {"PULSE WIDTH", "DETUNE", "PARAM 1", "MOD FREQ", "MOD FREQ"};
char* oscParam2Names[NUM_OSCILLATORS] = {"", "", "PARAM2", "MOD AMP", ""};
int oscTypePointer = 0;
char* effectNames[NUM_EFFECTS] = {"OFF", "ECHO", "BIT CRUSH", "FS REDUCTION", "EFFECT 4", "EFFECT 5"};
char* effectParam1Names[NUM_EFFECTS] = {"", "TIME", "BIT DEPTH", "FS RATIO", "PARAM1", "PARAM1"};
char* effectParam2Names[NUM_EFFECTS] = {"", "VOLUME", "", "", "PARAM2", "PARAM2"};
int effectTypePointer = 0;
char* lfoTargetNames[NUM_LFO_TARGETS] = {"Frequency", "Osc Parameter", "Amplitude"};
char fCenterNames[15][NUM_EQ_BANDS];
int lfoTargetPointer = 0;
int GUI_MODE = SYNTH_MODE;

Key keys[17];
BandGUI bands[NUM_EQ_BANDS];
float bandLimits[NUM_EQ_BANDS + 1] = {22,44,88,177,355,710,1420,2840,5680,11360, 22720};
float fCenters[NUM_EQ_BANDS] = {32,64,125,250,500,1000,2000,4000,8000,16000};
Slider sliders[NUM_SLIDERS];
Button buttons[NUM_BUTTONS];
Button EQButtons[NUM_EQ_BANDS];
GUI_Section guiSections[6];
int currentBand = 0;
unsigned char spi_buffer[100];

Synth synth;
SpiHandler spiHandler;
float wavOutput;
int wavPointer;
float* wavBuffer;
Wave wavSound;

void processSpiInput(int byte){
    //spi_buffer[0] = byte;
    //wiringPiSPIDataRW(CHANNEL, spi_buffer, 1);
    printf("Current Byte: %d\n", spiHandler.byte);
    if(spiHandler.byte == 0 && (byte >> 7) == 1){
        spiHandler.module = (byte >> 4) & 7;
        spiHandler.param = byte & 15;
        spiHandler.byte++;
        printf("Module: %d, Param: %d, Byte: %d\n\n", spiHandler.module,spiHandler.param,spiHandler.byte);
    }
    else if(byte >> 7 == 0 && spiHandler.byte > 0){
        if(spiHandler.module == 0){ // oscillator
            if(spiHandler.param == 0){ // oscSelect
                for(int i = 0; i < 4; i++){
                    synth.osc[i].oscType = byte;
                    synth.osc[i].phase = 0;
                    synth.osc[i].phase2 = 0;
                }
            }
            else if(spiHandler.param == 1) // oscParam1
                for(int i = 0; i < 4; i++)
                    synth.osc[i].param1 = (float)byte / 128;
            else if(spiHandler.param == 2) // oscParam2
                for(int i = 0; i < 4; i++)
                    synth.osc[i].param2 = (float)byte / 128;
            else if(spiHandler.param == 3) // wav frequency
                for(int i = 0; i < 4; i++)
                    synth.osc[i].wavFrequency = synth.keys[0].freq_table[byte];
            spiHandler.byte = 0;
        }
        else if(spiHandler.module == 1){ // keyboard
            //printf("enter keyboard branch\n");
            if(spiHandler.param == 0)
                //printf("get keyboard key\n");
                if(spiHandler.byte == 1){ // get key
                    bool keyInUse = false;
                    for(int i = 0; i < 4; i++){ //check if key is already being played/occupying a channel
                        if(synth.keys[i].key == byte && !keyInUse){
                            //if(byte > 16) printf("octave running\n");
                            synth.keyIndex = i;
                            keyInUse = true;
                            synth.keys[synth.keyIndex].key = byte;
                        }
                    }
                    if(!keyInUse){
                        synth.keyIndex++;
                        synth.keyIndex %= 4;
                        synth.keys[synth.keyIndex].key = byte;
                    }
                    printf("CHANNEL: %d KEY: %d ", synth.keyIndex,synth.keys[synth.keyIndex].key);
                    spiHandler.byte++;
                }
                else if(spiHandler.byte == 2){ // get gate
                    synth.env[synth.keyIndex].gate = byte;
                    spiHandler.byte = 0;
                    printf("GATE: %d\n\n", synth.env[synth.keyIndex].gate);
                }
                else spiHandler.byte = 0;
            else if(spiHandler.param == 1) {//select octave
                printf("CHANGING OCTAVE\n");
                for(int i = 0; i < 4; i++){
                    synth.keys[i].octave = (float)byte / 128;
                }
                spiHandler.byte = 0;
            }
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
                for(int i = 0; i < 4; i++)
                    synth.env[i].attack = (float)byte / 128;
            else if(spiHandler.param == 1) // decay
                for(int i = 0; i < 4; i++)
                    synth.env[i].decay = (float)byte / 128;
            else if(spiHandler.param == 2) // sustain
                for(int i = 0; i < 4; i++)
                    synth.env[i].sustain = (float)byte / 128;
            else if(spiHandler.param == 3) // release
                for(int i = 0; i < 4; i++)
                    synth.env[i].release = (float)byte / 128;
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

void loadWavSound(char* fileName, int key){
    wavSound = LoadWave(fileName);
    wavBuffer = LoadWaveSamples(wavSound);
    wavPointer = 0;
    processSpiInput(SPI_MODULE_OSC | SPI_OSC_WAVFREQ);
    processSpiInput(key);
}
Input masterInput;
void initMasterInput(){
    masterInput.keyPointer = SPI_MODULE_KEYBOARD | SPI_KEYBOARD_KEY;
    masterInput.gatePointer = SPI_MODULE_ENV | SPI_GATE;
}
float buffer[STREAM_BUFFER_SIZE];
int keySelection = -1;

void updateSignal(float* signal){
    for(int i = 0; i < STREAM_BUFFER_SIZE; i++){
        signal[i] = updateSynth(&synth);
        for(int j = 0; j < 4; j++)
        synth.osc[j].wavInput = wavBuffer[wavPointer + i];
    }
    if(wavPointer < 240000);
    //wavPointer += STREAM_BUFFER_SIZE;
}
void drawWaveform(float* signal,int width,int height,int x, int y){
    DrawRectangle(x, y, width, height, WHITE);
    int offset = (int)(synth.osc[synth.keyIndex].phase * (SAMPLE_RATE/synth.osc[synth.keyIndex].frequency));
    int loop = (int)1.0 * (SAMPLE_RATE/synth.osc[synth.keyIndex].frequency);
    if (loop > STREAM_BUFFER_SIZE) loop = STREAM_BUFFER_SIZE;
    int start = (STREAM_BUFFER_SIZE-offset)%loop;
    Vector2 prev;
    prev.x = x;
    prev.y = (height/2)+0.5*(int)(signal[0]*100)+y;
    for(int i = 1; i < width - 1; i++){
        int index = (start + (int)(500*i/(synth.osc[synth.keyIndex].frequency))%loop)%STREAM_BUFFER_SIZE;
        Vector2 current;
        current.x = i+x;
        current.y = (height/2)+0.5*(int)(signal[index]*100)+y;
        DrawLineEx(current, prev, 1.0f, RED);
        prev = current;
    }
}
void buildGuiSections(){
    //build oscillator section
    GUI_Section tempSection;
    tempSection.title = "OSCILLATOR";
    tempSection.xPos = 0;
    tempSection.yPos = 0;
    tempSection.height = SCREEN_HEIGHT*3/8;
    tempSection.width = SCREEN_WIDTH*7/16;
    guiSections[0] = tempSection;
    tempSection.title = "ENVELOPE";
    tempSection.xPos = SCREEN_WIDTH*7/16;
    tempSection.yPos = 0;
    tempSection.height = SCREEN_HEIGHT*3/8;
    tempSection.width = SCREEN_WIDTH*5/16;
    guiSections[1] = tempSection;
    tempSection.title = "WAVEFORM";
    tempSection.xPos = SCREEN_WIDTH*3/4;
    tempSection.yPos = 0;
    tempSection.height = SCREEN_HEIGHT*3/8;
    tempSection.width = SCREEN_WIDTH/4;
    guiSections[2] = tempSection;
    tempSection.title = "LFO";
    tempSection.xPos = 0;
    tempSection.yPos = SCREEN_HEIGHT*3/8;
    tempSection.height = SCREEN_HEIGHT*3/8;
    tempSection.width = SCREEN_WIDTH*11/32;
    guiSections[3] = tempSection;
    tempSection.title = "EFFECTS";
    tempSection.xPos = SCREEN_WIDTH*11/32;
    tempSection.yPos = SCREEN_HEIGHT*3/8;
    tempSection.height = SCREEN_HEIGHT*3/8;
    tempSection.width = SCREEN_WIDTH*11/32;
    guiSections[4] = tempSection;
    tempSection.title = "";
    tempSection.noTitle = true;
    tempSection.xPos = SCREEN_WIDTH*11/16;
    tempSection.yPos = SCREEN_HEIGHT*3/8;
    tempSection.height = SCREEN_HEIGHT*3/8;
    tempSection.width = SCREEN_WIDTH*5/16;
    guiSections[5] = tempSection;
}
void drawGuiSections(){
    for(int i = 0; i < 6; i++){
        GUI_Section temp = guiSections[i];
        //top line
        DrawLine(temp.xPos, temp.yPos, temp.xPos + temp.width,temp.yPos,BLACK);
        //line under title
        if(!temp.noTitle)
        DrawLine(temp.xPos, temp.yPos + 50, temp.xPos + temp.width,temp.yPos + 50,BLACK);
        //left border
        DrawLine(temp.xPos, temp.yPos, temp.xPos,temp.yPos + temp.height,BLACK);
        //right border
        DrawLine(temp.xPos + temp.width, temp.yPos, temp.xPos + temp.width,temp.yPos + temp.height,BLACK);
        if(!temp.noTitle)
        DrawText(temp.title, temp.xPos + temp.width/2 - 6*strlen(temp.title),temp.yPos + 25,20,BLACK);
    }
}
void buildKeys(){
    int xPos = 0;
    for(int i = 0; i < 17; i++){
        Key tempKey;
        tempKey.xPos = xPos;
        tempKey.pressed = false;
        int note = i%12;
        if((note < 4 && note%2 == 1)||(note > 5 && note%2 == 0)){
            tempKey.black = true;
            tempKey.xPos = xPos + WHITE_KEY_WIDTH*3/4;
            xPos += WHITE_KEY_WIDTH;
        }
        else{
            tempKey.black = false;
            if(i > 0){
                if(keys[i-1].black == false){
                    xPos += WHITE_KEY_WIDTH;
                }
            }
            tempKey.xPos = xPos;

        }
        keys[i] = tempKey;
    }
}
void loadConfig(int input){
    //Verify config data saved by python code can be read
    unsigned char config_buffer[11];
    FILE *ptr;
    ptr = fopen("../synth_settings.bin","rb");
    if (! ptr)
    {
        printf("Failed to open synth-settings file\n");

    }
    else
    {
        printf("Successfully opened synth-settings file\n");
        fread(config_buffer,sizeof(config_buffer),10,ptr); //read 8 bytes from config_data.data
        for (int i = 0; i < 10; i++){
            printf("%d\n", config_buffer[i]);
        }
        fclose(ptr);
    }            
}
int changeOsc(int input){
    oscTypePointer++;
    oscTypePointer %= NUM_OSCILLATORS;
    processSpiInput(SPI_MODULE_OSC | SPI_OSCTYPE);
    processSpiInput(oscTypePointer);
    buttons[1].text = oscNames[oscTypePointer];
    sliders[1].name = oscParam1Names[oscTypePointer];
    sliders[2].name = oscParam2Names[oscTypePointer];
    return oscTypePointer;
}
void changeMode(int input){
    if(GUI_MODE == SYNTH_MODE){
        GUI_MODE = EQ_MODE;
        buttons[4].text = "SYNTH MODE";
        buttons[4].xPos = 100;
        buttons[4].yPos = 100;
    }
    else{
        GUI_MODE = SYNTH_MODE;
        buttons[4].text = "EQ MODE";
        buttons[4].xPos = 930;
        buttons[4].yPos = 350;
    }
}
void changeLfo(int input){
    lfoTargetPointer++;
    lfoTargetPointer %= NUM_LFO_TARGETS;
    processSpiInput(SPI_MODULE_LFO | SPI_LFO_TARGET);
    processSpiInput(lfoTargetPointer);
    buttons[2].text = lfoTargetNames[lfoTargetPointer];
}
void changeEQBand(int input){
    currentBand = input;
}
void updateCenterFreq(float frequency){
    bands[currentBand].fCenter = frequency;
    sprintf(fCenterNames[currentBand], "%d Hz", (int)frequency);
}
void changeEffect(void){
    effectTypePointer++;
    effectTypePointer %= NUM_EFFECTS;
    processSpiInput(SPI_MODULE_FX | SPI_FX_SEL);
    processSpiInput(effectTypePointer);
    buttons[3].text = effectNames[effectTypePointer];
    sliders[9].name = effectParam1Names[effectTypePointer];
    sliders[10].name = effectParam2Names[effectTypePointer];
}
void buildButtons(){
    Button load_config;
    load_config.xPos = 1090;
    load_config.yPos = 350;
    load_config.width = SCREEN_WIDTH/10;
    load_config.height = SCREEN_HEIGHT/12;
    load_config.color = BLACK;
    load_config.text = "LOAD CONFIG";
    load_config.buttonAction = &loadConfig;
    buttons[0] = load_config;
    Button oscSelect;
    oscSelect.xPos = 40;
    oscSelect.yPos = SCREEN_HEIGHT/5;
    oscSelect.width = SCREEN_WIDTH/10;
    oscSelect.height = SCREEN_HEIGHT/12;
    oscSelect.color = GREEN;
    oscSelect.text = "PULSE WAVE";
    oscSelect.buttonAction = &changeOsc;
    buttons[1] = oscSelect;
    Button lfoSelect;
    lfoSelect.xPos = 40;
    lfoSelect.yPos = SCREEN_HEIGHT/2;
    lfoSelect.width = SCREEN_WIDTH/10;
    lfoSelect.height = SCREEN_HEIGHT/12;
    lfoSelect.color = GREEN;
    lfoSelect.text = "Frequency";
    lfoSelect.buttonAction = &changeLfo;
    buttons[2] = lfoSelect;
    Button effectSelect;
    effectSelect.xPos = 480;
    effectSelect.yPos = SCREEN_HEIGHT/2;
    effectSelect.width = SCREEN_WIDTH/10;
    effectSelect.height = SCREEN_HEIGHT/12;
    effectSelect.color = GREEN;
    effectSelect.text = "OFF";
    effectSelect.buttonAction = &changeEffect;
    buttons[3] = effectSelect;
    Button eqMode;
    eqMode.xPos = 930;
    eqMode.yPos = 350;
    eqMode.width = SCREEN_WIDTH/10;
    eqMode.height = SCREEN_HEIGHT/12;
    eqMode.color = GREEN;
    eqMode.text = "EQ MODE";
    eqMode.buttonAction = &changeMode;
    buttons[4] = eqMode;
}
void buildBandGUIs(){
    for(int i = 0; i < NUM_EQ_BANDS; i++){
        BandGUI tempBand;
        //calculate slider values
        tempBand.fCenter = fCenters[i];
        tempBand.gain = 0;
        tempBand.qFactor = 1;
        tempBand.lowerLimit = bandLimits[i];
        tempBand.upperLimit = bandLimits[i+1];
        bands[i] = tempBand;
    }
}
void buildEQButtons(){
    int xPosStart = 150;
    int buttonWidth = 100;
    for(int i = 0; i < NUM_EQ_BANDS; i++){
        Button tempButton;
        tempButton.height = 60;
        tempButton.width = buttonWidth;
        tempButton.xPos = xPosStart + buttonWidth*i;
        tempButton.yPos = 200;
        tempButton.buttonAction = &changeEQBand;
        sprintf(fCenterNames[i],"%d Hz", (int)fCenters[i]);
        tempButton.text = fCenterNames[i];
        EQButtons[i] = tempButton;
    }
}
void buildSliders(){
    Slider octave;
    octave.xPos = 200;
    octave.yPos = 80;
    octave.value = 0;
    octave.param = SPI_MODULE_KEYBOARD | SPI_KEYBOARD_OCTAVE;
    octave.name = "OCTAVE";
    sliders[0] = octave;
    Slider oscParam1;
    oscParam1.xPos = 300;
    oscParam1.yPos = 80;
    oscParam1.value = 0;
    oscParam1.param = SPI_MODULE_OSC | SPI_OSCPARAM1;
    oscParam1.name = "PULSE WIDTH";
    sliders[1] = oscParam1;
    Slider oscParam2;
    oscParam2.xPos = 420;
    oscParam2.yPos = 80;
    oscParam2.value = 0;
    oscParam2.param = SPI_MODULE_OSC | SPI_OSCPARAM2;
    oscParam2.name = "";
    sliders[2] = oscParam2;
    Slider Attack;
    Attack.xPos = 600;
    Attack.yPos = 80;
    Attack.value = 0;
    Attack.param = SPI_MODULE_ENV | SPI_ENV_ATTACK;
    Attack.name = "A";
    sliders[3] = Attack;
    Slider Decay;
    Decay.xPos = 700;
    Decay.yPos = 80;
    Decay.value = 0;
    Decay.param = SPI_MODULE_ENV | SPI_ENV_DECAY;
    Decay.name = "D";
    sliders[4] = Decay;
    Slider Sustain;
    Sustain.xPos = 800;
    Sustain.yPos = 80;
    Sustain.value = 0;
    Sustain.param = SPI_MODULE_ENV | SPI_ENV_SUSTAIN;
    Sustain.name = "S";
    sliders[5] = Sustain;
    Slider Release;
    Release.xPos = 900;
    Release.yPos = 80;
    Release.value = 0;
    Release.param = SPI_MODULE_ENV | SPI_ENV_RELEASE;
    Release.name = "R";
    sliders[6] = Release;
    Slider lfoSpeed;
    lfoSpeed.xPos = 200;
    lfoSpeed.yPos = 380;
    lfoSpeed.value = 0;
    lfoSpeed.param = SPI_MODULE_LFO | SPI_LFO_SPEED;
    lfoSpeed.name = "SPEED";
    sliders[7] = lfoSpeed;
    Slider lfoval;
    lfoval.xPos = 320;
    lfoval.yPos = 380;
    lfoval.value = 0;
    lfoval.param = SPI_MODULE_LFO | SPI_LFO_VAL;
    lfoval.name = "VALUE";
    sliders[8] = lfoval;
    Slider Effect1;
    Effect1.xPos = 640;
    Effect1.yPos = 380;
    Effect1.value = 0;
    Effect1.param = SPI_MODULE_FX | SPI_FX_PARAM1;
    Effect1.name = "";
    sliders[9] = Effect1;
    Slider Effect2;
    Effect2.xPos = 760;
    Effect2.yPos = 380;
    Effect2.value = 0;
    Effect2.param = SPI_MODULE_FX | SPI_FX_PARAM2;
    Effect2.name = "";
    sliders[10] = Effect2;
}
void drawSliders(){
    for(int i = 0; i < NUM_SLIDERS; i++){
        Slider tempSlider = sliders[i];
        if(strlen(tempSlider.name) > 0){
            //draw black rectangle
            int visibleXPos = tempSlider.xPos + SLIDER_WIDTH/2 - SLIDER_VISIBLE_WIDTH/2;
            DrawRectangle(visibleXPos, tempSlider.yPos, SLIDER_VISIBLE_WIDTH, SLIDER_HEIGHT, BLACK);
            //draw white rectangle representing current value
            int whiteRectHeight = SLIDER_HEIGHT*tempSlider.value;
            DrawRectangle(visibleXPos, tempSlider.yPos+SLIDER_HEIGHT-whiteRectHeight, SLIDER_VISIBLE_WIDTH, whiteRectHeight, WHITE);
            //draw circle at end of white rectangle
            //DrawCircle(int centerX, int centerY, float radius, Color color);
            DrawCircle(tempSlider.xPos+SLIDER_WIDTH/2, tempSlider.yPos+SLIDER_HEIGHT-whiteRectHeight, SLIDER_WIDTH/2, RED);
            //draw text
            DrawText(tempSlider.name, tempSlider.xPos, tempSlider.yPos + SLIDER_HEIGHT + 30, 20, BLACK);
        }
    }
}
void drawKeys(int height){
    //draw white keys
    for(int i = 0; i < 17; i++){
        Key tempKey = keys[i];
        if(!tempKey.black){
            if(!tempKey.pressed){
                DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/10, height, WHITE);
            }
            else{
                DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/10, height, DARKGRAY);
            }
                DrawRectangleLines(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/10, height, BLACK);
        }
    }
    //draw black keys
    for(int i = 0; i < 17; i++){
        Key tempKey = keys[i];
        if(tempKey.black){
            if(!tempKey.pressed){
                DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/20, height/2, BLACK);
            }
            else{
                DrawRectangle(tempKey.xPos, SCREEN_HEIGHT-height, SCREEN_WIDTH/20, height/2, DARKGRAY);
            }
        }
    }
}
void drawButtons(){
    for(int i = 0; i < NUM_BUTTONS; i++){
        Button tempButton = buttons[i];
        bool pressed = GuiButton((Rectangle){
        tempButton.xPos,
        tempButton.yPos,
        tempButton.width,
        tempButton.height
        }, tempButton.text);
        if(pressed){
            tempButton.buttonAction(0);
        }
    } 
}
void drawEQButtons(){
    for(int i = 0; i < NUM_EQ_BANDS; i++){
        Button tempButton = EQButtons[i];
        bool pressed = GuiButton((Rectangle){
        tempButton.xPos,
        tempButton.yPos,
        tempButton.width,
        tempButton.height
        }, tempButton.text);
        if(pressed){
            tempButton.buttonAction(i);
        }
    } 
    Button tempButton = buttons[4];
    bool pressed = GuiButton((Rectangle){
    tempButton.xPos,
    tempButton.yPos,
    tempButton.width,
    tempButton.height
    }, tempButton.text);
    if(pressed){
        tempButton.buttonAction(0);
    }
}
void drawEQSliders(){
    int xPos = 200;
    int width = 800;
    int height = 75;
    int yPos = 300;
    char leftFreqText[10];
    char rightFreqText[10];
    sprintf(leftFreqText, "%d Hz", (int)bandLimits[currentBand]);
    sprintf(rightFreqText, "%d Hz", (int)bandLimits[currentBand+1]);
    BandGUI tempBand = bands[currentBand];
    //center frequency slider
    updateCenterFreq(GuiSlider((Rectangle){
        xPos,
        yPos,
        width,
        height
    },"",
    "Center Frequency",
    bands[currentBand].fCenter,
    bandLimits[currentBand], 
    bandLimits[currentBand + 1]));
    //gain slider
    yPos += 100;
    bands[currentBand].gain = GuiSlider((Rectangle){
        xPos,
        yPos,
        width,
        height
    },"",
    "Gain (dB)",
    bands[currentBand].gain,
    -15, 
    15);
    //qFactor slider
    yPos += 100;
    bands[currentBand].qFactor = GuiSlider((Rectangle){
        xPos,
        yPos,
        width,
        height
    },"",
    "Q Factor",
    bands[currentBand].qFactor,
    0.1, 
    10);
    //if values changed, update via spi
    if(tempBand.fCenter != bands[currentBand].fCenter || 
        tempBand.gain != bands[currentBand].gain || 
        tempBand.qFactor != bands[currentBand].qFactor)
    {
        processSpiInput(SPI_MODULE_FILTER | currentBand);
        //fCenter ratio
        float ratio = (bands[currentBand].fCenter - bands[currentBand].lowerLimit) / 
            (bands[currentBand].upperLimit - bands[currentBand].lowerLimit);
        processSpiInput(127 * ratio);
        //gain ratio
        ratio = (bands[currentBand].gain + 15) / 30;
        processSpiInput(127 * ratio);
        //qFactor ratio
        ratio = bands[currentBand].qFactor / 10;
        processSpiInput(127 * ratio);
    }
}
void drawGUI(){
    BeginDrawing();
    ClearBackground(GRAY);
    if(GUI_MODE == SYNTH_MODE){
        drawWaveform(buffer,SCREEN_WIDTH*3/16,SCREEN_HEIGHT*7/32,SCREEN_WIDTH*25/32,80);
        drawKeys(SCREEN_HEIGHT/4);
        drawSliders();
        drawButtons();
        drawGuiSections();
    }
    else{
        drawEQButtons();
        drawEQSliders();
    }
    EndDrawing();
}
int clearPressCounter;
void clearKeyPress(){
    int octave = 0;
    if(sliders[0].value > 0.75) octave = 3;
    else if(sliders[0].value > 0.5) octave = 2;
    else if(sliders[0].value > 0.25) octave = 1;
    if(masterInput.keyPressed == true){
        printf("clear key press %d\n", clearPressCounter);
        clearPressCounter++;
        for(int i = 0; i < 17; i++){
            if(keys[i].pressed == true){
                processSpiInput(masterInput.keyPointer);
                processSpiInput(i + 12*octave);
                if(octave != 0)
                printf("octave command\n");
                processSpiInput(0);
            }
            keys[i].pressed = false;
        }
        masterInput.keyPressed = false;
    }
}
void processInput(){
    masterInput.y = GetMouseY();
    masterInput.x = GetMouseX();

    if(IsMouseButtonDown(0)){
        if(masterInput.y > (3*SCREEN_HEIGHT / 4)){
            int octave = 0;
            if(sliders[0].value > 0.75) octave = 3;
            else if(sliders[0].value > 0.5) octave = 2;
            else if(sliders[0].value > 0.25) octave = 1;
            if(masterInput.keyPressed == false){
                wavPointer = 0;
                masterInput.keyPressed = true;
            }
            bool checkBlack = false;
            bool foundKey = false;
            if(masterInput.y < 7*SCREEN_HEIGHT/8) checkBlack = true;
            for(int i = 0; i < 17; i++){
                keys[i].pressed = false;
                if(!foundKey){
                    Key tempKey = keys[i];
                    if(tempKey.black){
                        if(checkBlack){
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH/2){
                                masterInput.keyIndex = i + 12*octave;
                                keys[i].pressed = true;
                                foundKey = true;
                            }
                        }
                    }
                    else{
                        if(!checkBlack){
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH){
                                if(masterInput.keyIndex != i + 12*octave){
                                    processSpiInput(masterInput.keyPointer);
                                    processSpiInput(masterInput.keyIndex);
                                    processSpiInput(0);
                                    masterInput.keyIndex = i +12*octave;
                                }
                                foundKey = true;
                            }
                        }
                        else{
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH*0.75){
                                if(masterInput.keyIndex != i + 12*octave){
                                    processSpiInput(masterInput.keyPointer);
                                    processSpiInput(masterInput.keyIndex);
                                    processSpiInput(0);
                                    masterInput.keyIndex = i +12*octave;
                                }
                                foundKey = true;
                            }
                        }
                    }
                }
            }
            //*masterInput.key = keyIndex;
            processSpiInput(masterInput.keyPointer);
            processSpiInput(masterInput.keyIndex);
            processSpiInput(1);
            keys[masterInput.keyIndex - 12*octave].pressed = true;
        }
        else{
            clearKeyPress();
            //check if slider is selected
            for(int i = 0; i < NUM_SLIDERS; i++){
                Slider tempSlider = sliders[i];
                if(masterInput.x > tempSlider.xPos && masterInput.x -tempSlider.xPos < SLIDER_WIDTH && masterInput.y > tempSlider.yPos && masterInput.y -tempSlider.yPos < SLIDER_HEIGHT){
                    tempSlider.value = (float)(tempSlider.yPos + SLIDER_HEIGHT - masterInput.y)/SLIDER_HEIGHT;
                    if(tempSlider.value < 0.01) tempSlider.value = 0;
                    //*tempSlider.param = tempSlider.value;
                    processSpiInput(tempSlider.param);
                    processSpiInput(tempSlider.value * 127);
                    sliders[i] = tempSlider;
                    //printf("SPI COMMAND\n");
                    //printf("%d (%s)\n", i+2,tempSlider.name);
                    int output = 127*tempSlider.value;
                    //printf("%d\n", output);
                    spi_buffer[0] = 128 | (i+2);
                    spi_buffer[1] = output;
                    //wiringPiSPIDataRW(CHANNEL, spi_buffer, 2);
                }
            }
        }
    }
    else {
        clearKeyPress();
    }
}


void main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Synth");
    SetTargetFPS(60);
    InitAudioDevice();
    initMasterInput();
    initSynth(&synth);
    loadWavSound("piano.wav",36);
    buildKeys();
    buildSliders();
    buildBandGUIs();
    buildButtons();
    buildGuiSections();
    buildEQButtons();
    SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
    AudioStream synthStream = LoadAudioStream(SAMPLE_RATE,
        32 ,
        1
    );
    SetAudioStreamVolume(synthStream, 0.25f);
    PlayAudioStream(synthStream);
    
    //spi config
    //int fd, result;

    //cout << "Initializing" << endl ;

    // Configure the interface.
    // CHANNEL insicates chip select,
    // 50000 indicates bus speed.
    //fd = wiringPiSPISetup(CHANNEL, 50000);

    //cout << "Init result: " << fd << endl;

    // clear display
	/*
    spi_buffer[0] = 0x76;
    wiringPiSPIDataRW(CHANNEL, spi_buffer, 1);
        unsigned char firstByte = 0;
        unsigned char secondByte = 0;
        unsigned char midipacket[4];
	
        int seqfd = open(MIDI_DEVICE, O_RDONLY);
        if (seqfd == -1) {
                printf("Error: cannot open %s\n", MIDI_DEVICE);
                exit(1);
        }
	
    sleep(5);
*/
    while(WindowShouldClose() == false)
    {
        if(IsAudioStreamProcessed(synthStream)){
            UpdateAudioStream(synthStream, buffer, STREAM_BUFFER_SIZE);
            processInput();
            updateSignal(buffer);
            drawGUI();
		/*
                read(seqfd, &midipacket, sizeof(midipacket));
		if(firstByte != midipacket[1] || secondByte != midipacket[2]){
                        spi_buffer[0] = 128;
                        spi_buffer[1] = midipacket[2];
                        //wiringPiSPIDataRW(CHANNEL, spi_buffer, 2);
                        spi_buffer[0] = 129;
                        spi_buffer[1] = midipacket[1] - 24;
                        //wiringPiSPIDataRW(CHANNEL, spi_buffer, 2);
                        firstByte = midipacket[1];
                        secondByte = midipacket[2];
		}
		*/
        }
    }
    CloseAudioDevice();
    CloseWindow();
}

