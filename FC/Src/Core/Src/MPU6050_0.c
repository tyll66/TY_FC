#define M_PI 3.14159265358979323846f
#include "MPU6050_0.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include <math.h>
#include "Filter.h"
#include <stdint.h>
#include "NRF24L01.h"

// MPU6050基本配置
#define MPU6050_ADDR     0x68          // I2C设备地址（HAL的Mem函数会自动左移）
#define SMPLRT_DIV_REG   0x19          // 采样率分频寄存器
#define CONFIG_REG       0x1A          // 配置寄存器（低通滤波）
#define GYRO_CONFIG_REG  0x1B          // 陀螺仪配置（量程）
#define ACCEL_CONFIG_REG 0x1C          // 加速度计配置（量程）
#define PWR_MGMT_1_REG   0x6B          // 电源管理寄存器
#define ACCEL_XOUT_H_REG 0x3B          // 加速度计X轴高位寄存器
#define GYRO_XOUT_H_REG  0x43          // 陀螺仪X轴高位寄存器
#define LOG_BUFFER_SIZE  512

// 传感器量程配置（更新：加速度计最大±16g，陀螺仪已最大±2000°/s）
#define GYRO_FS_2000DPS  3               // 陀螺仪最大量程±2000°/s（无需修改）
#define ACCEL_FS_16G     3               // 加速度计最大量程±16g（替代原来的±2g）

#define GYRO_FS_500DPS   1
#define ACCEL_FS_4G      1


#define GYRO_SCALE       (1.0f / 65.5f)     // ±500°/s：65.5 LSB/(°/s)
#define ACCEL_SCALE      (1.0f / 8192.0f)   // ±4g：8192 LSB/g

#define COMPLEMENTARY_ALPHA 0.98f        // 权重不变，平衡平滑性和纠偏
#define SAMPLING_FREQ      250.0f        // 采样率100Hz（核心修改）
#define DT                 (1.0f / SAMPLING_FREQ) // 采样周期0.01s（10ms
#define PRINT_DIV_FACTOR   5             // 每5次采样打印1次（100Hz→20Hz打印，进一步减少串口阻塞）

#define CALIBRATION_SAMPLES 500          // 校准采样次数（5秒）
#define ACCEL_THRESHOLD     0.05f        // 加速度计校准阈值（g）
#define GYRO_THRESHOLD      5.0f         // 陀螺仪校准阈值（°/s）



#define Q_ANGLE 0.001f    // 角度过程噪声（调大，降低对预测的信任）
#define Q_BIAS 0.003f     // 偏置过程噪声（默认值）
#define R_MEASURE 0.03f   // 测量噪声（调大，降低对加速度计的信任）


MPU6050_Data_t mpu_data = {0};
MPU6050_Data_t* MPU_Data = &mpu_data;
MPU_Data_PID_t mpu_pid_data = {0};


// 函数声明
static void MPU6050_WriteReg(uint8_t reg, uint8_t data);
static void MPU6050_ReadRegs(uint8_t reg, uint8_t len, uint8_t *data);
static void MPU6050_GetRawData(short *accel, short *gyro);
static void MPU6050_Calibrate(void);
void MPU6050_GETANGLE(float dt,MPU_Data_PID_t* mpu_pid_data);

/**
 * @brief  MPU6050写寄存器
 */
static void MPU6050_WriteReg(uint8_t reg, uint8_t data)
{
    HAL_I2C_Mem_Write(&hi2c3, MPU6050_ADDR<<1, reg, I2C_MEMADD_SIZE_8BIT, 
                      &data, 1, 100);
}

/**
 * @brief  MPU6050读多个寄存器
 */
static void MPU6050_ReadRegs(uint8_t reg, uint8_t len, uint8_t *data)
{
    HAL_I2C_Mem_Read(&hi2c3, MPU6050_ADDR<<1, reg, I2C_MEMADD_SIZE_8BIT, 
                     data, len, 100);
}

/**
 * @brief  读取加速度计/陀螺仪原始数据
 */
static void MPU6050_GetRawData(short *accel, short *gyro)
{
    uint8_t buf[14] = {0};
    MPU6050_ReadRegs(ACCEL_XOUT_H_REG, 14, buf);
    
    // 加速度计数据（高8位+低8位）
    accel[0] = (short)((buf[0] << 8) | buf[1]);
    accel[1] = (short)((buf[2] << 8) | buf[3]);
    accel[2] = (short)((buf[4] << 8) | buf[5]);
    
    // 陀螺仪数据（高8位+低8位）
    gyro[0] = (short)((buf[8] << 8) | buf[9]);
    gyro[1] = (short)((buf[10] << 8) | buf[11]);
    gyro[2] = (short)((buf[12] << 8) | buf[13]);
    
}

