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
#include "include/db.h"
#include "sqlite/sqlite3.h"
// PORT AUDIO
// #include "alsa/error.h"
// #include "portaudio.h"

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
#define NUM_BUTTONS 8
#define MIDI_DEVICE1 "/dev/midi1"
#define MIDI_DEVICE2 "/dev/midi2"
#define SYNTH_MODE 0
#define EQ_MODE 1
#define WAV_MODE 2
#define NUM_EQ_BANDS 10
#define NUM_CONFIGS 10

#define NUM_OSCILLATORS 5

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
GUI_Section guiSections[5];
int currentBand = 0;
unsigned char spi_buffer[100];

Synth synth;
SpiHandler spiHandler;
Sound wavSound;

// Static Database 
sqlite3* dbDisco;
int configPointer = 1; // 1 <= configPOinter <= numConfigs
int numConfigs;
char* configNames[NUM_CONFIGS+1]= {"DEBUG","Conf1","Conf2","Conf3","Conf4","Conf5","Conf6","Conf7","Conf8","Conf9","Conf10"}; 

// File handling
GuiFileDialogState fileDialogState;
char* wavDirectory = "/home/pi/GatorDisco/disco_server/fileshare/media/wavs/";
char fileNameToLoad[512] = { 0 };
Texture texture = { 1 };

int screenshotTarget = 0;
bool screenshotNeeded = false;


