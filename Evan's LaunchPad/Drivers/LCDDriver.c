/*
 * LCDDriver.c
 *
 *  Created on: Sep 30, 2021
 *      Author: rives
 */

#include <F28x_Project.h>
#include "LCDDriver.h"
void init_LCD(){
    Uint16  const commands[] = {0x3C,0x38,0x3C,0x38,
                                0x3C,0x38,0x2C,0x28,
                                0x2C,0x28,0x8C,0x88,
                                0x0C,0x08,0xFC,0xF8,
                                0x0C,0x08,0x1C,0x18};

    Uint16 commandSize = sizeof(commands) / sizeof(*commands);

    I2C_O2O_SendBytes(&commands,commandSize);

}

void print_string(char* msg, bool newline){
    if(newline == true){
        Uint16  const newline[] = {0xCC,0xC8,0x0C,0x08};
        Uint16 newlineSize = 4;
        I2C_O2O_SendBytes(&newline,newlineSize);
    }

    Uint16* const data = String_to_send(msg); //converts string into appropriate bytes to send over 4 bit mode
    Uint16 stringSize  = 4*(strlen(msg));

    I2C_O2O_SendBytes(data,stringSize);

}

void clearscreen(){
        Uint16  const commands[] = {0x0C,0x08,0x1C,0x18}; //0x01 is clear screen command

        Uint16 commandSize = sizeof(commands) / sizeof(*commands);

        I2C_O2O_SendBytes(&commands,commandSize);
}

Uint16* String_to_send(char* msg){

    Uint16 temphigh;
    Uint16 templow;
    Uint16 UPnibble_E;
    Uint16 UPnibble;
    Uint16 LOWnibble_E;
    Uint16 LOWnibble;

    Uint16 len  = strlen(msg); //gets how long the string is

    //the new array should be double the string because we are sending out 4 bytes for each char
    Uint16* ptr = (Uint16*)malloc(4*len*sizeof(Uint16));

    //main logic which creates the bytes to send is contained in this for loop
    for(int i = 0; i < len; i++)
    {
        temphigh = (Uint16)msg[i] & 0x00F0; //cast char to Uint16 and clear everything but upper nibble
        templow  = ((Uint16)msg[i] & 0x000F) << 4; //cast char to Uint16 and clear everything but lower nibble

        UPnibble_E = temphigh | 0x000D;
        UPnibble   = temphigh | 0x0009;

        LOWnibble_E = templow | 0x000D;
        LOWnibble   = templow | 0x0009;

        //puts the bytes to send in int array
        ptr[i*4] = UPnibble_E;
        ptr[i*4+1] = UPnibble;
        ptr[i*4+2] = LOWnibble_E;
        ptr[i*4+3] = LOWnibble;

    }

    return ptr;
}


void printDigit(Uint16 data){

    Uint16* const data1 = sendDigit(data); //converts string into appropriate bytes to send over 4 bit mode
    Uint16 stringSize  = 4;

    I2C_O2O_SendBytes(data1,stringSize);

}


Uint16* sendDigit(Uint16 data){
    Uint16 digitdata = data + 0x30;
    Uint16 temphigh;
    Uint16 templow;
    Uint16 UPnibble_E;
    Uint16 UPnibble;
    Uint16 LOWnibble_E;
    Uint16 LOWnibble;

    Uint16 len  = 1; //gets how long the string is

    //the new array should be double the string because we are sending out 4 bytes for each char
    Uint16* ptr = (Uint16*)malloc(4*len*sizeof(Uint16));

    //main logic which creates the bytes to send is contained in this for loop
    for(int i = 0; i < len; i++)
    {
        temphigh = digitdata & 0x00F0; //cast char to Uint16 and clear everything but upper nibble
        templow  = (digitdata & 0x000F) << 4; //cast char to Uint16 and clear everything but lower nibble

        UPnibble_E = temphigh | 0x000D;
        UPnibble   = temphigh | 0x0009;

        LOWnibble_E = templow | 0x000D;
        LOWnibble   = templow | 0x0009;

        //puts the bytes to send in int array
        ptr[i*4] = UPnibble_E;
        ptr[i*4+1] = UPnibble;
        ptr[i*4+2] = LOWnibble_E;
        ptr[i*4+3] = LOWnibble;
    }
    return ptr;
}

