#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include "sqlite/sqlite3.h"

int configPointer = 0;
int numConfigs;
sqlite3* dbDisco;


void closeDB(){
    sqlite3_close(dbDisco);
}

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

int setNumConfigs(){ //returns the number of configurations
    // sqlite3* dbDisco;
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
    return count;
}

void listConfigs(){
    sqlite3_stmt* stmt;
    int rc;
    char *err;
    int nByte = -1; //don't care
    int count;
    int i = 1;
    char* name;
    rc = sqlite3_prepare_v2(dbDisco, "SELECT name FROM fileshare_configmodel;", nByte, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to connect to db....code %d\nCode info: https://www.sqlite.org/rescode.html\n", rc);
    }
    
    while (sqlite3_step(stmt) != SQLITE_DONE) {
        name = sqlite3_column_text(stmt,0);
        printf("%d: %s\n",i,name);
        i += 1;
    }
}

void loadConfig(int dir){
    // Check DB for number of configs
    // if (current_config == num_configs)
    //      configPointer = 1; //Reset
    // else (there is a config after this one)
    //      configPointer = current_config+1
    
    // select <configPointer> from fileshare_configmodel
    // -> store the data from this and set corresponding values
    // Close the db connection 
    sqlite3_stmt* stmt;
    int rc;
    char *err;
    // if (checkDB() == SQLITE_OK)
    setNumConfigs(); // set numConfigs
    char* baseQ = "SELECT * FROM fileshare_configmodel LIMIT 1 OFFSET ";
    char query[sizeof(baseQ)];
    sprintf(query, "%d", 2 );
    strcat(query, ";");

    sqlite3_prepare_v2(dbDisco, query, -1, &stmt, 0);
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
    }
}

void viewTables(){
    sqlite3* dbTest;
    sqlite3* dbDisco;
    sqlite3_stmt* stmt;
    int rcTest, rcDjango;
    char *err;

    // rcTest = sqlite3_open("./dbspot/test.sqlite3", &dbTest);
    rcDjango = sqlite3_open("/home/pi/GatorDisco/disco_server/dbspot/gdiscoDb.sqlite3", &dbDisco);
    printf("Received return code %d upon opening Gdiscodb.\n", rcDjango );
    // printf("Received return code %d upon opening testdb.\n", rcTest );

    // rcDjango = sqlite3_exec(dbDisco, "SELECT * from fileshare_configmodel",NULL,NULL,&err);
    // if (rcDjango != SQLITE_OK){
    //     printf("Error: %s",err);
    // }
    sqlite3_prepare_v2(dbDisco, "select name, octave, oscParam1, oscParam2, lfoSpeed, lfoval, Attack, Decay, Sustain, Release, Effect1, Effect2, OscType, effectType, lfoTarget from fileshare_configmodel", -1, &stmt, 0);

}

int main(){
    openDB();
    int nConfigs;
    // loadConfig(0);
    setNumConfigs();
    printf("Found %d config files.\n",numConfigs);
    listConfigs();
    loadConfig(0);
    closeDB();
}