void processSpiInput(int byte){
    //printf("sent byte: %d\n", byte);
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
drawConfigDisplay(){
    DrawRectangle(1050,50,SCREEN_WIDTH/8,SCREEN_HEIGHT/10,WHITE);
    DrawText(configNames[configPointer],1070,80,20,RED);
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
    tempSection.title = "";
    tempSection.noTitle = true;
    tempSection.xPos = SCREEN_WIDTH*3/4;
    tempSection.yPos = 0;
    tempSection.height = SCREEN_HEIGHT*3/4;
    tempSection.width = SCREEN_WIDTH/4;
    guiSections[2] = tempSection;
    tempSection.title = "LFO";
    tempSection.xPos = 0;
    tempSection.yPos = SCREEN_HEIGHT*3/8;
    tempSection.height = SCREEN_HEIGHT*3/8;
    tempSection.width = SCREEN_WIDTH*3/8;
    tempSection.noTitle = false;
    guiSections[3] = tempSection;
    tempSection.title = "EFFECTS";
    tempSection.xPos = SCREEN_WIDTH*3/8;
    tempSection.yPos = SCREEN_HEIGHT*3/8;
    tempSection.height = SCREEN_HEIGHT*3/8;
    tempSection.width = SCREEN_WIDTH*3/8;
    tempSection.noTitle = false;
    guiSections[4] = tempSection;
}
void drawGuiSections(){
    for(int i = 0; i < 5; i++){
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


//*************************DB OPERATION***************************************
int openDB(){
    // sqlite3* dbDisco;
    // check connection to DB
    int rc = SQLITE_ERROR; //assume erroneous by default
    
    rc = sqlite3_open("/home/pi/GatorDisco/disco_server/dbspot/gdiscoDb.sqlite3", &dbDisco);
    if (rc != SQLITE_OK){
        printf("Failed to connect to db....code %d\nCode info: https://www.sqlite.org/c3ref/c_abort.html\n", rc);
    }
    else{
        printf("Successfully connected to config DB!\n");
    }
    return rc;
}

void closeDB(){
    sqlite3_close(dbDisco);
}

void saveConfig(void){
    // This funciton will save the 'state' of the synthesizer. 

    // REPLACE DB.at(id=configPointer) with new settings
    // By django's grace, it should replace at the ID location by defualt
    
    openDB();
    sqlite3_stmt *stmt;
    int rc;
    int paramCount;

    //List of VARS
    int ID;
    char* name;
    int octave = (int)(sliders[0].value*127);
    int oscParam1 = (int)(sliders[1].value*127);
    int oscParam2 = (int)(sliders[2].value*127);

    int Attack = (int)(sliders[3].value*127);
    int Decay = (int)(sliders[4].value*127);
    int Sustain = (int)(sliders[5].value*127);
    int Release = (int)(sliders[6].value*127);

    int lfoSpeed = (int)(sliders[7].value*127);
    int lfoval = (int)(sliders[8].value*127);

    int Effect1 = (int)(sliders[9].value*127);
    int Effect2 = (int)(sliders[10].value*127);
    int OscType = oscTypePointer;
    int effectType = effectTypePointer;
    int lfoTarget = lfoTargetPointer;
    
    char* sql = "UPDATE fileshare_configmodel SET octave= ?, oscParam1= ?, oscParam2= ?, lfoSpeed= ?, lfoval= ?, Attack= ?, Decay= ?, Sustain= ?, Release= ?, Effect1= ?, Effect2= ?, OscType= ?, effectType= ?, lfoTarget= ? WHERE id= ?;";
    rc = sqlite3_prepare_v2(dbDisco, sql, -1, &stmt, NULL);
    
    if (rc) { // anything but 0 is failure
       printf("Error preparing sql statement\n");
       printf("Received rc %d\n",rc);
       sqlite3_finalize(stmt);
       sqlite3_close(dbDisco);
    }
    else
    {
        paramCount = sqlite3_bind_parameter_count(stmt);
        printf("The number of parameters in the statement: %d\n", paramCount);

        //*************BINDS***********//    
        rc = sqlite3_bind_int(stmt, 1, octave );
        printf("Octave : %d\n", octave);
        if (rc != SQLITE_OK) {

            fprintf(stderr, "Failed to bind octave: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 2, oscParam1 );
        printf("oscParam1 : %d\n", oscParam1);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind oscParam1: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 3, oscParam2 );
        printf("oscParam2 : %d\n", oscParam2);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind oscParam2: %s\n", sqlite3_errmsg(dbDisco));
        }

        // LFO and params 
        rc = sqlite3_bind_int(stmt, 4, lfoSpeed );
        printf("lfoSpeed : %d\n", lfoSpeed);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind oscParam2: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 5, lfoval );
        printf("lfoval : %d\n", lfoval);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind oscParam2: %s\n", sqlite3_errmsg(dbDisco));
        }

        //ADSR
        rc = sqlite3_bind_int(stmt, 6, Attack );
        printf("Attack : %d\n", Attack);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind Attack: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 7, Decay );
        printf("Decay : %d\n", Decay);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind Decay: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 8, Sustain );
        printf("Sustain : %d\n", Sustain);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind Sustain: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 9, Release );
        printf("Release : %d\n", Release);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind Release: %s\n", sqlite3_errmsg(dbDisco));
        }

        // Effects 1 and 2
        rc = sqlite3_bind_int(stmt, 10, Effect1 );
        printf("Effect1 : %d\n", Effect1);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind Effect1: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 11, Effect2 );
        printf("Effect2 : %d\n", Effect2);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind Effect2: %s\n", sqlite3_errmsg(dbDisco));
        }
        
        // Type pointers
        rc = sqlite3_bind_int(stmt, 12, OscType);
        printf("OscType : %d\n", OscType);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind OscType: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 13, effectType);
        printf("effectType : %d\n", effectType);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind effectType: %s\n", sqlite3_errmsg(dbDisco));
        }
        rc = sqlite3_bind_int(stmt, 14, lfoTarget);
        printf("lfoTarget : %d\n", lfoTarget);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind lfoTarget: %s\n", sqlite3_errmsg(dbDisco));
        }
        //******************************/

        // WHERE
        rc = sqlite3_bind_int(stmt, 15, configPointer);
        printf("configPointer : %d\n", configPointer);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind configPointer: %s\n", sqlite3_errmsg(dbDisco));
        }

        char *sentSQL = sqlite3_expanded_sql(stmt);
        printf("Sent SQL: %s\n",sentSQL);
        
        // Do the REPLACMENT:
        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Cannot step from statement: %d ; %s\n", rc, sqlite3_errmsg(dbDisco));
            sqlite3_close(dbDisco);
        }
        sqlite3_reset(stmt);
        rc = sqlite3_finalize(stmt);
        sqlite3_close(dbDisco);
    }
  
    // Lastly take a screenshot with <config_name>.png and save to the same directory
    // TakeScreenshot("../../disco_server/MEDIA/config.png"); 
    // Taking a screenshot here may lead to incomplete capture (usually just the background)
    // TESTING: setting a boolean to true and checking it at after endDrawing()
    // screenshotNeeded = true;
}