void MPU6050_Calibrate(void){
	uint32_t i;
    int32_t accel_sum[3] = {0}, gyro_sum[3] = {0};
    const uint16_t calib_count = 500;  // 采集500组数据求平均
	
	MPU_Data->accel_bias[0] = MPU_Data->accel_bias[1] = MPU_Data->accel_bias[2] = 0;
    MPU_Data->gyro_bias[0] = MPU_Data->gyro_bias[1] = MPU_Data->gyro_bias[2] = 0;
	
	printf("MPU6050开始校准，请保持设备静止...\r\n");
    HAL_Delay(500); // 校准前稳定
	
	for(i = 0; i < calib_count; i++) {
        MPU6050_GetRawData(MPU_Data->accel, MPU_Data->gyro);
        accel_sum[0] += MPU_Data->accel[0];
        accel_sum[1] += MPU_Data->accel[1];
        accel_sum[2] += MPU_Data->accel[2];
        gyro_sum[0] += MPU_Data->gyro[0];
        gyro_sum[1] += MPU_Data->gyro[1];
        gyro_sum[2] += MPU_Data->gyro[2];
        HAL_Delay(2);
    }
	MPU_Data->accel_bias[0] = accel_sum[0] / calib_count;
    MPU_Data->accel_bias[1] = accel_sum[1] / calib_count;
    MPU_Data->accel_bias[2] = accel_sum[2] / calib_count + 8192;
    MPU_Data->gyro_bias[0] = gyro_sum[0] / calib_count;
    MPU_Data->gyro_bias[1] = gyro_sum[1] / calib_count;
    MPU_Data->gyro_bias[2] = gyro_sum[2] / calib_count;
	
	printf("MPU6050校准完成！\r\n");
}
void MPU6050_GETANGLE(float dt,MPU_Data_PID_t* mpu_pid_data)
{
	
	MPU6050_GetRawData(MPU_Data->accel, MPU_Data->gyro);
		
	MPU_Data->accel_g[0] = (float)(MPU_Data->accel[0] - MPU_Data->accel_bias[0])* ACCEL_SCALE;
    MPU_Data->accel_g[1] = (float)(MPU_Data->accel[1] - MPU_Data->accel_bias[1])* ACCEL_SCALE;
    MPU_Data->accel_g[2] = (float)(MPU_Data->accel[2] - MPU_Data->accel_bias[2])* ACCEL_SCALE;

	//ROLL角速度
    MPU_Data->gyro_dps[0] = (float)(MPU_Data->gyro[0] - MPU_Data->gyro_bias[0]) * GYRO_SCALE;
    MPU_Data->gyro_dps[1] = (float)(MPU_Data->gyro[1] - MPU_Data->gyro_bias[1]) * GYRO_SCALE;
    MPU_Data->gyro_dps[2] = (float)(MPU_Data->gyro[2] - MPU_Data->gyro_bias[2]) * GYRO_SCALE;
	
	MPU_Data->accel_g[1] = -MPU_Data->accel_g[1];
	MPU_Data->accel_g[2] = -MPU_Data->accel_g[2];
	MPU_Data->gyro_dps[1] = -MPU_Data->gyro_dps[1];
	MPU_Data->gyro_dps[2] = -MPU_Data->gyro_dps[2];
	
	// float accel_mag = sqrt(MPU_Data->accel_g[0]*MPU_Data->accel_g[0] + 
    //                        MPU_Data->accel_g[1]*MPU_Data->accel_g[1] + 
    //                        MPU_Data->accel_g[2]*MPU_Data->accel_g[2]);

    mpu_pid_data->gyro_x = LowPass_Filter(MPU_Data->last_gyro_x, MPU_Data->gyro_dps[0],0.15f);
    mpu_pid_data->gyro_y = LowPass_Filter(MPU_Data->last_gyro_y, MPU_Data->gyro_dps[1],0.15f);
    mpu_pid_data->gyro_z = LowPass_Filter(MPU_Data->last_gyro_z, MPU_Data->gyro_dps[2],0.95f);
    MPU_Data->last_gyro_x = mpu_pid_data->gyro_x;
    MPU_Data->last_gyro_y = mpu_pid_data->gyro_y;
    MPU_Data->last_gyro_z = mpu_pid_data->gyro_z;

    MPU_Data->accel_g[0] = KalmanFilter(&kfs[0],MPU_Data->accel_g[0]);
    MPU_Data->accel_g[1] = KalmanFilter(&kfs[1],MPU_Data->accel_g[1]);
    MPU_Data->accel_g[2] = KalmanFilter(&kfs[2],MPU_Data->accel_g[2]);
    

	float roll_acc = atan2f(MPU_Data->accel_g[1], sqrt(MPU_Data->accel_g[0]*MPU_Data->accel_g[0]+MPU_Data->accel_g[2]*MPU_Data->accel_g[2])) * 180.0f / M_PI;
    float pitch_acc = atan2f(-MPU_Data->accel_g[0], sqrt(MPU_Data->accel_g[1]*MPU_Data->accel_g[1] + MPU_Data->accel_g[2]*MPU_Data->accel_g[2])) * 180.0f / M_PI;
    
    // mpu_pid_data->roll_acc = roll_acc;
    // mpu_pid_data->pitch_acc = pitch_acc;

    // mpu_pid_data->roll_gyro = mpu_pid_data->roll + mpu_pid_data->gyro_x*dt;
    // mpu_pid_data->pitch_gyro = mpu_pid_data->pitch + mpu_pid_data->gyro_y*dt;

    mpu_pid_data->roll = LowPass_Filter(mpu_pid_data->roll + mpu_pid_data->gyro_x*dt,roll_acc,0.02f);
    mpu_pid_data->pitch = LowPass_Filter(mpu_pid_data->pitch + mpu_pid_data->gyro_y*dt,pitch_acc,0.02f);
    // mpu_pid_data->yaw =mpu_pid_data->yaw + mpu_pid_data->gyro_z*dt;

//	printf("mpu_pid_data->roll%.1f",mpu_pid_data->roll);
//	printf("mpu_pid_data->pitch%.1f",mpu_pid_data->pitch);

    // mpu_pid_data->roll = LowPass_Filter(roll_acc,mpu_pid_data->gyro_x*dt,0.98f);
    // mpu_pid_data->pitch = LowPass_Filter(pitch_acc, mpu_pid_data->gyro_y*dt, 0.98f);

    // printf("accel_mag: %.2f\n",accel_mag);
}

