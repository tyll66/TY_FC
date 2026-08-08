#include "ADRC.h"

static float Limit_Float(float value, float min, float max)
{
    if (value > max) return max;
    if (value < min) return min;
    return value;
}

void ADRC_Init(ADRC_t *adrc,
               float kp,
               float b0,
               float beta1,
               float beta2,
               float output_limit)
{
    adrc->kp = kp;
    adrc->b0 = b0;
    adrc->beta1 = beta1;
    adrc->beta2 = beta2;

    adrc->z1 = 0.0f;
    adrc->z2 = 0.0f;

    adrc->u = 0.0f;
    adrc->u_last = 0.0f;

    adrc->output_limit = output_limit;
}

float ADRC_Update(ADRC_t *adrc,
                  float target,
                  float measure,
                  float dt)
{
    float e;
    float u0;
    float u;

    if (dt <= 0.000001f)
    {
        return adrc->u_last;
    }

    /*
     * ESO：估计角速度 z1 和总扰动 z2
     */
    e = adrc->z1 - measure;

    adrc->z1 += dt * (adrc->z2 + adrc->b0 * adrc->u_last - adrc->beta1 * e);
    adrc->z2 += dt * (-adrc->beta2 * e);

    /*
     * 控制律
     */
    u0 = adrc->kp * (target - adrc->z1);

    u = (u0 - adrc->z2) / adrc->b0;

    /*
     * 输出限幅
     */
    u = Limit_Float(u, -adrc->output_limit, adrc->output_limit);

    adrc->u = u;
    adrc->u_last = u;

    return u;
}