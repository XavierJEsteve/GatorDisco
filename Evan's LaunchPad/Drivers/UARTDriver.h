/*
 * UARTDriver.h
 *
 *  Created on: Nov 19, 2021
 *      Author: rives
 */

#ifndef DRIVERS_UARTDRIVER_H_
#define DRIVERS_UARTDRIVER_H_

#include <F28x_Project.h>
#include <string.h>
#include <stdio.h>

#define SCIA_BASE 0x00007200U

void UART_INIT();

char IN_CHAR(); //takes in char one at a time from uart buffer rx
char* IN_STRING(); //appends character from IN_CHAR to a string

int str_to_int(char* string);

void OUT_CHAR(char a);
void OUT_STRING(char*);


#endif /* DRIVERS_UARTDRIVER_H_ */
