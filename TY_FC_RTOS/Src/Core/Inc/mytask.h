#ifndef __MYTASK_H__
#define __MYTASK_H__

#include "FreeRTOS.h"
#include "task.h"

void Star_task(void);

extern osThreadId_t MPU6050_ReadTaskHandle;
extern osThreadId_t NRF24L01_RecvTaskHandle;
extern osThreadId_t PID_TaskHandle;

extern osMessageQueueId_t MPU_DataQueueHandle;
extern osMessageQueueId_t NRF_RecvQueueHandle;

extern osSemaphoreId_t NRF_RecvSemaphoreHandle;


#endif


