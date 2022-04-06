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

int configPointer = 1;
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
    openDB(dbDisco);
    
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
    closeDB(dbDisco);
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
   
    sqlite3_stmt* stmt;
    int rc;
    char *err;
    int baselen;
    int OFFSET;
    int nConfigs;
    
    //logic for setting index    
    nConfigs = setNumConfigs(); // set numConfigs
    
    // The DB is empty
    if (nConfigs == 0){
        // Load values of 0
        return;
    }

    // Moving to the NEXT config
    else if (dir == 1){
        // Make sure there is a next row
        if (configPointer < nConfigs){
            OFFSET = configPointer; //
        }

        else{ // We are at the last row, or there is only 1 row anyways
            OFFSET = 0; // to get the first row 
        } 
    }
    else if (dir == 0) {
        // Make sure there is a prev row
        if (configPointer > 1){
            OFFSET = configPointer-2;
        }
        else{ // We are at the first row, or there is only 1 row anyways
            OFFSET = nConfigs-1;
        } 
    }

    char baseQ[100];
    
    strcpy(baseQ, "SELECT * FROM fileshare_configmodel LIMIT 1 OFFSET ");
    baselen = strlen(baseQ);
    baseQ[baselen] = OFFSET+'0'; //Requested index goes HERE
    baseQ[baselen+1] = ';';
    
    sqlite3_prepare_v2(dbDisco, baseQ, -1, &stmt, 0);
    char* name;
    int octave, oscParam1, oscParam2, 
    lfoSpeed,   lfoval, 
    Attack,     Decay,      Sustain,    Release, 
    Effect1,    Effect2, 
    OscType,    effectType, lfoTarget;

    while (sqlite3_step(stmt) != SQLITE_DONE){

        configPointer = sqlite3_column_int(stmt,0); // Use index as the pointer

        name        = sqlite3_column_text(stmt,1);
        printf("Loaded config: %s\n", name);
        octave      = sqlite3_column_int(stmt,2);
        oscParam1   = sqlite3_column_int(stmt,3);
        oscParam2   = sqlite3_column_int(stmt,4);
        lfoSpeed    = sqlite3_column_int(stmt,5);
        lfoval      = sqlite3_column_int(stmt,6);
        Attack      = sqlite3_column_int(stmt,7);
        Decay       = sqlite3_column_int(stmt,8);
        Sustain     = sqlite3_column_int(stmt,9);
        Release     = sqlite3_column_int(stmt,10);
        Effect1     = sqlite3_column_int(stmt,11);
        Effect2     = sqlite3_column_int(stmt,12);
        OscType     = sqlite3_column_int(stmt,13);
        effectType  = sqlite3_column_int(stmt,14);
        lfoTarget   = sqlite3_column_int(stmt,15);
        
    }
}

void viewTables(){
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

void insertEntry_Image(){

    // First, read the image file
    FILE *fp = fopen("./media/config1.png","rb");
    if (fp == NULL){
        printf("Cannot open image file\n");
    }

    fseek(fp,0,SEEK_END);

    if(ferror(fp)){
        fprintf(stderr, "fseek() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");          
        }    
    }

    int flen = ftell(fp);

    if (flen == -1) {
        
        perror("error occurred");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }     
    }

    fseek(fp, 0, SEEK_SET);

    if (ferror(fp)) {
        
        fprintf(stderr, "fseek() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }
    }
    char data[flen+1];
    int size = fread(data, 1, flen, fp);

    if (ferror(fp)) {
        
        fprintf(stderr, "fread() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }
    }
    
    int r = fclose(fp);

    if (r == EOF) {
        fprintf(stderr, "Cannot close file handler\n");
    } 

    sqlite3_stmt* pStmt;
    int rc;
    char *errMsg = 0;
    int configPointer=4;
    openDB();

    char *sql = "INSERT INTO fileshare_configmodel (name, octave, oscParam1, oscParam2, lfoSpeed, lfoval, Attack, Decay, Sustain, Release, Effect1, Effect2, oscType, effectType, lfoTarget, image) VALUES('test4',0,1,2,3,4,5,6,7,8,9,10,11,12,13,?);";

    rc = sqlite3_prepare_v2(dbDisco, sql, -1, &pStmt, 0);
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(dbDisco));
    }


    //fill all but name and image with 0s
    // for (int i = 1; i < 14; i++){
    //     sqlite3_bind_int(pStmt, i, 0);
    //     rc = sqlite3_step(pStmt);
    //     if (rc){
    //         printf("Step %d failed: %s\n",i, sqlite3_errmsg(dbDisco));
    //     }
    // }
    //Image
    sqlite3_bind_blob(pStmt, 1, data, size, SQLITE_STATIC);
    rc = sqlite3_step(pStmt);

    if (rc != SQLITE_DONE) {
        
        printf("execution failed: %s", sqlite3_errmsg(dbDisco));
    }
        
    sqlite3_finalize(pStmt);    

    sqlite3_close(dbDisco);
}

