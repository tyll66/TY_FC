/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "MPU6050_0.h"
#include "PID.h"
#include "NRF24L01.h"
#include "Filter.h"
#include "tim.h"
#include "spi.h"
#include "semphr.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK_PRIORITY_NRF osPriorityNormal
#define TASK_PRIORITY_MPU osPriorityNormal
#define TASK_PRIORITY_PID osPriorityNormal

#define TASK_STACK_SIZE_NRF  512
#define TASK_STACK_SIZE_MPU  512
#define TASK_STACK_SIZE_PID  512

#define QUEUE_LENGTH_MPU_DATA     1
#define QUEUE_LENGTH_NRF_DATA     1

#define MAX_SPEED 839
#define INIT_SPEED 0
#define M1_OFFEST_K 0.93f
#define M2_OFFEST_K 1.0f
#define M3_OFFEST_K 0.9f
#define M4_OFFEST_K 0.93f

// typedef struct {
//     float gyro_x;    // 角速度 (deg/s)
//     float gyro_y;
//     float gyro_z;
//     float roll;     // 加速度 (g)
//     float pitch;
// } MPU_Data_t;

PID_Struct pitch_pid = {.kp = 10.0f,.ki = 0.0f ,.kd = 0.0f};
PID_Struct gyro_y_pid ={.kp = 0.48f,.ki = 0.0f ,.kd = 0.04f};
PID_Struct roll_pid = {.kp = 10.0f,.ki = 0.0f ,.kd = 0.0f};
PID_Struct gyro_x_pid ={.kp = 0.48f,.ki = 0.0f ,.kd = 0.04f};
PID_Struct gyro_z_pid ={.kp = 0.5f,.ki = 0.0f ,.kd = 0.0f};

void MPU6050_ReadTask(void *argument);
void NRF24L01_RecvTask(void *argument);
void PID_Task(void *argument);
void PID_Process(float roll_angle,float pitch_angle,float dt,MPU_Data_PID_t *mpu_pid);
void Start_Wait(void);
void motor_init_obyo(void);
void motor_init_together(void);
void speed_contral(int *value);
void setup_contral(int *setup);
void Motor_Totol_Control(Target_State_t *motor_target_state);
void NRF_Init(void);

Target_State_t target_state = {.target_speed = 0,.target_roll = 0,.target_pitch = 0};

int m1_value = 0;
int m2_value = 0;
int m3_value = 0;
int m4_value = 0;

int m1_step = 0;
int m2_step = 0;
int m3_step = 0;
int m4_step = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
osThreadId_t MPU6050_ReadTaskHandle;
osThreadId_t NRF24L01_RecvTaskHandle;
osThreadId_t PID_CalcTaskHandle;
osThreadId_t Motor_CtrlTaskHandle;

osMessageQueueId_t MPU_DataQueueHandle;
osMessageQueueId_t NRF_RecvQueueHandle;

osSemaphoreId_t NRF_RecvSemaphoreHandle;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
void Start_task(void);
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
const osThreadAttr_t MPU6050_ReadTask_attributes = {
  .name = "MPU6050_ReadTask",
  .stack_size = TASK_STACK_SIZE_MPU * 4,
  .priority = TASK_PRIORITY_MPU,
};

const osThreadAttr_t NRF24L01_RecvTask_attributes = {
  .name = "NRF24L01_RecvTask",
  .stack_size = TASK_STACK_SIZE_NRF * 4,
  .priority = TASK_PRIORITY_NRF,
};

const osThreadAttr_t PID_CalcTask_attributes = {
  .name = "PID_CalcTask",
  .stack_size = TASK_STACK_SIZE_PID * 4,
  .priority = TASK_PRIORITY_PID,
};

const osMessageQueueAttr_t IMU_DataQueue_attributes = {
  .name = "IMU_DataQueue"
};
const osMessageQueueAttr_t NRF_RecvQueue_attributes = {
  .name = "NRF_RecvQueue"
};