int setNumConfigs(){ //returns the number of configurations
    // sqlite3* dbDisco;
    openDB();
    sqlite3_stmt* stmt;
    int rc;
    char *err;
    int nByte = -1; //don't care
    int count;
    
    rc = sqlite3_prepare_v2(dbDisco, "SELECT COUNT(*) FROM fileshare_configmodel;", nByte, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to connect to db....code %d\nCode info: https://www.sqlite.org/rescode.html\n", rc);
        return 0;
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        printf("no configs found");
        return 0;
    }
    count = sqlite3_column_int(stmt, 0);
    numConfigs = count;
    
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    closeDB();
    return count;
}

void loadConfig(int dir){

    // openDB();
    sqlite3_stmt* stmt;
    int rc;
    char *err;
    int baselen;
    int OFFSET=0;
    int nConfigs;
    int id = configPointer;

    char* name;
    int oscP, lfoP, effP = 0; 
    
    //logic for setting index    
    nConfigs = setNumConfigs(); // set numConfigs
    openDB();

    // The DB is empty
    if (nConfigs == 0){
        // Load values of 0
        printf("No configurations found\n");
        return;
    }

    // Moving to the NEXT config
    else if (dir == 1){
        // Make sure there is a next row
        if (configPointer < nConfigs){ // Move up in the middle of the table
            id = configPointer + 1; // Offset will point the query to the NEXT ENTRY 
        }

        else{ // We are at the last row, or there is only 1 row anyways. Loop back to start
           id = 1; // to get the first row 
        } 
    }
    else if (dir == 0) {
        // Make sure there is a prev row
        if (configPointer > 1){ // Move down in the middle of the table
            id = configPointer-1;
        }
        else{ // move "down" (n-1) at the bottom of the table
            id = nConfigs;
        } 
    }
    printf("Found %d configs\n",nConfigs);
    printf("Currently at config %d\n",configPointer);
    printf("Moving in dir %d\n", dir);
    printf("Target ID is %d\n",id);

    char* sql = "SELECT * FROM fileshare_configmodel WHERE id= ? ";

    rc = sqlite3_prepare_v2(dbDisco, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(dbDisco));
        exit(-1);
    }

    rc = sqlite3_bind_int(stmt, 1, id); // The while loop will hopefully handle the first step
    // rc = sqlite3_step(stmt);
    sqlite3_bind_int(stmt, 1, id); // The while loop will hopefully handle the first step
    while (sqlite3_step(stmt) != SQLITE_DONE){

        configPointer = sqlite3_column_int(stmt,0); // Use index as the pointer
        printf("New config pointer is %d\n", configPointer);
        
        name = sqlite3_column_text(stmt,1);
        printf("Loading config: %s\n", name);
        
        // OSC params
        sliders[0].value = (float)sqlite3_column_int(stmt,2)/127;
        sliders[1].value = (float)sqlite3_column_int(stmt,3)/127;
        sliders[2].value = (float)sqlite3_column_int(stmt,4)/127;
        // LFO params
        sliders[7].value = (float)sqlite3_column_int(stmt,5)/127;
        sliders[8].value = (float)sqlite3_column_int(stmt,6)/127;
        //ADSR
        sliders[3].value = (float)sqlite3_column_int(stmt,7)/127; //atk
        sliders[4].value = (float)sqlite3_column_int(stmt,8)/127; //decay
        sliders[5].value = (float)sqlite3_column_int(stmt,9)/127; //sustain
        sliders[6].value = (float)sqlite3_column_int(stmt,10)/127; //release
        
        //Effects 1 and 2
        sliders[9].value     = (float)sqlite3_column_int(stmt,11)/127;
        sliders[10].value    = (float)sqlite3_column_int(stmt,12)/127;
        
        // Type pointers
        oscP    = sqlite3_column_int(stmt,15);
        changeOsc(oscP);
        effP  = sqlite3_column_int(stmt,13);
        changeEffect(effP);
        lfoP   = sqlite3_column_int(stmt,14);
        changeLfo(lfoP);
    }
    // Chek loaded values
    for (int i = 0; i< NUM_SLIDERS; i++){
        printf("Slider %d has value %f.\n",i,sliders[i].value);
    }
    printf("New OSC Pointer: %d\n", oscP);
    printf("New effP Pointer: %d\n", effP);
    printf("New lfoP Pointer: %d\n", lfoP);

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    closeDB();
}

