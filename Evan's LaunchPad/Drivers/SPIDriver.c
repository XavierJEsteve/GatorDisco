/*
 * SPIDriver.c
 *
 *  Created on: Sep 30, 2021
 *      Author: rives
 */


/*
 * SPIDriver.c
 *
 *  Created on: Sep 29, 2021
 *      Author: Evan
 */


#include <F28x_Project.h>
#include "SPIDriver.h"
static void SPI_GPIOA_INIT();
static void SPI_GPIOB_INIT();
static void SPISLAVE_GPIOB_INIT();


void SPI_INITA(Uint16 SPI_CLKDIVIDER)
{
    /*=======================================DO NOT USE THIS FUNCTION================================ */

    SPI_GPIOA_INIT();
    //reset the SPI register
    EALLOW;
    SpiaRegs.SPICCR.bit.SPISWRESET = 0;
    SpiaRegs.SPICTL.bit.MASTER_SLAVE = 1;
    SpiaRegs.SPICCR.bit.CLKPOLARITY = 0; //data output from uP on rising edge and latched on rising edge for SRAM
    SpiaRegs.SPICTL.bit.CLK_PHASE = 0;
    SpiaRegs.SPICTL.bit.TALK = 1;

    //The SPI peripheral clock needs to be running at 200 MHz
    EALLOW;
    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0;

    SpiaRegs.SPICCR.bit.HS_MODE = 0x1; //HIGH SPEED MODE ENGAGED
    //we divide the SPI peripheral clock by a clk divider  200MHz/Divider = Bit_Rate
    SpiaRegs.SPIBRR.bit.SPI_BIT_RATE = SPI_CLKDIVIDER - 1; //this will set bit rate to 200MHz/Rate = 2MHz
    SpiaRegs.SPICCR.bit.SPICHAR = 0x7;

    SpiaRegs.SPICCR.bit.SPISWRESET = 1;
    SpiaRegs.SPIPRI.bit.FREE = 1;


}

void SPI_INITB(Uint16 SPI_CLKDIVIDER)
{

    SPI_GPIOB_INIT();
    //reset the SPI register
    EALLOW;
    SpibRegs.SPICCR.bit.SPISWRESET = 0;
    SpibRegs.SPICTL.bit.MASTER_SLAVE = 1;
    SpibRegs.SPICCR.bit.CLKPOLARITY = 0; //data output from uP on rising edge and latched on rising edge for SRAM
    SpibRegs.SPICTL.bit.CLK_PHASE = 0;
    SpibRegs.SPICTL.bit.TALK = 1;

    //The SPI peripheral clock needs to be running at 200 MHz
    EALLOW;
    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0;

    SpibRegs.SPICCR.bit.HS_MODE = 0x1; //HIGH SPEED MODE ENGAGED
    //we divide the SPI peripheral clock by a clk divider  200MHz/Divider = Bit_Rate
    SpibRegs.SPIBRR.bit.SPI_BIT_RATE = SPI_CLKDIVIDER - 1; //this will set bit rate to 200MHz/Rate = 2MHz
    SpibRegs.SPICCR.bit.SPICHAR = 0x7;

    SpibRegs.SPICCR.bit.SPISWRESET = 1;
    SpibRegs.SPIPRI.bit.FREE = 1;


}

void SPI_SLAVEINIT(){

    /*
     * NOT CONFIGURED FOR INTERRUPTS CURRENTLY
     */
    SPISLAVE_GPIOB_INIT();

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
    SpibRegs.SPICTL.bit.CLK_PHASE = 0;
    SpibRegs.SPICTL.bit.TALK = 1;               //might disable if there is nothing we need to send back to the R.PI

    //The SPI peripheral clock needs to be running at 200 MHz
    EALLOW;
    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0;

    SpibRegs.SPICCR.bit.HS_MODE = 0x1;          //HIGH SPEED MODE ENGAGED

    //in slave mode this register has no effect
    //SpibRegs.SPIBRR.bit.SPI_BIT_RATE = divider; //this will set bit rate to 200MHz/(BRR+1) = bit rate
    SpibRegs.SPICCR.bit.SPICHAR = 0x7;          //8-bit word transfer length

    SpibRegs.SPICCR.bit.SPISWRESET = 1;
    SpibRegs.SPIPRI.bit.FREE = 1;

}

Uint16 SpiATransmit(Uint16 data)
{
    /*=================================================DO NOT USE THIS FUNCTION================================================= */
    //send data to the SPI register
    SpiaRegs.SPITXBUF = data;

    //wait until the data has been sent
    while (!SpiaRegs.SPISTS.bit.INT_FLAG);

    //return the data you received
    Uint16 received = SpiaRegs.SPIRXBUF;
    //flush any buffers and clear any flags
    return received;
}

