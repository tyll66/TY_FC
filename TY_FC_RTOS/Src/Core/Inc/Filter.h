#ifndef __FILTER_H_
#define __FILTER_H_

#include "stdint.h"

typedef struct
{
    float LastP;
    float Now_P;
    float out;
    float kg;
    float Q;
    float R;
}KalmanFilter_Struct;

extern KalmanFilter_Struct kfs[3];

float LowPass_Filter(float new_value, float last_value,float LowPass_Filter_k);

double KalmanFilter(KalmanFilter_Struct *kf, double data);


#endif

