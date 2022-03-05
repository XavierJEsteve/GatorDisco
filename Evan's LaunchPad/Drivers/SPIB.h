#ifndef SPIB_H_
#define SPIB_H_
#include <F28x_Project.h>
void SPIB_Init(Uint16 baudRate);

Uint16 SPItransmit(Uint16 data);

Uint16 SPIreceive(void);
void SPIB_Slave_Init();
void SPIB_Slave_Gpio();
#endif
