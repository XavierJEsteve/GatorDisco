/*
 * LCDDriver.h
 *
 *  Created on: Sep 30, 2021
 *      Author: rives
 */

#ifndef LCDDRIVER_H_
#define LCDDRIVER_H_

#include <F28x_Project.h>
#include "OneToOneI2CDriver.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void print_string(char* msg, bool newline);
Uint16* String_to_send(char* msg);

void clearscreen();
void init_LCD();

Uint16* sendDigit(Uint16 data);
void printDigit(Uint16 data);

#endif /* LCDDRIVER_H_ */