Uint16 SpiBTransmit(Uint16 data)
{
    //send data to the SPI register
    SpibRegs.SPITXBUF = data;

    //wait until the data has been sent
    while (!SpibRegs.SPISTS.bit.INT_FLAG);

    //return the data you received
    Uint16 received = SpibRegs.SPIRXBUF;
    //flush any buffers and clear any flags
    return received;
}

Uint16 SpiBReceive(Uint16 data)
{


    //wait until the data has been sent
    while (!SpibRegs.SPISTS.bit.INT_FLAG);
    //flush any buffers and clear any flags

    //return the data you received
    Uint16 received = SpibRegs.SPIRXBUF;

    //send data to the SPI register
    SpibRegs.SPITXBUF = data;
    return received;
}


static void SPI_GPIOA_INIT()
{
    EALLOW;
    //GPB has the MOSI signal
    //GPC has MISO, SPICLK, and the gpio pins to configure
    //set the GPxGMUX first cause doc said so

    GpioCtrlRegs.GPBGMUX2.bit.GPIO58 = 3; // MOSI signal
    GpioCtrlRegs.GPBGMUX2.bit.GPIO59 = 3; // MISO signal
    GpioCtrlRegs.GPBGMUX2.bit.GPIO60 = 3; // SPICLK signal

    GpioCtrlRegs.GPBMUX2.bit.GPIO58 = 3;
    GpioCtrlRegs.GPBMUX2.bit.GPIO59 = 3;
    GpioCtrlRegs.GPBMUX2.bit.GPIO60 = 3;

    GpioCtrlRegs.GPCGMUX1.all = (GpioCtrlRegs.GPCGMUX1.all & 0xFFFFFF0F); //this is to configure the GPIO for CS
    GpioCtrlRegs.GPCMUX1.all = (GpioCtrlRegs.GPCGMUX1.all & 0xFFFFFF0F); // pins 66 & 67

    //set input pins to asynchronous in GPxQSELn
    GpioCtrlRegs.GPBQSEL2.bit.GPIO59 = 3;

    GpioCtrlRegs.GPBDIR.bit.GPIO58 = 1; //MOSI output
    GpioCtrlRegs.GPBDIR.bit.GPIO59 = 0; //MISO input
    GpioCtrlRegs.GPBDIR.bit.GPIO60 = 1; //SPIACLK output
    GpioCtrlRegs.GPCDIR.bit.GPIO66 = 1; //set CS0 as output
    GpioCtrlRegs.GPCDIR.bit.GPIO67 = 1; //set CS1 as output
}




static void SPI_GPIOB_INIT()
{
    EALLOW;
    //GPB has the MOSI signal
    //GPC has MISO, SPICLK, and the gpio pins to configure
    //set the GPxGMUX first cause doc said so


    GpioCtrlRegs.GPCGMUX1.all = (GpioCtrlRegs.GPCGMUX1.all | 0x0000000F); //or mask to only configure the pins we need
    GpioCtrlRegs.GPBGMUX2.all = (GpioCtrlRegs.GPBGMUX2.all | 0xC0000000);

    GpioCtrlRegs.GPBMUX2.all = (GpioCtrlRegs.GPBMUX2.all | 0xC0000000);
    GpioCtrlRegs.GPCMUX1.all = (GpioCtrlRegs.GPCGMUX1.all | 0x0000000F);

    GpioCtrlRegs.GPCGMUX1.all = (GpioCtrlRegs.GPCGMUX1.all & 0xFFFFFF0F); //this is to configure the GPIO for CS
    GpioCtrlRegs.GPCMUX1.all = (GpioCtrlRegs.GPCGMUX1.all & 0xFFFFFF0F);

    //set input pins to asynchronous in GPxQSELn
    GpioCtrlRegs.GPCQSEL1.all = (GpioCtrlRegs.GPCQSEL1.all | 0x00000003); //set MISO as async


    GpioCtrlRegs.GPBDIR.bit.GPIO63 = 1; //set MOSI as output
    GpioCtrlRegs.GPCDIR.bit.GPIO64 = 0; //set MISO as input
    GpioCtrlRegs.GPCPUD.bit.GPIO64 = 0; //enable pull up
    GpioCtrlRegs.GPCDIR.bit.GPIO65 = 1; //set SPICLK as output

    GpioCtrlRegs.GPCDIR.bit.GPIO66 = 1; //set CS0 as output
    GpioCtrlRegs.GPCDIR.bit.GPIO67 = 1; //set CS1 as output
}

static void SPISLAVE_GPIOB_INIT()
{
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






