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
#include "ssd1306.h"
#include "NRF24L01.h"
#include "adc.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define LOG_BUFFER_SIZE  512
#define OLED_LOG_ROWS 4
#define OLED_LOG_COLS 21
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c2;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
void data_care(float *speed,float *roll,float *pitch);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
  float speed = 0;
  float roll = 0;
  float pitch = 0;
uint8_t exit_status = 0;
uint16_t adc_buffer[4];
//static char oled_log_buf[OLED_LOG_ROWS][OLED_LOG_COLS + 1] = {0};
//static uint8_t oled_log_row_idx = 0;
char oled_str_buf[20] = {0};
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
 uint8_t tx_addr[5] = {0x12, 0x34, 0x56, 0x78, 0x90};
 void Show_Msg(void);
 void Unsend_Show(void);
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(500);
  OLED_Init(hi2c2);
//  NRF_LOGI("STARTTEST");
  OLED_Refresh();
  
  
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 4);
  NRF_InitTypeDef nrf_init;
  NRF24L01_GetDefaultConfig(&nrf_init);
  NRF24L01_Init(&hspi1, &nrf_init);
  NRF24L01_SetMode(&hspi1, NRF_MODE_TX);
  unsigned char send_data1[32] = {0};
  uint8_t value = 0;
//  GPIO_PinState pin_state;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	HAL_Delay(10);
	speed = 20.0f - ( (float)adc_buffer[2] / 4096.0f ) * 40.0f;
	roll = (float)adc_buffer[1]/4096*20-10;
	pitch = 10.0f - ( (float)adc_buffer[0] / 4096.0f ) * 20.0f;
	data_care(&speed,&roll,&pitch);
	if(fabs(speed)!=0.0||fabs(roll)!=0.0||fabs(pitch)!=0.0)
	{
		value = 1;
		sprintf((char*)send_data1, "%.1f %.1f %.1f", speed, roll, pitch);
		uint8_t send_len = strlen((char*)send_data1);
		NRF24L01_SimpleSend(send_data1,send_len);
		Show_Msg();
	}
	else if(value == 1)
	{
		speed = 0.0f;
		roll = 0.0f;
		pitch = 0.0f;
		sprintf((char*)send_data1, "%.1f %.1f %.1f", speed, roll, pitch);
		uint8_t send_len = strlen((char*)send_data1);
		NRF24L01_SimpleSend(send_data1,send_len);
		Show_Msg();
		value = 0;
	}
  else
  {
    Unsend_Show();
  }
//	  pin_state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15);
//	  if(pin_state == GPIO_PIN_RESET)
//	  {
//		NRF_LOGI("LOW");

//	  }
//	  NRF_LOGI("CH1: %lu", adc_buffer[0]);
//	  NRF_LOGI("CH2: %lu", adc_buffer[1]);
//	  NRF_LOGI("CH3: %lu", adc_buffer[2]);
//	  NRF_LOGI("CH4: %lu", adc_buffer[3]);
//	  if(adc_buffer[0] > 2100 || adc_buffer[0] < 2000 || adc_buffer[1] > 2100 || adc_buffer[1] < 2000 || adc_buffer[2] > 2100 || adc_buffer[2] < 2000)
//	  {

//		sprintf((char*)send_data1, "%.2f %.2f %.2f", speed, roll, pitch);
//		uint8_t send_len = strlen((char*)send_data1);
//		NRF24L01_SimpleSend(send_data1,send_len);
//		NRF_LOGI("SENDMESSAGE");

//		speed = 0;
//		roll = 0;
//		pitch = 0;
//	  }
	  //		NRF_LOGI("Speed: %d", speed);  // 第1行：速度值
//		NRF_LOGI("Roll: %d", roll);    // 第2行：横滚角
//		NRF_LOGI("Pitch: %d", pitch);  // 第3行：俯仰角
//	  else
//	  {	
//		speed = 0;
//		roll = 0;
//		pitch = 0;
//		sprintf((char*)send_data1, "%.1f %.1f %.1f", speed, roll, pitch);
//		uint8_t send_len = strlen((char*)send_data1);
//		NRF24L01_SimpleSend(send_data1,send_len);
//		NRF_LOGI("Speed: %d", speed);  // 第1行：速度值
//		NRF_LOGI("Roll: %d", roll);    // 第2行：横滚角
//		NRF_LOGI("Pitch: %d", pitch);  // 第3行：俯仰角
//	  }
//	HAL_Delay(10);
//	HAL_Delay(100);
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA12 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // 仅处理PB6的中断（避免EXTI5-EXTI9其他引脚干扰）
  if(GPIO_Pin == GPIO_PIN_6)
  {
    /* USER CODE BEGIN PB6中断处理逻辑 */
    // 示例：比如处理NRF24L01+的IRQ中断（你之前关注的场景）
    // 1. 延时消抖（可选，根据硬件情况） 
    // 2. 确认引脚仍为低电平（消抖验证）
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_RESET)
    {
		exit_status = 1;
		NRF24L01_ClearInterruptFlags(&hspi1,NRF_STATUS_RX_DR | NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
    }
    /* USER CODE END PB6中断处理逻辑 */
  }
}

void Show_Msg(void)
{
    // 第一行：CH1: 数值  (y=0  上半区)
    OLED_DrawStr(0, 0, (uint8_t*)"speed:");
    OLED_ShowFloat(36, 0, speed, 2,1);  // 显示5位数字

    // 第二行：CH2: 数值  (y=8)
    OLED_DrawStr(0, 8, (uint8_t*)"pitch:");
    OLED_ShowFloat(36, 8, pitch, 2,1);

    // 第三行：CH3: 数值  (y=16 下半区)
    OLED_DrawStr(0, 16, (uint8_t*)"roll:");
    OLED_ShowFloat(36, 16, roll, 2,1);

    OLED_DrawStr(0, 24, (uint8_t*)"SENDING");
    // 第四行：CH4: 数值  (y=24)
//    OLED_DrawStr(0, 24, (uint8_t*)"CH4: ");
//    OLED_ShowNum(36, 24, adc_buffer[3], 5);

    // 刷新整个屏幕（你驱动自带的标准刷新）
    OLED_Refresh();
}

void Unsend_Show(void)
{
  OLED_DrawStr(0, 24, (uint8_t*)"UNSENDING");
  OLED_Refresh();
}

void data_care(float *speed,float *roll,float *pitch)
{
	if(fabs(*speed)<0.8)
		*speed = 0.0f;
	if(fabs(*roll)<0.5)
		*roll = 0.0f;
	if(fabs(*pitch)<0.5)
		*pitch = 0.0f;
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