uint8_t mpu6050_init(void)
{
    // 1. 唤醒MPU6050（解除睡眠）
    MPU6050_WriteReg(PWR_MGMT_1_REG, 0x00);
    HAL_Delay(100);
//    MPL_LOGI("1");
    MPU6050_WriteReg(SMPLRT_DIV_REG, 0x03);//设置采样率
    HAL_Delay(10);
//	MPL_LOGI("2");
    MPU6050_WriteReg(CONFIG_REG, 0x04);//低通滤波截止频率，0x04:20hz 0x03:42hz
    HAL_Delay(10);
//    MPL_LOGI("3");
    // 4. 配置陀螺仪量程：±2000°/s
    MPU6050_WriteReg(GYRO_CONFIG_REG, GYRO_FS_500DPS << 3);//GYRO_FS_2000DPS << 3
    HAL_Delay(10);
//    MPL_LOGI("4");
    // 5. 配置加速度计量程：±16g
    MPU6050_WriteReg(ACCEL_CONFIG_REG, ACCEL_FS_4G << 3);//ACCEL_FS_16G << 3
    HAL_Delay(10);
//    MPL_LOGI("5");
	MPU6050_Calibrate();
//	MPL_LOGI("6");
	// MPU6050_GetRawData(MPU_Data->accel, MPU_Data->gyro);
    // int32_t accel_cal[3] = {
    //     (int32_t)MPU_Data->accel[0] - MPU_Data->accel_bias[0],
    //     (int32_t)MPU_Data->accel[1] - MPU_Data->accel_bias[1],
    //     (int32_t)MPU_Data->accel[2] - MPU_Data->accel_bias[2]
    // };
    // float ax = (float)accel_cal[0] * ACCEL_SCALE;
    // float ay = (float)accel_cal[1] * ACCEL_SCALE;
    // float az = (float)accel_cal[2] * ACCEL_SCALE;
	
	// ay = -ay;
	// az = -az;
//	MPL_LOGI("7");
	
	// float init_roll = atan2(ay, az) * 180.0f / M_PI;
    // float init_pitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180.0f / M_PI;
	
	// KalmanFilter_Init(&KF_Roll, init_roll); 
    // KalmanFilter_Init(&KF_Pitch, init_pitch);
//    KF_Roll.Angle = atan2(ay, az) * 180.0f / M_PI;
//    KF_Pitch.Angle = atan2(-ax, sqrt(ay*ay + az*az)) * 180.0f / M_PI;
	
//    MPL_LOGI("MPU6050初始化完成！\r\n");
    return 0;
}

