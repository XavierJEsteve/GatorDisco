/*
 * Echo.h
 *
 *  Created on: Feb 1, 2022
 *      Author: rives
 */

#ifndef ECHO_H_
#define ECHO_H_

#include <F28x_Project.h>

void updateEchoParams(float32 time, float32 ratio);

int16 processEcho(int16 sampleIn);



#endif /* ECHO_H_ */