const osSemaphoreAttr_t NRF_RecvSemaphore_attributes = {
  .name = "NRF_RecvSemaphore"
};
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  NRF_RecvSemaphoreHandle = osSemaphoreNew(1, 0, &NRF_RecvSemaphore_attributes);

  /* 创建消息队列 */
  MPU_DataQueueHandle = osMessageQueueNew(QUEUE_LENGTH_MPU_DATA, sizeof(MPU_Data_PID_t), &IMU_DataQueue_attributes);
  NRF_RecvQueueHandle = osMessageQueueNew(QUEUE_LENGTH_NRF_DATA, sizeof(Target_State_t), &NRF_RecvQueue_attributes);

  /* 创建任务 */
  MPU6050_ReadTaskHandle = osThreadNew(MPU6050_ReadTask, NULL, &MPU6050_ReadTask_attributes);
  NRF24L01_RecvTaskHandle = osThreadNew(NRF24L01_RecvTask, NULL, &NRF24L01_RecvTask_attributes);
  PID_CalcTaskHandle = osThreadNew(PID_Task, NULL, &PID_CalcTask_attributes);

  if (MPU6050_ReadTaskHandle == NULL)
  {
      printf("MPU task create failed\r\n");
  }
  else
  {
      printf("MPU task create ok\r\n");
  }

  if (NRF24L01_RecvTaskHandle == NULL)
  {
      printf("NRF task create failed\r\n");
  }
  else
  {
      printf("NRF task create ok\r\n");
  }

  if (PID_CalcTaskHandle == NULL)
  {
      printf("PID task create failed\r\n");
  }
  else
  {
      printf("PID task create ok\r\n");
  }

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void MPU6050_ReadTask(void *argument)
{
  // printf("MPU_Read_Task_Start\n");
  MPU_Data_PID_t mpu_pid_data_send;
  uint32_t next_wake_time;
  next_wake_time = osKernelGetTickCount();
  uint16_t print_cnt = 0;
  MPU_Data_PID_t old_data;
  while (1)
  {
    MPU6050_GETANGLE(0.005,&mpu_pid_data_send);
     if (++print_cnt >= 100)
        {
          print_cnt = 0;
          // printf("MPU roll=%.1f pitch=%.1f gx=%.1f gy=%.1f gz=%.1f\r\n",
          //          (double)mpu_pid_data_send.roll,
          //          (double)mpu_pid_data_send.pitch,
          //          (double)mpu_pid_data_send.gyro_x,
          //          (double)mpu_pid_data_send.gyro_y,
          //          (double)mpu_pid_data_send.gyro_z);
        }
        if (osMessageQueuePut(MPU_DataQueueHandle, &mpu_pid_data_send, 0, 0) != osOK)
        {
            osMessageQueueGet(MPU_DataQueueHandle, &old_data, NULL, 0);
            osMessageQueuePut(MPU_DataQueueHandle, &mpu_pid_data_send, 0, 0);
        }
    next_wake_time += 5; 
    osDelayUntil(next_wake_time);
  }
}
void PID_Task(void *argument)
{
  // printf("PID_Task_Start\n");
  Target_State_t PID_Target_State = {0};
  MPU_Data_PID_t mpu_pid_data_recv = {0};
  uint32_t next_wake_time;
  next_wake_time = osKernelGetTickCount();
  uint16_t print_cnt = 0;
  while (1)
  {
    if (osMessageQueueGet(MPU_DataQueueHandle, &mpu_pid_data_recv, NULL, 0) == osOK) {

    }else{
      // printf("mpu_pid_data_recv_Error\n");
    }
    if(osMessageQueueGet(NRF_RecvQueueHandle, &PID_Target_State, NULL, 0) == osOK){

    }
    else{
      // printf("PID_Target_State_Error\n");
      PID_Target_State.target_roll = 0.0;
      PID_Target_State.target_pitch = 0.0;
    }
    if (++print_cnt >= 100)
      {
          print_cnt = 0;

          // printf("PID recv roll=%.1f pitch=%.1f gx=%.1f gy=%.1f gz=%.1f\r\n",
          //         (double)mpu_pid_data_recv.roll,
          //         (double)mpu_pid_data_recv.pitch,
          //         (double)mpu_pid_data_recv.gyro_x,
          //         (double)mpu_pid_data_recv.gyro_y,
          //         (double)mpu_pid_data_recv.gyro_z);
      }
    PID_Process(PID_Target_State.target_roll,PID_Target_State.target_pitch,0.005,&mpu_pid_data_recv);
    Motor_Totol_Control(&PID_Target_State);
    next_wake_time += 5; 
    osDelayUntil(next_wake_time);
  }
  
}

