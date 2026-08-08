/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"  
#include "MPU6050_0.h"
#include "NRF24L01.h"
#include "PID.h" 
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
#define MAX_SPEED 4199
#define INIT_SPEED 2000
#define M1_OFFEST_K 1.0f
#define M2_OFFEST_K 1.0f
#define M3_OFFEST_K 1.0f
#define M4_OFFEST_K 1.0f



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void Start_Wait(void);
void motor_init_obyo(void);
void motor_init_together(void);
void speed_contral(int *value);
void PID_Process(float roll_angle,float pitch_angle,float dt);
void setup_contral(int *setup);
void NRF_Receive(void);
void Motor_Totol_Control(void);
void NRF_Init(void);
void Signal_PID(float roll_angle,float pitch_angle);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

PID_Struct pitch_pid = {.kp = 15.0f,.ki = 0.0f ,.kd = 0.0f};
PID_Struct gyro_y_pid ={.kp = 8.0f,.ki = 0.0f ,.kd = 0.03f};
PID_Struct roll_pid = {.kp = 15.0f,.ki = 0.0f ,.kd = 0.0f};
PID_Struct gyro_x_pid ={.kp = 8.0f,.ki = 0.0f ,.kd = 0.03f};
// PID_Struct yaw_pid ={.kp = 1.0f,.ki = 0.0f ,.kd = 0.0f};
PID_Struct gyro_z_pid ={.kp = 15.0f,.ki = 0.0f ,.kd = 0.0f};

Target_State_t target_state = {.target_speed = 0,.target_roll = 0,.target_pitch = 0};

int m1_value = 0;
int m2_value = 0;
int m3_value = 0;
int m4_value = 0;

int m1_step = 0;
int m2_step = 0;
int m3_step = 0;
int m4_step = 0;

uint16_t lost_count = 0;
// int Base_Speed_B = INIT_SPEED;
// int Base_Speed_F = MAX_SPEED-INIT_SPEED;

volatile uint8_t nrf_rx_flag = 0;
volatile uint8_t start_flag = 0;
// volatile uint32_t dbg_step = 0;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C3_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
	HAL_Delay(500);

  // NRF24L01_RX_SelfTest_Enhanced();
  // NRF24L01_RX_SelfTest_NoSender();
  NRF_Init();
	HAL_Delay(1000);
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
	Start_Wait();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
	
	mpu6050_init();
	HAL_Delay(500);
	motor_init_together();
  // motor_init_obyo();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	static uint32_t last_log = 0;
  static uint32_t last_time = 0;
  uint32_t run_time = 0;
