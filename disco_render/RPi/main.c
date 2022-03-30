#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "include/raygui.h"
#define GUI_FILE_DIALOG_IMPLEMENTATION
#include "include/gui_file_dialog.h"
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
#define NUM_SLIDERS 11
#define NUM_BUTTONS 6
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
    int (*buttonAction)(int input);
} Button;
typedef struct{
    int x;
    int y;
    int gatePointer;
    int keyPointer;
    bool keyPressed;
} Input;
typedef struct{
    float fCenter;
    float gain;
    float qFactor;
    float upperLimit;
    float lowerLimit;
} BandGUI;
char* oscNames[NUM_OSCILLATORS] = {"PULSE WAVE", "SAWTOOTH", "WAV FILE", "OSCILLATOR 4"};
char* oscParam1Names[NUM_OSCILLATORS] = {"PULSE WIDTH", "DETUNE", "OSC3 PARAM", "OSC4 PARAM"};
char* oscParam2Names[NUM_OSCILLATORS] = {"", "", "OSC3 PARAM2", "OSC4 PARAM2"};
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
int currentBand = 0;
unsigned char spi_buffer[100];

Synth synth;
SpiHandler spiHandler;
Sound wavSound;

// File handling
GuiFileDialogState fileDialogState;
char* configDirectory = "/home/pi/GatorDisco/disco_server/MEDIA/";
char fileNameToLoad[512] = { 0 };
Texture texture = { 1 };