void NRF24L01_RecvTask(void *argument)
{
  // printf("NRF24L01_RecvTask_START\n");
  while (1)
  {
    Target_State_t NRF_Target_State = {0.0,0.0,0.0};
    osSemaphoreAcquire(NRF_RecvSemaphoreHandle, osWaitForever);
    NRF24L01_data_care(&NRF_Target_State);
    if (osMessageQueuePut(NRF_RecvQueueHandle, &NRF_Target_State, 0, 0) != osOK)
    {
      Target_State_t old_data;
      osMessageQueueGet(NRF_RecvQueueHandle, &old_data, NULL, 0);
      osMessageQueuePut(NRF_RecvQueueHandle, &NRF_Target_State, 0, 0);
    }
    NRF24L01_ClearInterruptFlags(&hspi2, NRF_STATUS_RX_DR);
    HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_15);
  }

}

void PID_Process(float roll_angle,float pitch_angle,float dt,MPU_Data_PID_t *mpu_pid)
{
	pitch_pid.desire = pitch_angle;
	pitch_pid.measure = mpu_pid->pitch;
	gyro_y_pid.measure = mpu_pid->gyro_y;
	PID_Circulate_Chain(&pitch_pid,&gyro_y_pid,dt);

	roll_pid.desire = roll_angle;
	roll_pid.measure = mpu_pid->roll;
	gyro_x_pid.measure = mpu_pid->gyro_x;
	PID_Circulate_Chain(&roll_pid,&gyro_x_pid,dt);

  gyro_z_pid.desire = 0;
  gyro_z_pid.measure = mpu_pid->gyro_z;
  PID_Circulate(&gyro_z_pid,dt);
}


void setup_contral(int *setup)
{
	if(*setup > 839)
		*setup = 839;
	else if(*setup < -839)
		*setup = -839;
}

void Motor_Totol_Control(Target_State_t *motor_target_state)
{ 
  //串式
	m1_step = (int)(gyro_x_pid.output +gyro_y_pid.output - gyro_z_pid.output);
	m2_step = (int)(-gyro_x_pid.output +gyro_y_pid.output - gyro_z_pid.output);
	m3_step = (int)(-gyro_x_pid.output -gyro_y_pid.output - gyro_z_pid.output);
	m4_step = (int)(gyro_x_pid.output -gyro_y_pid.output - gyro_z_pid.output);

	setup_contral(&m1_step);
	setup_contral(&m2_step);
	setup_contral(&m3_step);
	setup_contral(&m4_step);
			
	m1_value=839.0f*motor_target_state->target_speed/10.0f*M1_OFFEST_K + m1_step*M1_OFFEST_K;
	m2_value=839.0f-839.0f*motor_target_state->target_speed/10.0f*M2_OFFEST_K + m2_step*M2_OFFEST_K;
	m3_value=839.0f*motor_target_state->target_speed/10.0f*M3_OFFEST_K + m3_step*M3_OFFEST_K;
	m4_value=839.0f-839.0f*motor_target_state->target_speed/10.0f*M4_OFFEST_K + m4_step*M4_OFFEST_K;

	speed_contral(&m1_value);
	speed_contral(&m2_value);
	speed_contral(&m3_value);
	speed_contral(&m4_value);
			
	// __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, m1_value);
	// __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, m2_value);
	// __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, m3_value);
	// __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, m4_value);
}

void NRF_Init(void)
{
  NRF_InitTypeDef nrf_init;
  NRF24L01_GetDefaultConfig(&nrf_init);
  NRF24L01_Init(&hspi2, &nrf_init);
	NRF24L01_SetMode(&hspi2, NRF_MODE_RX);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

    if(GPIO_Pin == GPIO_PIN_1)
    {
      osSemaphoreRelease(NRF_RecvSemaphoreHandle);
    }
}

void speed_contral(int *value)
{
  if(*value > 839)
    *value = 839;
  else if(*value < -839)
    *value = -839;
}
/* USER CODE END Application */