void changeOsc(int p){
    if (p == -1){
        oscTypePointer++;
    }
    else{
        oscTypePointer = p;
    }
    oscTypePointer %= NUM_OSCILLATORS;
    processSpiInput(SPI_MODULE_OSC | SPI_OSCTYPE);
    processSpiInput(oscTypePointer);
    buttons[6].text = oscNames[oscTypePointer];
    sliders[1].name = oscParam1Names[oscTypePointer];
    sliders[2].name = oscParam2Names[oscTypePointer];
    if(oscTypePointer == 2){
        loadWavSound("wavs/piano.wav",36);
    }
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
        buttons[4].xPos = 1050;
        buttons[4].yPos = 450;
    }
}
void changeLfo(int p){
    if (p == -1){
        lfoTargetPointer++;
    }
    else{
        lfoTargetPointer = p;
    }
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

// change implemented. Allow manual input of effectTypePointer in order to enable loading configs
void changeEffect(int p){
    if (p == -1) {
        effectTypePointer++;
    }
    else{
        effectTypePointer = p;
    }    
    effectTypePointer %= NUM_EFFECTS;
    processSpiInput(SPI_MODULE_FX | SPI_FX_SEL);
    processSpiInput(effectTypePointer);
    buttons[3].text = effectNames[effectTypePointer];
    sliders[9].name = effectParam1Names[effectTypePointer];
    sliders[10].name = effectParam2Names[effectTypePointer];
}
void buildButtons(){
    Button load_config;
    load_config.xPos = 1050;
    load_config.yPos = 150;
    load_config.width = SCREEN_WIDTH/16;
    load_config.height = SCREEN_HEIGHT/10;
    load_config.color = BLACK;
    load_config.text = "<";
    load_config.buttonAction = &loadConfig;
    buttons[0] = load_config;
    Button load_config2;
    load_config2.xPos = 1050 + SCREEN_WIDTH/16;
    load_config2.yPos = 150;
    load_config2.width = SCREEN_WIDTH/16;
    load_config2.height = SCREEN_HEIGHT/10;
    load_config2.color = BLACK;
    load_config2.text = ">";
    load_config2.buttonAction = &loadConfig;
    buttons[1] = load_config2;
    Button oscSelect;
    oscSelect.xPos = 40;
    oscSelect.yPos = SCREEN_HEIGHT/5;
    oscSelect.width = SCREEN_WIDTH/10;
    oscSelect.height = SCREEN_HEIGHT/12;
    oscSelect.color = GREEN;
    oscSelect.text = "PULSE WAVE";
    oscSelect.buttonAction = &changeOsc;
    buttons[6] = oscSelect;
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
    effectSelect.xPos = 550;
    effectSelect.yPos = SCREEN_HEIGHT/2;
    effectSelect.width = SCREEN_WIDTH/10;
    effectSelect.height = SCREEN_HEIGHT/12;
    effectSelect.color = GREEN;
    effectSelect.text = "OFF";
    effectSelect.buttonAction = &changeEffect;
    buttons[3] = effectSelect;
    Button eqMode;
    eqMode.xPos = 1050;
    eqMode.yPos = 450;
    eqMode.width = SCREEN_WIDTH/8;
    eqMode.height = SCREEN_HEIGHT/10;
    eqMode.color = GREEN;
    eqMode.text = "EQ MODE";
    eqMode.buttonAction = &changeMode;
    buttons[4] = eqMode;
    // Save Config
    Button save_config;
    save_config.xPos = 1050;
    save_config.yPos = 250;
    save_config.width = SCREEN_WIDTH/8;
    save_config.height = SCREEN_HEIGHT/10;
    save_config.color = BLACK;
    save_config.text = "SAVE CONFIG";
    save_config.buttonAction = &saveConfig;
    buttons[5] = save_config;
    Button wavSelect;
    wavSelect.xPos = 1050;
    wavSelect.yPos = 350;
    wavSelect.width = SCREEN_WIDTH/8;
    wavSelect.height = SCREEN_HEIGHT/10;
    wavSelect.color = BLACK;
    wavSelect.text = "WAV SELECT";
    wavSelect.buttonAction = &saveConfig;
    buttons[7] = wavSelect;

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
    Effect1.xPos = 710;
    Effect1.yPos = 380;
    Effect1.value = 0;
    Effect1.param = SPI_MODULE_FX | SPI_FX_PARAM1;
    Effect1.name = "";
    sliders[9] = Effect1;
    Slider Effect2;
    Effect2.xPos = 830;
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
            if (!strcmp(tempButton.text, "WAV SELECT"))
            {   
                fileDialogState.fileDialogActive = true;
                GUI_MODE = WAV_MODE;

            }
            else if (!strcmp(tempButton.text, ">"))
            {
                tempButton.buttonAction(1);
            }
            else if (!strcmp(tempButton.text, "<"))
            {
                tempButton.buttonAction(0);
            }
            else
            {
                tempButton.buttonAction(-1);
            }
        }
        // GUI: Dialog Window
        //--------------------------------------------------------------------------------
        //GuiFileDialog(&fileDialogState);
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
    if(oscTypePointer == 0) ClearBackground(LIME);
    else if (oscTypePointer == 1) ClearBackground(ORANGE);
    else if (oscTypePointer == 2){
        int red = (int)random() % 256;
        int green = (int)random() % 256;
        int blue = (int)random() % 256;
        ClearBackground((Color){red,green,blue,255});
    }
    else if (oscTypePointer == 3) ClearBackground(SKYBLUE);
    else if (oscTypePointer == 4) ClearBackground(MAGENTA);

    if(GUI_MODE == SYNTH_MODE){
        //drawWaveform(buffer,SCREEN_WIDTH*3/16,SCREEN_HEIGHT*7/32,SCREEN_WIDTH*25/32,80);
        drawConfigDisplay();
        drawKeys(SCREEN_HEIGHT/4);
        drawSliders();
        drawButtons();
        drawGuiSections();
        // drawFileMenu();
    }
    else if(GUI_MODE == EQ_MODE){
        drawEQButtons();
        drawEQSliders();
        // drawFileMenu();
    }
    else{
        if(!fileDialogState.fileDialogActive)
        GUI_MODE = SYNTH_MODE;
        GuiFileDialog(&fileDialogState);
    }
    EndDrawing();
    if (screenshotNeeded){

        char screenPath[100];
        strcpy(screenPath,"../../disco_server/media/");
        
        strcat(screenPath,configNames[configPointer]);
    
        TakeScreenshot(screenPath); 
        screenshotNeeded = false;
    }
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
        processSpiInput(SPI_MODULE_ENV | SPI_GATE);
        processSpiInput(0);
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
                masterInput.keyPressed = true;
                PlaySound(wavSound);
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
                                    //processSpiInput(SPI_MODULE_ENV | SPI_GATE);
                                    //processSpiInput(0);
                                    // processSpiInput(masterInput.keyPointer);
                                    // processSpiInput(masterInput.keyIndex);
                                    // processSpiInput(0);
                                    masterInput.keyIndex = i +12*octave;
                                }
                                foundKey = true;
                            }
                        }
                        else{
                            if(masterInput.x > tempKey.xPos && masterInput.x < tempKey.xPos + WHITE_KEY_WIDTH*0.75){
                                if(masterInput.keyIndex != i + 12*octave){
                                    //processSpiInput(SPI_MODULE_ENV | SPI_GATE);
                                    //processSpiInput(0);
                                    // processSpiInput(masterInput.keyPointer);
                                    // processSpiInput(masterInput.keyIndex);
                                    // processSpiInput(0);
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
            processSpiInput(SPI_MODULE_ENV | SPI_GATE);
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

// typedef struct
// {
//     float left_phase;
//     float right_phase;
// }   
// paTestData;

// static int patestCallback( const void *inputBuffer, void *outputBuffer,
//                            unsigned long framesPerBuffer,
//                            const PaStreamCallbackTimeInfo* timeInfo,
//                            PaStreamCallbackFlags statusFlags,
//                            void *userData )
// {
//     /* Cast data passed through stream to our structure. */
//     paTestData *data = (paTestData*)userData; 
//     float *out = (float*)outputBuffer;
//     unsigned int i;
//     (void) inputBuffer; /* Prevent unused variable warning. */
    
//     for( i=0; i<framesPerBuffer; i++ )
//     {
//          *out = data->left_phase;  /* left */
//          out++;
//          *out = data->right_phase;  /* right */
//          out++;
//         /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
//         data->left_phase += 0.01f;
//         /* When signal reaches top, drop back down. */
//         if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
//         /* higher pitch so we can distinguish left and right. */
//         data->right_phase += 0.03f;
//         if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
//     }
//     return 0;
// }

void main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Synth");
    SetTargetFPS(60);
    InitAudioDevice();
    initMasterInput();
    loadWavSound("wavs/piano.wav",36);
    buildKeys();
    buildSliders();
    buildBandGUIs();
    buildButtons();
    buildEQButtons();
    buildGuiSections();
    
    //spi config
    int fd, result;
    // CHANNEL insicates chip select,
    // 50000 indicates bus speed.
    fd = wiringPiSPISetup(CHANNEL, 500000);
    // clear display
	spi_buffer[0] = 0x76;
    wiringPiSPIDataRW(CHANNEL, spi_buffer, 1);

    unsigned char midipacket[8];
	//open midi device
    int seqfd = open(MIDI_DEVICE1, O_RDONLY);
    if (seqfd == -1) {
        seqfd = open(MIDI_DEVICE2, O_RDONLY);
        if(seqfd == -1){
            printf("Error: cannot open %s\n", MIDI_DEVICE1);
            // exit(1);
        }
    }

	
    fileDialogState = InitGuiFileDialog(3*SCREEN_HEIGHT/4, 3*SCREEN_HEIGHT/4, wavDirectory, false);
    // Choose an extenstion to filter by
    char* filterExt = ".wav";
    // strcpy(fileDialogState.filterExt,filterExt);

    // char* filterExt = ".wav";
    // strcpy(fileDialogState.filterExt,filterExt);
    
    //   /******************************/
    //  /***   port audio config   ****/
    // /******************************/
    
    // //initialize function
    // PaError err = Pa_Initialize();
    // if( err != paNoError ){
    //     printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    //     Pa_Terminate();
    //     exit(1);
    // }
    // static paTestData data;
    // PaStream *stream;
    // /* Open an audio I/O stream. */
    // err = Pa_OpenDefaultStream( &stream,
    //                             0,          /* no input channels */
    //                             2,          /* stereo output */
    //                             paFloat32,  /* 32 bit floating point output */
    //                             SAMPLE_RATE,
    //                             256,        /* frames per buffer, i.e. the number
    //                                                of sample frames that PortAudio will
    //                                                request from the callback. Many apps
    //                                                may want to use
    //                                                paFramesPerBufferUnspecified, which
    //                                                tells PortAudio to pick the best,
    //                                                possibly changing, buffer size.*/
    //                             patestCallback, /* this is your callback function */
    //                             &data ); /*This is a pointer that will be passed to
    //                                                your callback*/
    // if( err != paNoError ){
    //     printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    //     Pa_Terminate();
    //     exit(1);
    // }

    // err = Pa_StartStream( stream );
    // if( err != paNoError ){
    //     printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    //     Pa_Terminate();
    //     exit(1);
    // }
    /****   main loop   ****/

    while(WindowShouldClose() == false)
    {            
        if (fileDialogState.fileDialogActive) GuiLock();

        /// FILE BROWSER GUI  ////////////
        if (fileDialogState.SelectFilePressed)
        {
            // Load image file (if supported extension)
            if (IsFileExtension(fileDialogState.fileNameText, ".wav"))
            {
                printf("Dir path text: %s\n", fileDialogState.dirPathText);
                printf("File Name: %s\n", fileDialogState.fileNameText);

                strcpy(fileNameToLoad, TextFormat("%s/%s", fileDialogState.dirPathText, fileDialogState.fileNameText));
                printf("%s\n",fileNameToLoad);
                // loadWavSound(fileNameToLoad, 36);
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
            
            for(int i = 0; i < 6; i++){
                if(midipacket[i] == 144){
                    printf("byte 0: %d, byte 1: %d, byte 2: %d\n", midipacket[i], midipacket[i+1], midipacket[i+2]);
                    processSpiInput(masterInput.keyPointer);
                    processSpiInput(midipacket[i+1]-24);
                    processSpiInput(SPI_MODULE_ENV | SPI_GATE);
                    processSpiInput(midipacket[i+2]);
                    if(midipacket[i+2] != 0) PlaySound(wavSound);
                }
            }
        //}
    }
    UnloadTexture(texture);     // Unload texture
    // Pa_StopStream(stream);
    // Pa_Terminate();
    CloseAudioDevice();
    CloseWindow();
}

