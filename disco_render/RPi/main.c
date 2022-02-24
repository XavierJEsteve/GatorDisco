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
#include <sys/soundcard.h>
#include <fcntl.h>
#include "include/synth.h"
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
#define NUM_SLIDERS 10
#define NUM_BUTTONS 4
#define MIDI_DEVICE "/dev/midi2"

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
    int (*buttonAction)(void);
} Button;
typedef struct{
    int x;
    int y;
    int gatePointer;
    int keyPointer;
    bool keyPressed;
} Input;
char* oscNames[NUM_OSCILLATORS] = {"PULSE WAVE", "SAWTOOTH", "OSCILLATOR 3", "OSCILLATOR 4"};
char* oscParamNames[NUM_OSCILLATORS] = {"PULSE WIDTH", "DETUNE", "OSC3 PARAM", "OSC4 PARAM"};
int oscTypePointer = 0;
char* effectNames[NUM_EFFECTS] = {"OFF", "ECHO", "BIT CRUSH", "FS REDUCTION", "EFFECT 4", "EFFECT 5"};
char* effectParam1Names[NUM_EFFECTS] = {"", "TIME", "BIT DEPTH", "FS RATIO", "PARAM1", "PARAM1"};
char* effectParam2Names[NUM_EFFECTS] = {"", "VOLUME", "", "", "PARAM2", "PARAM2"};
int effectTypePointer = 0;
char* lfoTargetNames[NUM_LFO_TARGETS] = {"Frequency", "Osc Parameter", "Amplitude"};
int lfoTargetPointer = 0;


Key keys[17];
Slider sliders[NUM_SLIDERS];
Button buttons[NUM_BUTTONS];
unsigned char spi_buffer[100];

Synth synth;
SpiHandler spiHandler;

void processSpiInput(int byte){
    printf("sent byte: %d\n", byte);
    spi_buffer[0] = byte;
    wiringPiSPIDataRW(CHANNEL, spi_buffer, 1);
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
            else if(spiHandler.param == 1) // oscParam
            synth.osc.param1 = (float)byte / 128;
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

        }
    }
}