//  float f_run_time = 0.0f;
  float dt =0.0f;
  // uint32_t last_time = 0.0f;
  // uint32_t start_time = 0;
  // uint32_t finish_time = 0;
  // volatile uint32_t loop_count = 0;
  while (1)
  {
	  // printf("runing");
    // loop_count++;
    // printf("Loop Count: %lu\n",loop_count);
    // start_time = HAL_GetTick();
    lost_count++;
    // run_time = HAL_GetTick() - last_time;
    dt = 0.001f;
    MPU6050_GETANGLE(dt,&mpu_pid_data);
    PID_Process(Target_State.target_roll-1.0f,Target_State.target_pitch+2.0f,dt);
    // last_time = HAL_GetTick();
    NRF_Receive();
    Motor_Totol_Control();
    // finish_time = HAL_GetTick();  
//		HAL_Delay(100);
		// if(HAL_GetTick()- last_log >= 250)
		// {
    //   printf("dt:%.4f\n",dt);
		//   printf("***********************************\n");
		//   printf("姿态互补翻滚角 = %.1f °\n", mpu_pid_data.roll);
		//   printf("姿态互补俯仰角 = %.1f °\n",mpu_pid_data.pitch);
      // printf("偏航角 = %.1f °\n",mpu_pid_data.yaw);
      // printf("纯陀螺仪翻滚角 = %.1f °\n",mpu_pid_data.roll_gyro);
      // printf("纯陀螺仪俯仰角 = %.1f °\n",mpu_pid_data.pitch_gyro);
      // printf("纯加速度翻滚角 = %.1f °\n",mpu_pid_data.roll_acc);
      // printf("纯加速度俯仰角 = %.1f °\n",mpu_pid_data.pitch_acc);
		  // printf("陀螺仪原始y轴角速度 = %.1f \n", mpu_pid_data.gyro_y);
		  // printf("陀螺仪原始x轴角速度 = %.1f \n", mpu_pid_data.gyro_x);
      // printf("陀螺仪原始z轴角速度 = %.1f\n",mpu_pid_data.gyro_z);
      // printf("运行时长：%1u s\n",finish_time-start_time);
      // printf("roll_pid.err: %.1f\n",roll_pid.err);
      // printf("roll_pid.kp: %.1f\n",roll_pid.kp);
      // printf("dt: %.4f\n",dt);
      // printf("run_time: %lu\n",run_time);
      // printf("f_run_time: %.4f\n",f_run_time);
      // printf("PID输出翻滚角目标角速度 = %.1f \n", roll_pid.output);
      // printf("PID输出俯仰角目标角速度 = %.1f \n", pitch_pid.output);
      // printf("PID翻滚角最终输出结果 = %.1f \n", gyro_x_pid.output);
      // printf("PID俯仰角最终输出结果 = %.1f \n", gyro_y_pid.output);
      // printf("PID偏航角最终输出结果 = %.1f \n",gyro_z_pid.output);
//		
//		
//		printf("目标翻滚角度：%.1f\n",f_roll);
//		printf("目标俯仰角度：%.1f\n",f_pitch);
//		printf("提速状况：%.1f,\n",speed);
//		printf("m1_value:%d\n",m1_value);	
//		GPIO_PinState a = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4);
//		printf("电平状态: %d\n", a);
//		printf("m2_value:%d\n",m2_value);
//		GPIO_PinState b = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5);
//		printf("电平状态: %d\n", b);
//		printf("m3_value:%d\n",m3_value);
//		GPIO_PinState c = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);
//		printf("电平状态: %d\n", c);
//		printf("m4_value:%d\n",m4_value);
//		GPIO_PinState d = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5);
//		printf("电平状态: %d\n", d);
//			
      // printf("M1当前速度：%.2f%%\n",(float)m1_value/MAX_SPEED*100.0f);
      // printf("M2当前速度：%.2f%%\n",(float)m2_value/MAX_SPEED*100.0f);
      // printf("M3当前速度：%.2f%%\n",(float)m3_value/MAX_SPEED*100.0f);
      // printf("M4当前速度：%.2f%%\n",(float)m4_value/MAX_SPEED*100.0f);
//			
      // printf("M1步进值：%d\n",m1_step);
      // printf("M2步进值：%d\n",m2_step);
      // printf("M3步进值：%d\n",m3_step);
      // printf("M4步进值：%d\n",m4_step);
//			
		// last_log = HAL_GetTick();
		// }
//		printf("循环用时 %lu\n",HAL_GetTick()-start_time);
//	  balance_contral();
//	  speed_contral(&m1_value);
//	  speed_contral(&m2_value);
//	  speed_contral(&m3_value);
//	  speed_contral(&m4_value);
//	  m1_contral(m1_value);
//	  m2_contral(m2_value);
//	  m3_contral(m3_value);
//	  m4_contral(m4_value);
//	  printf("电机控制值: M1=%d, M2=%d, M3=%d, M4=%d\r\n", 
//         m1_value, m2_value, m3_value, m4_value);
//	  HAL_Delay(100);
    /* USER CODE END WHILE */
	
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};//时钟晶振配置结构体
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};//时钟配置结构体

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();//使能电源控制时钟
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);//配置电源为最高级以发挥最大频率

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;//配置使用外部高速晶振
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;//开启外部高速晶振
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;//开启锁相环
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;//配置锁相环的时钟源
  RCC_OscInitStruct.PLL.PLLM = 8;//8倍分频
  RCC_OscInitStruct.PLL.PLLN = 336;//倍频
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;//2分频
  RCC_OscInitStruct.PLL.PLLQ = 4;//4分频
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;//使能需要使用的时钟
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;//配置时钟源为PPL
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;//配置AHB频率不分频
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;//配置APB1频率4分频
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;//配置APB2频率2分频

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;// 配置IIC速率
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;// 快速模式下需要的配置，配置低：高为2：1
  hi2c3.Init.OwnAddress1 = 0;// 不设置作为从机的地址
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;// 设置地址为7位
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;// 不设置双地址
  hi2c3.Init.OwnAddress2 = 0; //不设置第二个地址
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; // 禁用广播模式
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;// 禁用无拉伸主机等待从机准备数据
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */
 
  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;// 设置为主机
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;// 设置全双工模式
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;// 设置数据位8位
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;// 设置空闲时电平0，此时第一个边沿是上升沿
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;// 设置上升沿采集信号
  hspi2.Init.NSS = SPI_NSS_SOFT;//使用软件片选
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;//设置SPI通信速率
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;//设置从高位起进行发送
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;//不使用TI协议
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;//不进行CRC校验
  hspi2.Init.CRCPolynomial = 10;//禁止CRC,随便填的值
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;//不预分频
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;//向上计数
  htim2.Init.Period = 4199;//设置PWM频率为80khz
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;//滤波时钟不分频
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;//启用自动重装载
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;//用于触发其他外设（无用）
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;//关闭定时器的主从同步模式
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;//设置为PWM1模式，<CCR时输出高电平
  sConfigOC.Pulse = 0;//设置初始CCR为0
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;//设置有效电平为高电平
//  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE; //设置为高速模式
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */
  
  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);
  
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 4199;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 4199;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;//波特率
  huart1.Init.WordLength = UART_WORDLENGTH_8B;//8位数据位
  huart1.Init.StopBits = UART_STOPBITS_1;//1位停止位
  huart1.Init.Parity = UART_PARITY_NONE;//无校验位
  huart1.Init.Mode = UART_MODE_TX_RX;//全双工模式
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;//无硬件流控
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;//16倍采样
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();//使能各时钟

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC1 PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;//下降沿触发的外部中断
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;//通用推挽
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PC4 PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;//通用推挽
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB15 PB4 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;//通用推挽
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);//设置中断优先级
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);//设置中断优先级
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
	
	
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// void motor_init_obyo()
// {
	
	// HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	// HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
	// for(uint16_t duty=0; duty<=420; duty+=5)
  //   {
  //       __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, duty);
  //       HAL_Delay(10);
  //   }
	// HAL_Delay(1000);
	
	// HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
	// HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
	// for(uint16_t duty=839; duty>=420; duty-=5)
  //   {
  //       __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, duty);
  //       HAL_Delay(10);
  //   }
	// HAL_Delay(1000);
	
	// HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	// HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	// for(uint16_t duty=0; duty<=420; duty+=5)
  //   {
  //       __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, duty);
  //       HAL_Delay(10);
  //   }
	// HAL_Delay(1000);
	
	// HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
	// HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	// for(uint16_t duty=839; duty>=420; duty-=5)
  //   {
  //       __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, duty);
  //       HAL_Delay(10);
  //       printf("M4 Running...\n");
  //   }
	// HAL_Delay(1000);
