#ifndef __ADRC_H
#define __ADRC_H

#include <stdint.h>

typedef struct
{
    float kp;          // 误差反馈增益
    float b0;          // 控制增益估计
    float beta1;       // ESO 参数1
    float beta2;       // ESO 参数2

    float z1;          // 估计角速度
    float z2;          // 估计总扰动

    float u;           // 当前输出
    float u_last;      // 上一次输出

    float output_limit;
} ADRC_t;

void ADRC_Init(ADRC_t *adrc,
               float kp,
               float b0,
               float beta1,
               float beta2,
               float output_limit);

float ADRC_Update(ADRC_t *adrc,
                  float target,
                  float measure,
                  float dt);

#endif