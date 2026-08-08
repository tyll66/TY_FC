#ifndef __MPU6050_0_H
#define __MPU6050_0_H

#include <stdint.h>

typedef struct {
    short gyro_bias[3];    // 陀螺仪零偏（x/y/z）
    short accel_bias[3];   // 加速度计零偏（x/y/z）
	
	short gyro[3];    //陀螺仪原始数据
    short accel[3];   // 加速度计原始数据
	
	float accel_g[3];       // 加速度（单位：g）
    float gyro_dps[3];      // 陀螺仪角速度（单位：°/s）

    float last_gyro_x;
    float last_gyro_y;
    float last_gyro_z;
} MPU6050_Data_t;

typedef struct {
    float Angle;      // 卡尔曼滤波后的角度（°）
    float Bias;       // 陀螺仪零偏估计（°/s）
    float P[2][2];    // 协方差矩阵（简化一维，仅2x2）
    float Q_angle;    // 角度过程噪声（可调，默认0.001）
    float Q_bias;     // 偏置过程噪声（可调，默认0.003）
    float R_measure;  // 测量噪声（可调，默认0.03）
} KalmanFilter_t;

typedef struct
{
    float pitch;    // 俯仰角，单位：°
    float roll;     // 翻滚角，单位：°
    float gyro_x;   // X轴角速度，单位：°/s
    float gyro_y;   // Y轴角速度，单位：°/s
    float gyro_z;  //z轴加速度，单位：g
    // float yaw;
    // float roll_acc;
    // float pitch_acc;
    // float roll_gyro;
    // float pitch_gyro;
} MPU_Data_PID_t;

extern MPU6050_Data_t mpu_data;
// extern MPU_Data_PID_t mpu_pid_data;

uint8_t mpu6050_init(void);

void MPU6050_GETANGLE(float dt,MPU_Data_PID_t* mpu_pid_data);
// static HAL_StatusTypeDef MPU6050_GetRawData(short *accel, short *gyro);


#endif

