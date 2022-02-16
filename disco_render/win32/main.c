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
#define NUM_SLIDERS 8
#define NUM_BUTTONS 2
#define NUM_OSCILLATORS 2
#define MIDI_DEVICE "/dev/midi2"

typedef struct{
    bool black;
    float frequency;
    bool pressed;
    int xPos;
} Key;
typedef struct{
    float value;
    int xPos;
    int yPos;
    char* name;
    float* param;
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
    bool* keyPressed;
    int* key;
} Input;

Key keys[17];
char* oscNames[NUM_OSCILLATORS] = {"PULSE WAVE", "SAWTOOTH"};
Slider sliders[NUM_SLIDERS];
Button buttons[NUM_BUTTONS];
unsigned char spi_buffer[100];

Synth synth;

Input masterInput;
void initMasterInput(){
    masterInput.key = &synth.keys.key;
    masterInput.keyPressed = &synth.env.gate;
}
float buffer[STREAM_BUFFER_SIZE];
int keySelection = -1;

void updateSignal(float* signal){
    for(int i = 0; i < STREAM_BUFFER_SIZE; i++){
        signal[i] = updateSynth(&synth);
    }
}
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
    synth.osc.oscType++;
    synth.osc.oscType %= NUM_OSCILLATORS;
    buttons[1].text = oscNames[synth.osc.oscType];
    if(synth.osc.oscType == 0){
        sliders[1].name = "PULSE WIDTH";
        sliders[2].name = "PWM Freq";
        sliders[3].name = "PWM Val";
    }
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

}
void buildSliders(){
    Slider octave;
    octave.xPos = 200;
    octave.yPos = 100;
    octave.value = 0;
    octave.param = &synth.keys.octave;
    octave.name = "OCTAVE";
    sliders[0] = octave;
    Slider oscParam1;
    oscParam1.xPos = 300;
    oscParam1.yPos = 100;
    oscParam1.value = 0;
    oscParam1.param = &synth.osc.param1;
    oscParam1.name = "OSC PARAM";
    sliders[1] = oscParam1;
    Slider oscParam2;
    oscParam2.xPos = 450;
    oscParam2.yPos = 100;
    oscParam2.value = 0;
    oscParam2.param = &synth.lfo.speed;
    oscParam2.name = "LFO Freq";
    sliders[2] = oscParam2;
    Slider oscParam3;
    oscParam3.xPos = 600;
    oscParam3.yPos = 100;
    oscParam3.value = 0;
    oscParam3.param = &synth.lfo.val;
    oscParam3.name = "LFO Val";
    sliders[3] = oscParam3;
    Slider Attack;
    Attack.xPos = 200;
    Attack.yPos = 350;
    Attack.value = 0;
    Attack.param = &synth.env.attack;
    Attack.name = "Attack";
    sliders[4] = Attack;
    Slider Decay;
    Decay.xPos = 300;
    Decay.yPos = 350;
    Decay.value = 0;
    Decay.param = &synth.env.decay;
    Decay.name = "Decay";
    sliders[5] = Decay;
    Slider Sustain;
    Sustain.xPos = 450;
    Sustain.yPos = 350;
    Sustain.value = 0;
    Sustain.param = &synth.env.sustain;
    Sustain.name = "Sustain";
    sliders[6] = Sustain;
    Slider Release;
    Release.xPos = 600;
    Release.yPos = 350;
    Release.value = 0;
    Release.param = &synth.env.release;
    Release.name = "Release";
    sliders[7] = Release;
}
void drawSliders(){
    for(int i = 0; i < NUM_SLIDERS; i++){
        Slider tempSlider = sliders[i];
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
int test_button_counter = 0;
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
        if(*masterInput.keyPressed == true){
            /*
            printf("SPI COMMAND\n");
            printf("00000000 (Keypressed)\n");
            printf("00000000\n");
            */
            spi_buffer[0] = 128;
            spi_buffer[1] = 0;
            //wiringPiSPIDataRW(CHANNEL, spi_buffer, 2);
        }
        *masterInput.keyPressed = false;
    }
}
void processInput(){
    masterInput.y = GetMouseY();
    masterInput.x = GetMouseX();

    if(IsMouseButtonDown(0)){
        if(masterInput.y > (3*SCREEN_HEIGHT / 4)){
            if(*masterInput.keyPressed == false){
                /*
                printf("SPI COMMAND\n");
                printf("00000000 (Keypressed)\n");
                printf("00000001\n");
                */
                spi_buffer[0] = 128;
                spi_buffer[1] = 1;
                //wiringPiSPIDataRW(CHANNEL, spi_buffer, 2);
            }
            *masterInput.keyPressed = true;
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
            *masterInput.key = keyIndex;
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
                    *tempSlider.param = tempSlider.value;
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
    initSynth(&synth);
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