// }

void motor_init_together()
{
  //左黑右白：F
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	// HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
	// HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	// HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
	// HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	// uint16_t init_speed = 0;
	// while(init_speed<INIT_SPEED)
	// {
	// 	init_speed +=5;
	// 	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, init_speed);
	// 	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, init_speed);
	// 	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, init_speed);
	// 	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, init_speed);
	// 	HAL_Delay(100);
	// }
}

void speed_contral(int *value)
{
	if(*value >4199)
		*value = 4199;
	else if(*value <0)
		*value = 0;
}

void PID_Process(float roll_angle,float pitch_angle,float dt)
{
  if(4199.0f*Target_State.target_speed/20.0f < 1025)
  {
    pitch_pid.ki = 0.0;
    roll_pid.ki = 0.0;
  }
	pitch_pid.desire = pitch_angle;
	pitch_pid.measure = mpu_pid_data.pitch;
	gyro_y_pid.measure = mpu_pid_data.gyro_y;
	PID_Circulate_Chain(&pitch_pid,&gyro_y_pid,dt);

	roll_pid.desire = roll_angle;
	roll_pid.measure = mpu_pid_data.roll;
	gyro_x_pid.measure = mpu_pid_data.gyro_x;
	PID_Circulate_Chain(&roll_pid,&gyro_x_pid,dt);

  gyro_z_pid.desire = 0;
  gyro_z_pid.measure = mpu_pid_data.gyro_z;
  PID_Circulate(&gyro_z_pid,dt);
}