void processSpiInput(int byte){
    printf("sent byte: %d\n", byte);
    spi_buffer[0] = byte;
    wiringPiSPIDataRW(CHANNEL, spi_buffer, 1);
}
void loadWavSound(char* fileName, int key){
    wavSound = LoadSound(fileName);
    SetSoundVolume(wavSound,1);
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

void saveConfig(void){
    // This funciton will save the 'state' of the synthesizer. 
    // Reads:
    //  - Active effects and any assoc. slider values
    //  - All other slider values
    // Writes:
    //  - All read data to a file given a random name
    //      - Considering random selection from a noun dictionary and a verb dictionary
    // - TO configDirectory

    // First, read consistent parameters sliders := {ADSR, OCTAVE}
    int configData[NUM_SLIDERS+3]; // Plus 3 since there is also oscType, effType, and lfoTarget
    // int osc_pointer, fx_pointer, lfo_pointer;

    //This can all probably be optimized to not include copying, but it's tiny data anyways and I want debugging to be easy
    for (u_int8_t i = 0; i < NUM_SLIDERS; i++)
    {
        configData[i] = (int)(sliders[i].value*127);
        // printf( "Read slider FLOAT value %f\n",
        //         (sliders[i].value) );
        // printf( "Saved slider config INT value %d\n",
        //         configData[i]);
    }
    configData[NUM_SLIDERS]   = oscTypePointer;
    configData[NUM_SLIDERS+1] = effectTypePointer;
    configData[NUM_SLIDERS+2] = lfoTargetPointer;
    
    // TODO: Fix segmentation fault caused by string chenanigans
    // char* confPath = ""; // Strictly the path to cconfig directory
    // char* confName = "config1.gat"; // The name of the actual configuration file 
    // strcpy(confPath,configDirectory); // need a new confPath variable so that unique config names can be appended with strcat(confPath,<name>)
    // strcat(confPath,confName);
    
    FILE* confPtr;
    confPtr = fopen("/home/pi/GatorDisco/disco_server/MEDIA/config.gat","wb"); //WRITE Binary
    // confPtr = fopen(confPath,"wb"); //Replace with this once string induced segmentation fault stops 
    
    if (! confPtr)
        printf("Failed to save config file\n");
    else
    {
        fwrite(configData, sizeof(int), sizeof(configData), confPtr);
        fclose(confPtr);
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
    sliders[1].name = oscParam1Names[oscTypePointer];
    sliders[2].name = oscParam2Names[oscTypePointer];
    if(oscTypePointer == 2){
        loadWavSound("piano.wav",36);
    }
}
void changeMode(int input){
    if(GUI_MODE == SYNTH_MODE){
        GUI_MODE = EQ_MODE;
        buttons[4].text = "SYNTH MODE";
    }
    else{
        GUI_MODE = SYNTH_MODE;
        buttons[4].text = "EQ MODE";
    }
}
void changeLfo(void){
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
    sliders[8].name = effectParam1Names[effectTypePointer];
    sliders[9].name = effectParam2Names[effectTypePointer];
}
void buildButtons(){
    Button load_config;
    load_config.xPos = (7*SCREEN_WIDTH/10);
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
    Button eqMode;
    eqMode.xPos = (SCREEN_WIDTH/32);
    eqMode.yPos = SCREEN_HEIGHT/10;
    eqMode.width = SCREEN_WIDTH/8;
    eqMode.height = SCREEN_HEIGHT/12;
    eqMode.color = GREEN;
    eqMode.text = "EQ MODE";
    eqMode.buttonAction = &changeMode;
    buttons[4] = eqMode;
    // Save Config
    Button save_config;
    save_config.xPos = (7*SCREEN_WIDTH/10); //(3*SCREEN_WIDTH/5)
    save_config.yPos = (SCREEN_HEIGHT/3)+SCREEN_HEIGHT/12;
    save_config.width = SCREEN_WIDTH/5;
    save_config.height = SCREEN_HEIGHT/12;
    save_config.color = BLACK;
    save_config.text = "SAVE CONFIG";
    save_config.buttonAction = &saveConfig;
    buttons[5] = save_config;

}
void buildBandGUIs(){
    for(int i = 0; i < NUM_EQ_BANDS; i++){
        BandGUI tempBand;
        //calculate slider values
        tempBand.fCenter = fCenters[i];
        tempBand.gain = 0;
        tempBand.qFactor = 1.414;
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
    oscParam1.name = "PULSE WIDTH";
    sliders[1] = oscParam1;
    Slider oscParam2;
    oscParam2.xPos = 420;
    oscParam2.yPos = 100;
    oscParam2.value = 0;
    oscParam2.param = SPI_MODULE_OSC | SPI_OSCPARAM2;
    oscParam2.name = "";
    sliders[2] = oscParam2;
    Slider lfoSpeed;
    lfoSpeed.xPos = 570;
    lfoSpeed.yPos = 100;
    lfoSpeed.value = 0;
    lfoSpeed.param = SPI_MODULE_LFO | SPI_LFO_SPEED;
    lfoSpeed.name = "LFO Freq";
    sliders[3] = lfoSpeed;
    Slider lfoval;
    lfoval.xPos = 690;
    lfoval.yPos = 100;
    lfoval.value = 0;
    lfoval.param = SPI_MODULE_LFO | SPI_LFO_VAL;
    lfoval.name = "LFO Val";
    sliders[4] = lfoval;
    Slider Attack;
    Attack.xPos = 200;
    Attack.yPos = 350;
    Attack.value = 0;
    Attack.param = SPI_MODULE_ENV | SPI_ENV_ATTACK;
    Attack.name = "Attack";
    sliders[5] = Attack;
    Slider Decay;
    Decay.xPos = 300;
    Decay.yPos = 350;
    Decay.value = 0;
    Decay.param = SPI_MODULE_ENV | SPI_ENV_DECAY;
    Decay.name = "Decay";
    sliders[6] = Decay;
    Slider Sustain;
    Sustain.xPos = 450;
    Sustain.yPos = 350;
    Sustain.value = 0;
    Sustain.param = SPI_MODULE_ENV | SPI_ENV_SUSTAIN;
    Sustain.name = "Sustain";
    sliders[7] = Sustain;
    Slider Release;
    Release.xPos = 600;
    Release.yPos = 350;
    Release.value = 0;
    Release.param = SPI_MODULE_ENV | SPI_ENV_RELEASE;
    Release.name = "Release";
    sliders[8] = Release;
    Slider Effect1;
    Effect1.xPos = 750;
    Effect1.yPos = 350;
    Effect1.value = 0;
    Effect1.param = SPI_MODULE_FX | SPI_FX_PARAM1;
    Effect1.name = "";
    sliders[9] = Effect1;
    Slider Effect2;
    Effect2.xPos = 900;
    Effect2.yPos = 350;
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

    DrawText(fileNameToLoad, 208, GetScreenHeight() - 20, 10, GRAY);

    for(int i = 0; i < NUM_BUTTONS; i++){
        // raygui: controls drawing
        //----------------------------------------------------------------------------------
        

        Button tempButton = buttons[i];
        bool pressed = GuiButton((Rectangle){
        tempButton.xPos,
        tempButton.yPos,
        tempButton.width,
        tempButton.height
        }, tempButton.text);
        if(pressed){
            if (!strcmp(tempButton.text, "LOAD CONFIG"))
            {   
                fileDialogState.fileDialogActive = true;

            }
            else
            {
                tempButton.buttonAction(0);
            }

        }

        // GUI: Dialog Window
        //--------------------------------------------------------------------------------
        GuiFileDialog(&fileDialogState);
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

// void drawFileMenu(){

//     DrawText(fileNameToLoad, 208, GetScreenHeight() - 20, 10, GRAY);

//     // raygui: controls drawing
//     //----------------------------------------------------------------------------------
//     if (fileDialogState.fileDialogActive) GuiLock();

//     // if (GuiButton((Rectangle){ 20, 20, 140, 30 }, GuiIconText(RAYGUI_ICON_FILE_OPEN, "Open Image"))) fileDialogState.fileDialogActive = true;
//     if (buttons[0]) 
//         fileDialogState.fileDialogActive = true;

//     GuiUnlock();

//     // GUI: Dialog Window
//     //--------------------------------------------------------------------------------
//     GuiFileDialog(&fileDialogState);
// }

void drawGUI(){
    BeginDrawing();
    ClearBackground(GRAY);

    if(GUI_MODE == SYNTH_MODE){
        drawWaveform(buffer,SCREEN_WIDTH/6,SCREEN_HEIGHT/6,SCREEN_WIDTH-(SCREEN_WIDTH*1.5/6),SCREEN_HEIGHT/12);
        drawKeys(SCREEN_HEIGHT/4);
        drawSliders();
        drawButtons();
        // drawFileMenu();
    }
    else{
        drawEQButtons();
        drawEQSliders();
        // drawFileMenu();
    }
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
                PlaySound(wavSound);
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
    loadWavSound("piano.wav",36);
    buildKeys();
    buildSliders();
    buildBandGUIs();
    buildButtons();
    buildEQButtons();
    /*
    SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
    AudioStream synthStream = LoadAudioStream(SAMPLE_RATE,
        32 ,
        1
    );
    SetAudioStreamVolume(synthStream, 0.25f);
    PlayAudioStream(synthStream);
    */
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
                // exit(1);
        }
	
    fileDialogState = InitGuiFileDialog(3*SCREEN_HEIGHT/4, 3*SCREEN_HEIGHT/4, configDirectory, false);
    // Choose an extenstion to filter by
    char* filterExt = ".bin";
    strcpy(fileDialogState.filterExt,filterExt);

    while(WindowShouldClose() == false)
    {            
        if (fileDialogState.fileDialogActive) GuiLock();

        /// FILE BROWSER GUI  ////////////
        if (fileDialogState.SelectFilePressed)
        {
            // Load image file (if supported extension)
            if (IsFileExtension(fileDialogState.fileNameText, filterExt))
            {
                strcpy(fileNameToLoad, TextFormat("%s/%s", fileDialogState.dirPathText, fileDialogState.fileNameText));
                printf("%s",fileNameToLoad);
                UnloadTexture(texture);
                texture = LoadTexture(fileNameToLoad);
            }

            fileDialogState.SelectFilePressed = false;
        }
        GuiUnlock();

        //////////////////////////////////

        //---
        //if(IsAudioStreamProcessed(synthStream)){
            //UpdateAudioStream(synthStream, buffer, STREAM_BUFFER_SIZE);
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
            
        //}
    }
    UnloadTexture(texture);     // Unload texture
    CloseAudioDevice();
    CloseWindow();
}

