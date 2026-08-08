#include "Filter.h"

float LowPass_Filter(float new_value, float last_value,float LowPass_Filter_k)
{
    return LowPass_Filter_k*new_value+(1.0f-LowPass_Filter_k)*last_value;
}

KalmanFilter_Struct kfs[3] = {
    {0.02,0,0,0,0.001,0.543},
    {0.02,0,0,0,0.001,0.543},
    {0.02,0,0,0,0.001,0.543}
};

double KalmanFilter(KalmanFilter_Struct *kf, double data)
{
    kf->Now_P = kf->LastP + kf->Q;
    kf->kg = kf->Now_P/(kf->Now_P+kf->R);
    kf->out = kf->out + kf->kg*(data - kf->out);
    kf->LastP = (1-kf->kg)*kf->Now_P;
    return kf->out;
}

