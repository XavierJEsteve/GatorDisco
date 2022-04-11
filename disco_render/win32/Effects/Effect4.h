
#ifndef EFFECT4_H_
#define EFFECT4_H_

void updateEffect4Params(float time, float feedbackLine);
float cascade(float liveData, float buffData, float shift, int fback, int head, int tail, int myPointer);
float processEffect4(float sampleIn);


#endif