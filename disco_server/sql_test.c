#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "sqlite/sqlite3.h"


int main(){
    sqlite3* dbTest;
    sqlite3* dbDisco;
    sqlite3_stmt* stmt;
    int rcTest, rcDjango;
    char *err;

    // rcTest = sqlite3_open("./dbspot/test.sqlite3", &dbTest);
    rcDjango = sqlite3_open("./dbspot/gdiscoDb.sqlite3", &dbDisco);
    printf("Received return code %d upon opening Gdiscodb.\n", rcDjango );
    // printf("Received return code %d upon opening testdb.\n", rcTest );

    // rcDjango = sqlite3_exec(dbDisco, "SELECT * from fileshare_configmodel",NULL,NULL,&err);
    // if (rcDjango != SQLITE_OK){
    //     printf("Error: %s",err);
    // }

    sqlite3_prepare_v2(dbDisco, "select name, octave, oscParam1, oscParam2, lfoSpeed, lfoval, Attack, Decay, Sustain, Release, Effect1, Effect2, OscType, effectType, lfoTarget from fileshare_configmodel", -1, &stmt, 0);
    char* name;
    int octave, oscParam1, oscParam2, 
    lfoSpeed,   lfoval, 
    Attack,     Decay,      Sustain,    Release, 
    Effect1,    Effect2, 
    OscType,    effectType, lfoTarget;

    while (sqlite3_step(stmt) != SQLITE_DONE){
        name        = sqlite3_column_text(stmt,0);
        octave      = sqlite3_column_int(stmt,1);
        oscParam1   = sqlite3_column_int(stmt,2);
        oscParam2   = sqlite3_column_int(stmt,3);
        lfoSpeed    = sqlite3_column_int(stmt,4);
        lfoval      = sqlite3_column_int(stmt,5);
        Attack      = sqlite3_column_int(stmt,6);
        Decay       = sqlite3_column_int(stmt,7);
        Sustain     = sqlite3_column_int(stmt,8);
        Release     = sqlite3_column_int(stmt,9);
        Effect1     = sqlite3_column_int(stmt,10);
        Effect2     = sqlite3_column_int(stmt,11);
        OscType     = sqlite3_column_int(stmt,12);
        effectType  = sqlite3_column_int(stmt,13);
        lfoTarget   = sqlite3_column_int(stmt,14);
        // printf("Config name: %s\n", name);
        // printf("Config octave: %d\n", octave);
        // printf("Config oscParam1: %d\n", oscParam1);
        // printf("Config oscParam2: %d\n", oscParam2);
        // printf("Config lfoSpeed: %d\n", lfoSpeed);
        // printf("Config lfoval: %d\n", lfoval);
        // printf("Config Attack: %d\n", Attack);
        // printf("Config Decay: %d\n", Decay);
        // printf("Config Sustain: %d\n", Sustain);
        // printf("Config Release: %d\n", Release);
        // printf("Config Effect1: %d\n", Effect1);
        // printf("Config Effect2: %d\n", Effect2);
        // printf("Config OscType: %d\n", OscType);
        // printf("Config effectType: %d\n", effectType);
        // printf("Config lfoTarget: %d\n", lfoTarget);


    }
}