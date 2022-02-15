#include <F2837xD_Device.h>
#include "SPIB.h"

void SPIB_Gpio();

//initializes SPIB module
void SPIB_Init(Uint16 baudRate) {
    EALLOW;
    //initialize GPIO
    SPIB_Gpio();

    EALLOW;
    //initialize SPI
    SpibRegs.SPICCR.bit.SPISWRESET = 0x0;
    SpibRegs.SPICCR.bit.CLKPOLARITY = 0;
    SpibRegs.SPICCR.bit.HS_MODE = 1;
    SpibRegs.SPICCR.bit.SPICHAR = 7;

    SpibRegs.SPICTL.bit.MASTER_SLAVE = 1;
    SpibRegs.SPICTL.bit.TALK = 1;

    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0;

    SpibRegs.SPIBRR.all = baudRate;

    SpibRegs.SPICCR.bit.SPISWRESET = 1;
    SpibRegs.SPIPRI.bit.FREE = 1;
}
void SPIB_Slave_Init(){
    /*
         * NOT CONFIGURED FOR INTERRUPTS CURRENTLY
         */
        SPIB_Slave_Gpio();

        //reset the SPI register
        EALLOW;
        SpibRegs.SPICCR.bit.SPISWRESET = 0;
        SpibRegs.SPICTL.bit.MASTER_SLAVE = 0;       //Slave mode, recieve data from R.PI

        /*
         * CLKPLOARITY = 0 and CLK_PHASE = 0
         * The SPI transmits data on the rising edge of the SPICLK signal and
         * receives data on the falling edge of the SPICLK signal
         */
        SpibRegs.SPICCR.bit.CLKPOLARITY = 0;
        SpibRegs.SPICTL.bit.CLK_PHASE = 1;
        SpibRegs.SPICTL.bit.TALK = 0;               //might disable if there is nothing we need to send back to the R.PI
        SpibRegs.SPICTL.bit.SPIINTENA = 1;          //enable SPI interrupt

        //The SPI peripheral clock needs to be running at 200 MHz
        EALLOW;
        ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0;

        SpibRegs.SPICCR.bit.HS_MODE = 0x1;          //HIGH SPEED MODE ENGAGED

        //in slave mode this register has no effect
        //SpibRegs.SPIBRR.bit.SPI_BIT_RATE = divider; //this will set bit rate to 200MHz/(BRR+1) = bit rate
        SpibRegs.SPICCR.bit.SPICHAR = 0x7;          //8-bit word transfer length

        //In non-FIFO mode, enable the receiver overrun and/or SPI interrupts (OVERRUNINTENA and SPIINTENA).



        SpibRegs.SPICCR.bit.SPISWRESET = 1;
        SpibRegs.SPIPRI.bit.FREE = 1;

}

//initializes GPIO for SPI
void SPIB_Gpio() {
    EALLOW;

    GpioCtrlRegs.GPBGMUX2.bit.GPIO63 = 3;
    GpioCtrlRegs.GPBMUX2.bit.GPIO63 = 3;
    GpioCtrlRegs.GPBDIR.bit.GPIO63 = 1;
    GpioCtrlRegs.GPBQSEL2.bit.GPIO63 = 3;

    GpioCtrlRegs.GPCGMUX1.bit.GPIO64 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO64 = 3;
    GpioCtrlRegs.GPCDIR.bit.GPIO64 = 0;
    GpioCtrlRegs.GPCQSEL1.bit.GPIO64 = 3;

    GpioCtrlRegs.GPCGMUX1.bit.GPIO65 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO65 = 3;
    GpioCtrlRegs.GPCDIR.bit.GPIO65 = 1;
    GpioCtrlRegs.GPCQSEL1.bit.GPIO65 = 127;

    GpioCtrlRegs.GPCGMUX1.bit.GPIO66 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO66 = 3;
    GpioCtrlRegs.GPCDIR.bit.GPIO66 = 0;
    GpioCtrlRegs.GPCQSEL1.bit.GPIO66 = 3;

    GpioCtrlRegs.GPBPUD.bit.GPIO63;
    GpioCtrlRegs.GPCPUD.bit.GPIO64;
    GpioCtrlRegs.GPCPUD.bit.GPIO65;

}
void SPIB_Slave_Gpio() {
    EALLOW;
    /*
     * GPB has the MOSI signal
     * GPC has MISO, SPICLK, and CS
     * set the GPxGMUX first cause doc said so
     *
     */


        GpioCtrlRegs.GPCGMUX1.all = (GpioCtrlRegs.GPCGMUX1.all | 0x0000003F); //pins 64,65,66
        GpioCtrlRegs.GPBGMUX2.all = (GpioCtrlRegs.GPBGMUX2.all | 0xC0000000); //pin 63

        GpioCtrlRegs.GPBMUX2.all = (GpioCtrlRegs.GPBMUX2.all | 0xC0000000);
        GpioCtrlRegs.GPCMUX1.all = (GpioCtrlRegs.GPCGMUX1.all | 0x0000003F);

    /*    //selects the Slave select for GPIO config
        GpioCtrlRegs.GPCGMUX1.bit.GPIO66 = 3;
        GpioCtrlRegs.GPCMUX1.bit.GPIO66 = 3;*/

        //set input pins to asynchronous in GPxQSELn
        GpioCtrlRegs.GPCQSEL1.all = (GpioCtrlRegs.GPCQSEL1.all | 0x00000003); //set MISO as async


        GpioCtrlRegs.GPBDIR.bit.GPIO63 = 1; //set MOSI as output
        GpioCtrlRegs.GPCDIR.bit.GPIO64 = 0; //set MISO as input
        GpioCtrlRegs.GPCPUD.bit.GPIO64 = 0; //enable pull up
        GpioCtrlRegs.GPCDIR.bit.GPIO65 = 1; //set SPICLK as output
        GpioCtrlRegs.GPCDIR.bit.GPIO66 = 1; //set CS0 as input, this is our slave select
        GpioCtrlRegs.GPCPUD.bit.GPIO66 = 0; //enable pull up on CS0

        //GpioCtrlRegs.GPCDIR.bit.GPIO67 = 1; //set CS1 as output

}

//transmits 1 byte to SPI
#pragma CODE_SECTION(SPItransmit,".TI.ramfunc");
Uint16 SPItransmit(Uint16 data) {
    //left justified
    data = (data<<8) & 0xFF00;
    //wait for buffer to empty
    //while(SpibRegs.SPISTS.bit.BUFFULL_FLAG);
    //send data
    SpibRegs.SPITXBUF = data;
    //wait for data to transmit
    while(!(SpibRegs.SPISTS.bit.INT_FLAG));
    //receive and return data
    Uint16 ret = SpibRegs.SPIRXBUF;
    return ret;
}

//receives 1 byte from SPI
Uint16 SPIreceive(void) {
    //send dummy data
    SpibRegs.SPIDAT = 0xFF00;
    //wait for buffer to be set
    //while(!(SpibRegs.SPISTS.bit.INT_FLAG));
    //receive data
    //while(SpibRegs.SPISTS.bit.BUFFULL_FLAG);
    Uint16 data = SpibRegs.SPIRXBUF;
    //return right justified data
    return (data & 0xFF);
}
