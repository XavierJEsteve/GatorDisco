#ifndef TREMOLO_H_
#define TREMOLO_H_

#include <F28x_Project.h>

void updateTremolo(float32 time, float32 rate, float32 depth);

int16 processTremolo(int16 sampleIn);



#endif /* TREMOLO_H_ */