// void Signal_PID(float roll_angle,float pitch_angle)
// {
//   pitch_pid.desire = pitch_angle;
//   pitch_pid.measure = mpu_pid_data.pitch;
//   PID_Circulate(&pitch_pid);
//   roll_pid.desire = roll_angle;
//   roll_pid.measure = mpu_pid_data.roll;
//   PID_Circulate(&roll_pid);
// }

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

    if(GPIO_Pin == GPIO_PIN_1)
    {
        nrf_rx_flag = 1;
        lost_count = 0;
    }
}

void setup_contral(int *setup)
{
	if(*setup > 4199)
		*setup = 4199;
	else if(*setup < 0)
		*setup = 0;
}

void Start_Wait(void)
{
  while(1)
	{
		if(nrf_rx_flag == 1)
		{
			NRF24L01_ReadRxFIFO(&hspi2);
			NRF24L01_data_care(); 
			NRF24L01_ClearInterruptFlags(&hspi2, NRF_STATUS_RX_DR);
			nrf_rx_flag =0;
			if(Target_State.target_speed == -20.0f && Target_State.target_roll == -10.0f && start_flag == 0)
			{
				start_flag = 1;
			}
			if(Target_State.target_speed == 0.0f && Target_State.target_roll == 0.0f && start_flag == 1)
			{
				break;
			}
		}
	}
}
void NRF_Receive(void)
{
		if(nrf_rx_flag == 1)
		{
			NRF24L01_ReadRxFIFO(&hspi2);
			NRF24L01_data_care(); 
			NRF24L01_ClearInterruptFlags(&hspi2, NRF_STATUS_RX_DR);
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
      nrf_rx_flag = 0;
	  }
}
// void NRF_Receive(void)
// { 
//     if(NRF24L01_IsRxDataReady(&hspi2))
//     {
//         // 读取接收FIFO数据（自动存入对应管道的rxbuffer）
//         NRF24L01_ReadRxFIFO(&hspi2);
        
//         // 处理接收到的数据（你的原有函数）
//         NRF24L01_data_care();
        
//         // 清除RX_DR中断标志（必须，否则会重复检测）
//         NRF24L01_ClearInterruptFlags(&hspi2, NRF_STATUS_RX_DR);
        
//         // 接收指示LED翻转
//         HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
//     }
// }
void Motor_Totol_Control(void)
{ 
  //串式
	m1_step = (int)(gyro_x_pid.output +gyro_y_pid.output - gyro_z_pid.output);
	m2_step = (int)(gyro_x_pid.output -gyro_y_pid.output + gyro_z_pid.output);
	m3_step = (int)(-gyro_x_pid.output -gyro_y_pid.output - gyro_z_pid.output);
	m4_step = (int)(-gyro_x_pid.output +gyro_y_pid.output + gyro_z_pid.output);

	setup_contral(&m1_step);
	setup_contral(&m2_step);
	setup_contral(&m3_step);
	setup_contral(&m4_step);
	
  if (fabs(Target_State.last_speed-Target_State.target_speed) >= 2.5 || lost_count >= 30)
  {
    Target_State.target_speed = Target_State.last_speed - 0.003;
  }
  
	m1_value=(4199.0f*Target_State.target_speed/20.0f + m1_step)*M1_OFFEST_K;
	m2_value=(4199.0f*Target_State.target_speed/20.0f + m2_step)*M2_OFFEST_K;
	m3_value=(4199.0f*Target_State.target_speed/20.0f + m3_step)*M3_OFFEST_K;
	m4_value=(4199.0f*Target_State.target_speed/20.0f + m4_step)*M4_OFFEST_K;

  Target_State.last_speed = Target_State.target_speed;

	speed_contral(&m1_value);
	speed_contral(&m2_value);
	speed_contral(&m3_value);
	speed_contral(&m4_value);
			
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, m1_value);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, m2_value);
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, m3_value);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_3, m4_value);
}
void NRF_Init(void)
{
  NRF_InitTypeDef nrf_init;
  NRF24L01_GetDefaultConfig(&nrf_init);
  NRF24L01_Init(&hspi2, &nrf_init);
	NRF24L01_SetMode(&hspi2, NRF_MODE_RX);
}

int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 10);
    return ch;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
