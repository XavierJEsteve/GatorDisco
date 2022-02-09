/*
 * SRAMDriver.c
 *
 *  Created on: Sep 30, 2021
 *      Author: rives
 */

#include <F28x_Project.h>
#include "SRAMDriver.h"

void SRAMWrite(Uint16 data, long int address){
    //make sure CS are high (disabled)
    GpioDataRegs.GPCDAT.bit.GPIO66 = 1;
    GpioDataRegs.GPCDAT.bit.GPIO67 = 1;

    Uint16 byte1 = (data & 0x00FF) << 8; //this byte is the lower byte of word, will be sent 1st
    Uint16 byte2 = (data & 0xFF00);      //this byte is the higher byte of word, will be sent 2nd

    Uint16 WriteCommand = 0x0200;

    if (address > 0x3FFFF)
        {
            //break up the address
            Uint16 addr1 = ((address) >> 8) & 0xFF00; //isolate bits 23-16
            Uint16 addr2 = (address) & 0xFF00;          //isolate bits 15-8
            Uint16 addr3 = ((address) << 8) & 0xFF00;  //isolate bits 7-0

            //turn on CS1 GPIO 67
            GpioDataRegs.GPCDAT.bit.GPIO67 = 0;

            //transmit data
            SpiBTransmit(WriteCommand); //tell the SRAM we want to write
            SpiBTransmit(addr1); //send address of location
            SpiBTransmit(addr2);
            SpiBTransmit(addr3);
            SpiBTransmit(byte1); //Lower byte first, SRAM will increment. SRAM will effectively be little endian
            SpiBTransmit(byte2);

            GpioDataRegs.GPCDAT.bit.GPIO67 = 1;
        }
        else
        {
            //break up the address
            Uint16 addr1 = (address >> 8) & 0xFF00; //isolate bits 23-16
            Uint16 addr2 = (address & 0xFF00);  //isolate bits 15-8
            Uint16 addr3 = (address << 8) & 0xFF00; //isolate bits 7-0

            //turn on CS0 GPIO 66
            GpioDataRegs.GPCDAT.bit.GPIO66 = 0;

            //transmit data
            SpiBTransmit(WriteCommand); //tell the SRAM we want to write
            SpiBTransmit(addr1); //send address of location
            SpiBTransmit(addr2);
            SpiBTransmit(addr3);
            SpiBTransmit(byte1); //Lower byte first, SRAM will increment. SRAM will effectively be little endian
            SpiBTransmit(byte2);

            GpioDataRegs.GPCDAT.bit.GPIO66 = 1;
        }
}



Uint16 SRAMRead(long int address){

    //make sure CS are high (disabled)
        GpioDataRegs.GPCDAT.bit.GPIO66 = 1;
        GpioDataRegs.GPCDAT.bit.GPIO67 = 1;
        Uint16 low_byte  = 0;
        Uint16 high_byte = 0;

        Uint16 ReadCommand = 0x0300;

        if (address > 0x3FFFF)
            {
            //break up the address
                Uint16 addr1 = ((address) >> 8) & 0xFF00; //isolate bits 23-16
                Uint16 addr2 = (address) & 0xFF00;          //isolate bits 15-8
                Uint16 addr3 = (address << 8) & 0xFF00;  //isolate bits 7-0

                //turn on CS1 GPIO 67
                GpioDataRegs.GPCDAT.bit.GPIO67 = 0;

                //transmit data
                SpiBTransmit(ReadCommand); //tell the SRAM we want to write
                SpiBTransmit(addr1); //send address of location
                SpiBTransmit(addr2);
                SpiBTransmit(addr3);
                low_byte = SpiBTransmit(0x1234); //send dummy data, value returned is our data we read
                low_byte = SpiBTransmit(0x1234);
                high_byte = SpiBTransmit(0x1234);

                GpioDataRegs.GPCDAT.bit.GPIO67 = 1;
            }
            else
            {


                //break up the address
                Uint16 addr1 = (address >> 8) & 0xFF00; //isolate bits 23-16
                Uint16 addr2 = (address & 0xFF00);  //isolate bits 15-8
                Uint16 addr3 = (address << 8) & 0xFF00; //isolate bits 7-0

                //turn on CS0 GPIO 66
                GpioDataRegs.GPCDAT.bit.GPIO66 = 0;

                //transmit data
                SpiBTransmit(ReadCommand); //tell the SRAM we want to write
                SpiBTransmit(addr1); //send address of location
                SpiBTransmit(addr2);
                SpiBTransmit(addr3);
                low_byte = SpiBTransmit(0x1234); //send dummy data, value returned is our data we read
                low_byte = SpiBTransmit(0x1234);
                high_byte = SpiBTransmit(0x1234);
                GpioDataRegs.GPCDAT.bit.GPIO66 = 1;
            }


        low_byte = low_byte & 0x00FF;
        Uint16 data = (high_byte << 8) + low_byte; //concatenate the 2 bytes

        return data;
}

void SRAMZero()
{
    for (long int i = 0; i < 0x7FFFF; i += 2)
    {
        SRAMWrite(0x0000, i);
    }
}