Input masterInput;
void initMasterInput(){
    masterInput.keyPointer = SPI_MODULE_KEYBOARD | SPI_KEYBOARD_KEY;
    masterInput.gatePointer = SPI_MODULE_ENV | SPI_GATE;
}
float buffer[STREAM_BUFFER_SIZE];
int keySelection = -1;
/*
void updateSignal(float* signal){
    for(int i = 0; i < STREAM_BUFFER_SIZE; i++){
        signal[i] = updateSynth(&synth);
    }
}
*/
void drawWaveform(float* signal,int width,int height,int x, int y){
    DrawRectangle(x, y, width, height, WHITE);
    int offset = (int)(synth.osc.phase * (SAMPLE_RATE/synth.osc.frequency));
    int loop = (int)1.0 * (SAMPLE_RATE/synth.osc.frequency);
    if (loop > STREAM_BUFFER_SIZE) loop = STREAM_BUFFER_SIZE;
    int start = (STREAM_BUFFER_SIZE-offset)%loop;
    Vector2 prev;
    prev.x = x;
    prev.y = (height/2)+0.5*(int)(signal[0]*100)+y;
    for(int i = 1; i < width - 1; i++){
        int index = (start + (int)(500*i/(synth.osc.frequency))%loop)%STREAM_BUFFER_SIZE;
        Vector2 current;
        current.x = i+x;
        current.y = (height/2)+0.5*(int)(signal[index]*100)+y;
        DrawLineEx(current, prev, 1.0f, RED);
        prev = current;
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
void loadConfig(void){
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
void changeOsc(void){
    oscTypePointer++;
    oscTypePointer %= NUM_OSCILLATORS;
    processSpiInput(SPI_MODULE_OSC | SPI_OSCTYPE);
    processSpiInput(oscTypePointer);
    buttons[1].text = oscNames[oscTypePointer];
    sliders[1].name = oscParamNames[oscTypePointer];
}
void changeLfo(void){
    lfoTargetPointer++;
    lfoTargetPointer %= NUM_LFO_TARGETS;
    processSpiInput(SPI_MODULE_LFO | SPI_LFO_TARGET);
    processSpiInput(lfoTargetPointer);
    buttons[2].text = lfoTargetNames[lfoTargetPointer];
}
void changeEffect(void){
    effectTypePointer++;
    effectTypePointer %= NUM_EFFECTS;
    processSpiInput(SPI_MODULE_FX | SPI_FX_SEL);
    processSpiInput(effectTypePointer);
    buttons[3].text = effectNames[effectTypePointer];
    sliders[8].name = effectParam1Names[effectTypePointer];
    sliders[9].name = effectParam2Names[effectTypePointer];
}
void buildButtons(){
    Button load_config;
    load_config.xPos = (3*SCREEN_WIDTH/5);
    load_config.yPos = SCREEN_HEIGHT/3;
    load_config.width = SCREEN_WIDTH/5;
    load_config.height = SCREEN_HEIGHT/12;
    load_config.color = BLACK;
    load_config.text = "LOAD CONFIG";
    load_config.buttonAction = &loadConfig;
    buttons[0] = load_config;
    Button oscSelect;
    oscSelect.xPos = (SCREEN_WIDTH/32);
    oscSelect.yPos = SCREEN_HEIGHT/5;
    oscSelect.width = SCREEN_WIDTH/8;
    oscSelect.height = SCREEN_HEIGHT/12;
    oscSelect.color = GREEN;
    oscSelect.text = "PULSE WAVE";
    oscSelect.buttonAction = &changeOsc;
    buttons[1] = oscSelect;
    Button lfoSelect;
    lfoSelect.xPos = 9* SCREEN_WIDTH/16;
    lfoSelect.yPos = SCREEN_HEIGHT/5;
    lfoSelect.width = SCREEN_WIDTH/8;
    lfoSelect.height = SCREEN_HEIGHT/12;
    lfoSelect.color = GREEN;
    lfoSelect.text = "Frequency";
    lfoSelect.buttonAction = &changeLfo;
    buttons[2] = lfoSelect;
    Button effectSelect;
    effectSelect.xPos = 12* SCREEN_WIDTH/16;
    effectSelect.yPos = SCREEN_HEIGHT/2;
    effectSelect.width = SCREEN_WIDTH/8;
    effectSelect.height = SCREEN_HEIGHT/12;
    effectSelect.color = GREEN;
    effectSelect.text = "OFF";
    effectSelect.buttonAction = &changeEffect;
    buttons[3] = effectSelect;
}
void buildSliders(){
    Slider octave;
    octave.xPos = 200;
    octave.yPos = 100;
    octave.value = 0;
    octave.param = SPI_MODULE_KEYBOARD | SPI_KEYBAORD_OCTAVE;
    octave.name = "OCTAVE";
    sliders[0] = octave;
    Slider oscParam1;
    oscParam1.xPos = 300;
    oscParam1.yPos = 100;
    oscParam1.value = 0;
    oscParam1.param = SPI_MODULE_OSC | SPI_OSCPARAM1;
    oscParam1.name = "OSC PARAM";
    sliders[1] = oscParam1;
    Slider oscParam2;
    oscParam2.xPos = 450;
    oscParam2.yPos = 100;
    oscParam2.value = 0;
    oscParam2.param = SPI_MODULE_LFO | SPI_LFO_SPEED;
    oscParam2.name = "LFO Freq";
    sliders[2] = oscParam2;
    Slider oscParam3;
    oscParam3.xPos = 600;
    oscParam3.yPos = 100;
    oscParam3.value = 0;
    oscParam3.param = SPI_MODULE_LFO | SPI_LFO_VAL;
    oscParam3.name = "LFO Val";
    sliders[3] = oscParam3;
    Slider Attack;
    Attack.xPos = 200;
    Attack.yPos = 350;
    Attack.value = 0;
    Attack.param = SPI_MODULE_ENV | SPI_ENV_ATTACK;
    Attack.name = "Attack";
    sliders[4] = Attack;
    Slider Decay;
    Decay.xPos = 300;
    Decay.yPos = 350;
    Decay.value = 0;
    Decay.param = SPI_MODULE_ENV | SPI_ENV_DECAY;
    Decay.name = "Decay";
    sliders[5] = Decay;
    Slider Sustain;
    Sustain.xPos = 450;
    Sustain.yPos = 350;
    Sustain.value = 0;
    Sustain.param = SPI_MODULE_ENV | SPI_ENV_SUSTAIN;
    Sustain.name = "Sustain";
    sliders[6] = Sustain;
    Slider Release;
    Release.xPos = 600;
    Release.yPos = 350;
    Release.value = 0;
    Release.param = SPI_MODULE_ENV | SPI_ENV_RELEASE;
    Release.name = "Release";
    sliders[7] = Release;
    Slider Effect1;
    Effect1.xPos = 750;
    Effect1.yPos = 350;
    Effect1.value = 0;
    Effect1.param = SPI_MODULE_FX | SPI_FX_PARAM1;
    Effect1.name = "";
    sliders[8] = Effect1;
    Slider Effect2;
    Effect2.xPos = 900;
    Effect2.yPos = 350;
    Effect2.value = 0;
    Effect2.param = SPI_MODULE_FX | SPI_FX_PARAM2;
    Effect2.name = "";
    sliders[9] = Effect2;
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
            DrawText(tempSlider.name, tempSlider.xPos, tempSlider.yPos + SLIDER_HEIGHT + 30, 15, BLACK);
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
            tempButton.buttonAction();
        }
    } 
}
void drawGUI(){
    BeginDrawing();
    ClearBackground(GRAY);
    drawWaveform(buffer,SCREEN_WIDTH/6,SCREEN_HEIGHT/6,SCREEN_WIDTH-(SCREEN_WIDTH*1.5/6),SCREEN_HEIGHT/12);
    drawKeys(SCREEN_HEIGHT/4);
    drawSliders();
    drawButtons();
    EndDrawing();
}
void clearKeyPress(){
    if(masterInput.keyPressed){
        for(int i = 0; i < 17; i++){
            keys[i].pressed = false;
        }
        if(masterInput.keyPressed == true){
            /*
            printf("SPI COMMAND\n");
            printf("00000000 (Keypressed)\n");
            printf("00000000\n");
            */
            spi_buffer[0] = 128;
            spi_buffer[1] = 0;
            //wiringPiSPIDataRW(CHANNEL, spi_buffer, 2);
        }
        masterInput.keyPressed = false;
        processSpiInput(masterInput.gatePointer);
        processSpiInput(0);
    }
}
void processInput(){
    masterInput.y = GetMouseY();
    masterInput.x = GetMouseX();

    if(IsMouseButtonDown(0)){
        if(masterInput.y > (3*SCREEN_HEIGHT / 4)){
            if(masterInput.keyPressed == false){
                /*
                printf("SPI COMMAND\n");
                printf("00000000 (Keypressed)\n");
                printf("00000001\n");
                */
                spi_buffer[0] = 128;
                spi_buffer[1] = 1;
                //wiringPiSPIDataRW(CHANNEL, spi_buffer, 2);
            }
            masterInput.keyPressed = true;
            processSpiInput(masterInput.gatePointer);
            processSpiInput(1);
            printf("mouse button down\n");
            bool checkBlack = false;
            bool foundKey = false;
            int keyIndex = -1;
            if(masterInput.y < 7*SCREEN_HEIGHT/8) checkBlack = true;
            for(int i = 0; i < 17; i++){
                keys[i].pressed = false;
                if(!foundKey){
                    Key tempKey = keys[i];
                    if(tempKey.black){
                        if(checkBlack){
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH/2){
                                keyIndex = i;
                                foundKey = true;
                            }
                        }
                    }
                    else{
                        if(!checkBlack){
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH){
                                keyIndex = i;
                                foundKey = true;
                            }
                        }
                        else{
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH*0.75){
                                keyIndex = i;
                                foundKey = true;
                            }
                        }
                    }
                }
            }
            //*masterInput.key = keyIndex;
            processSpiInput(masterInput.keyPointer);
            processSpiInput(keyIndex);
            if(keyIndex != keySelection){
                /*
                printf("SPI COMMAND\n");
                printf("00000001 (Key Selection)\n");
                printf("%d\n", keyIndex);
                */
                spi_buffer[0] = 129;
                spi_buffer[1] = keyIndex;
                //wiringPiSPIDataRW(CHANNEL, spi_buffer, 2);
                keySelection = keyIndex;
            }
            keys[keyIndex].pressed = true;
        }
        else{
            clearKeyPress();
            //check if slider is selected
            for(int i = 0; i < NUM_SLIDERS; i++){
                Slider tempSlider = sliders[i];
                if(masterInput.x > tempSlider.xPos && masterInput.x -tempSlider.xPos < SLIDER_WIDTH && masterInput.y > tempSlider.yPos && masterInput.y -tempSlider.yPos < SLIDER_HEIGHT){
                    tempSlider.value = (float)(tempSlider.yPos + SLIDER_HEIGHT - masterInput.y)/SLIDER_HEIGHT;
                    if(tempSlider.value < 0.05) tempSlider.value = 0;
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
        for(int i = 0; i < NUM_BUTTONS; i++){
            //buttons[i].keyPressed = false;
        }
    }
}


void main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Synth");
    SetTargetFPS(60);
    InitAudioDevice();
    initMasterInput();
    //initSynth(&synth);
    buildKeys();
    buildSliders();
    buildButtons();
    SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
    AudioStream synthStream = LoadAudioStream(SAMPLE_RATE,
        32 ,
        1
    );
    SetAudioStreamVolume(synthStream, 0.25f);
    PlayAudioStream(synthStream);
    
    //spi config
    int fd, result;

    //cout << "Initializing" << endl ;

    // Configure the interface.
    // CHANNEL insicates chip select,
    // 50000 indicates bus speed.
    fd = wiringPiSPISetup(CHANNEL, 500000);

    //cout << "Init result: " << fd << endl;

    // clear display
	
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

    while(WindowShouldClose() == false)
    {
        if(IsAudioStreamProcessed(synthStream)){
            UpdateAudioStream(synthStream, buffer, STREAM_BUFFER_SIZE);
            processInput();
            //updateSignal(buffer);
            drawGUI();
		    read(seqfd, &midipacket, sizeof(midipacket));
            
            if((firstByte != midipacket[1] || secondByte != midipacket[2]) && midipacket[1] < 109 && midipacket[1] > 23){
                //send gate
                processSpiInput(SPI_MODULE_ENV | SPI_GATE);
                if(midipacket[2] > 0) processSpiInput(1);
                else processSpiInput(0);
                //send key
                processSpiInput(SPI_MODULE_KEYBOARD | SPI_KEYBOARD_KEY);
                processSpiInput(midipacket[1] - 24);
                firstByte = midipacket[1];
                secondByte = midipacket[2];
            }
            
        }
    }
    CloseAudioDevice();
    CloseWindow();
}