void replaceEntry_Image(){
    // First, read the image file
    FILE *fp = fopen("./media/config1.png","rb");
    if (fp == NULL){
        printf("Cannot open image file\n");
    }

    fseek(fp,0,SEEK_END);

    if(ferror(fp)){
        fprintf(stderr, "fseek() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");          
        }    
    }

    int flen = ftell(fp);

    if (flen == -1) {
        
        perror("error occurred");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }     
    }

    fseek(fp, 0, SEEK_SET);

    if (ferror(fp)) {
        
        fprintf(stderr, "fseek() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }
    }
    char data[flen+1];
    int size = fread(data, 1, flen, fp);

    if (ferror(fp)) {
        
        fprintf(stderr, "fread() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }
    }
    
    int r = fclose(fp);

    if (r == EOF) {
        fprintf(stderr, "Cannot close file handler\n");
    } 

    sqlite3_stmt* pStmt;
    int rc;
    char *errMsg = 0;
    int configPointer=4;
    openDB();

    // char *sql = "REPLACE INTO fileshare_configmodel (name, image) VALUES('test2', ?);";
    char *sql = "UPDATE fileshare_configmodel SET image = ?, octave=100 WHERE id=2;";

    rc = sqlite3_prepare_v2(dbDisco, sql, -1, &pStmt, 0);
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(dbDisco));
    }


    //fill all but name and image with 0s
    // for (int i = 1; i < 14; i++){
    //     sqlite3_bind_int(pStmt, i, 0);
    //     rc = sqlite3_step(pStmt);
    //     if (rc){
    //         printf("Step %d failed: %s\n",i, sqlite3_errmsg(dbDisco));
    //     }
    // }
    //Image
    sqlite3_bind_blob(pStmt, 1, data, size, SQLITE_STATIC);
    rc = sqlite3_step(pStmt);

    if (rc != SQLITE_DONE) {
        
        printf("execution failed: %s", sqlite3_errmsg(dbDisco));
    }
        
    sqlite3_finalize(pStmt);    

    sqlite3_close(dbDisco);
}

void replaceEntry_Image(){
    // First, read the image file
    FILE *fp = fopen("./media/screen1.png","rb");
    if (fp == NULL){
        printf("Cannot open image file\n");
    }

    fseek(fp,0,SEEK_END);

    if(ferror(fp)){
        fprintf(stderr, "fseek() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");          
        }    
    }

    int flen = ftell(fp);

    if (flen == -1) {
        
        perror("error occurred");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }     
    }

    fseek(fp, 0, SEEK_SET);

    if (ferror(fp)) {
        
        fprintf(stderr, "fseek() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }
    }
    char data[flen+1];
    int size = fread(data, 1, flen, fp);

    if (ferror(fp)) {
        
        fprintf(stderr, "fread() failed\n");
        int r = fclose(fp);

        if (r == EOF) {
            fprintf(stderr, "Cannot close file handler\n");
        }
    }
    
    int r = fclose(fp);

    if (r == EOF) {
        fprintf(stderr, "Cannot close file handler\n");
    } 

    sqlite3_stmt* pStmt;
    int rc;
    char *errMsg = 0;
    int configPointer=9;
    openDB();

    // char *sql = "REPLACE INTO fileshare_configmodel (name, image) VALUES('test2', ?);";
    char *sql = "UPDATE fileshare_configmodel SET image = ? WHERE id= 10 ;";

    rc = sqlite3_prepare_v2(dbDisco, sql, -1, &pStmt, 0);
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(dbDisco));
    }
    //Image
    sqlite3_bind_blob(pStmt, 1, data, size, SQLITE_STATIC);
    rc = sqlite3_step(pStmt);

    sqlite3_bind_int(pStmt, 2, configPointer);
    rc = sqlite3_step(pStmt);


    if (rc != SQLITE_DONE) {
        
        printf("execution failed: %s", sqlite3_errmsg(dbDisco));
        sqlite3_close(dbDisco);
    }
    sqlite3_finalize(pStmt);    
    sqlite3_close(dbDisco);
}

int main(){
    int nConfigs;
    // loadConfig(0);
    // setNumConfigs();
    // printf("Found %d config files.\n",numConfigs);
    // listConfigs();
    // loadConfig(1);
    // insertEntry_Image();
    replaceEntry_Image();
}