#include "main.h"
#include "NRF24L01.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> 
#define LOG_BUFFER_SIZE  512

/* Public global data buffers ,32 bytes data + 1 byte data_length*/
uint8_t NRF24L01_txbuffer[33]; 
uint8_t NRF24L01_rxbuffer_pipe0[33];
uint8_t NRF24L01_rxbuffer_pipe1[33];
uint8_t NRF24L01_rxbuffer_pipe2[33];
uint8_t NRF24L01_rxbuffer_pipe3[33];
uint8_t NRF24L01_rxbuffer_pipe4[33];
uint8_t NRF24L01_rxbuffer_pipe5[33];
Target_State_t Target_State = {0};


/* CE and CSN pin control functions (static, internal use only) */
static void NRF24L01_CE_High(void)
{
    HAL_GPIO_WritePin(GPIOA, NRF_CE_Pin, GPIO_PIN_SET);
}

static void NRF24L01_CE_Low(void)
{
    HAL_GPIO_WritePin(GPIOA, NRF_CE_Pin, GPIO_PIN_RESET);
}

static void NRF24L01_CSN_High(void)
{
  HAL_GPIO_WritePin(GPIOA, NRF_CSN_Pin, GPIO_PIN_SET);
} 

static void NRF24L01_CSN_Low(void)
{
  HAL_GPIO_WritePin(GPIOA, NRF_CSN_Pin, GPIO_PIN_RESET);
}

/* Register read/write functions */
/**
  * @brief  Read NRF24L01 register(s)
  * @param  hspi: Pointer to SPI handle
  * @param  reg: Register address to read
  */
//static uint8_t NRF24L01_ReadReg(SPI_HandleTypeDef *hspi, uint8_t reg)
//{
//  if ( hspi == NULL) {
//    /* User can add his own implementation to report the file name and line number*/
//    return 0xFF;
//  }

//  static uint8_t CommandByte;
//  static uint8_t RegisterData;
//  CommandByte = NRF_CMD_R_REGISTER | (reg & 0x1F);
//  
//  NRF24L01_CSN_Low();
//  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
//	 printf("READ ERROR");
//    /* User can add his own implementation to report the file name and line number*/
//    NRF24L01_CSN_High();
//    return 0xFF;
//  }
//  if(HAL_SPI_Receive(hspi, &RegisterData, 1, 100) != HAL_OK) {
//	printf("READ ERROR");
//    /* User can add his own implementation to report the file name and line number*/
//    NRF24L01_CSN_High();
//    return 0xFF;  
//  }
//  NRF24L01_CSN_High();

//  return RegisterData;
//}

static uint8_t NRF24L01_ReadReg(SPI_HandleTypeDef *hspi, uint8_t reg)
{
  if (hspi == NULL) return 0xFF;

  uint8_t tx_buf[2];
  uint8_t rx_buf[2];
  
  // 构造读命令：R_REGISTER (000A AAAA)
  tx_buf[0] = NRF_CMD_R_REGISTER | (reg & 0x1F); 
  tx_buf[1] = 0xFF; // 发送 dummy 字节以读取寄存器值

  NRF24L01_CSN_Low();
  
  // 一次性全双工交换 2 个字节
  if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 2, 100) != HAL_OK) {
    printf("READ ERROR");
    NRF24L01_CSN_High();
    return 0xFF;
  }
  
  NRF24L01_CSN_High();

  // rx_buf[0] 是 STATUS 寄存器（可选）
  // rx_buf[1] 才是真正读取到的寄存器数据
  return rx_buf[1]; 
}


/**
  * @brief  Write NRF24L01 register(s)
  * @param  hspi: Pointer to SPI handle
  * @param  reg: Register address to write
  * @param  Vlaue: Value to write to the register
  */
static void NRF24L01_WriteReg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t Vlaue)
{
    if ( hspi == NULL) {
      /* User can add his own implementation to report the file name and line number*/
      return;
    }
    
    static uint8_t CommandByte;
    CommandByte = NRF_CMD_W_REGISTER | (reg & 0x1F);

    NRF24L01_CSN_Low();
    if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
		printf("SPI_Transmit error 1");
        /* User can add his own implementation to report the file name and line number*/
        NRF24L01_CSN_High();
        return;
    }
    if (HAL_SPI_Transmit(hspi, &Vlaue, 1, 100) != HAL_OK) {
        /* User can add his own implementation to report the file name and line number*/
		printf("SPI_Transmit error 2");
        NRF24L01_CSN_High();
        return;
    }
    NRF24L01_CSN_High();
}

/**
  * @brief  Read multiple bytes from NRF24L01 register
  * @param  hspi: Pointer to SPI handle
  * @param  reg: Register address to read
  * @param  data: Pointer to data buffer
  * @param  length: Number of bytes to read
  */
//static void NRF24L01_ReadMultiReg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t *data, uint8_t length)
//{
//  if (hspi == NULL || data == NULL || length == 0) {
//    /* User can add his own implementation to report the file name and line number,
//  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//    return;
//  }
//    
//  uint8_t CommandByte = NRF_CMD_R_REGISTER | (reg & 0x1F);
//    
//  NRF24L01_CSN_Low();
//  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
//	  printf("ReadMultiReg ERROR");
//      NRF24L01_CSN_High();
//      /* User can add his own implementation to report the file name and line number,
//    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//      return;
//  }
//  if (HAL_SPI_Receive(hspi, data, length, 100) != HAL_OK) {
//	  printf("ReadMultiReg ERROR");
//      NRF24L01_CSN_High();
//      /* User can add his own implementation to report the file name and line number,
//    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//      return;
//  }
//  NRF24L01_CSN_High();
//}

/**
  * @brief  Write multiple bytes to NRF24L01 register
  * @param  hspi: Pointer to SPI handle
  * @param  reg: Register address to write
  * @param  data: Pointer to data buffer
  * @param  length: Number of bytes to write
  */
static void NRF24L01_WriteMultiReg(SPI_HandleTypeDef *hspi, uint8_t reg, uint8_t *data, uint8_t length)
{
  if (hspi == NULL || data == NULL || length == 0) {
    /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    return;
  }
    
  uint8_t CommandByte = NRF_CMD_W_REGISTER | (reg & 0x1F);
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
	  printf("WriteMultiReg ERROR");
      NRF24L01_CSN_High();
      /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
      return;
  }
  if (HAL_SPI_Transmit(hspi, data, length, 100) != HAL_OK) {
	  printf("WriteMultiReg ERROR");
      NRF24L01_CSN_High();
      /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
      return;
  }
  NRF24L01_CSN_High();
}

void NRF24L01_FlushRxFIFO(SPI_HandleTypeDef *hspi)
{
  if ( hspi == NULL) {
    /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    return;
  }
    
  uint8_t CommandByte = NRF_CMD_FLUSH_RX;//清空接收缓冲区
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
	  printf("FlushRxFIFO ERROR");
  /* User can add his own implementation to report the file name and line number,
ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    NRF24L01_CSN_High();
    return;
  }
  NRF24L01_CSN_High();
}

void NRF24L01_FlashTxFIFO(SPI_HandleTypeDef *hspi)
{
  if ( hspi == NULL) {
    /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    return;
  }
    
  uint8_t CommandByte = NRF_CMD_FLUSH_TX;//清空发送缓冲区
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
	  printf("FlushTXFIFO ERROR");
  /* User can add his own implementation to report the file name and line number,
ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    NRF24L01_CSN_High();
    return;
  }
  NRF24L01_CSN_High();
}

/**
  * @brief  Set NRF24L01 operation mode
  * @param  hspi: Pointer to SPI handle
  * @param  mode: Operation mode to set
  */
void NRF24L01_SetMode(SPI_HandleTypeDef *hspi, NRF_ModeTypeDef mode)
{
  if (hspi == NULL) {
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
      return;
  }
  NRF24L01_CE_Low();

  static uint8_t config_reg;
  config_reg = NRF24L01_ReadReg(hspi, NRF_REG_CONFIG);
  
  switch (mode) {
    case NRF_MODE_POWER_DOWN:
        config_reg &= ~NRF_POWER_UP;  /* Clear PWR_UP bit */
        NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_reg);
        NRF24L01_CE_Low();
        break;
        
    case NRF_MODE_STANDBY_I:
        config_reg |= NRF_POWER_UP;   /* Set PWR_UP bit */
        NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_reg);
        NRF24L01_CE_Low();
        break;
        
    case NRF_MODE_RX:
        config_reg |= NRF_POWER_UP;   /* Set PWR_UP bit */
        config_reg |= NRF_PRIMARY_RX; /* Set PRIM_RX bit */
        NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_reg);
        // Clear RX_DR interrupt flag
        NRF24L01_WriteReg(hspi, NRF_REG_STATUS, NRF_STATUS_RX_DR); 
        //flash RX FIFO
        NRF24L01_FlushRxFIFO(hspi);

        NRF24L01_CE_High();
        HAL_Delay(1);
        break;
        
    case NRF_MODE_TX:
        config_reg |= NRF_POWER_UP;   /* Set PWR_UP bit */
        config_reg &= ~NRF_PRIMARY_RX; /* Clear PRIM_RX bit */
        NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_reg);
        // Clear TX_DS and NRF_STATUS_MAX_RT interrupt flags
        NRF24L01_WriteReg(hspi, NRF_REG_STATUS, NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
        //flash TX FIFO
        NRF24L01_FlashTxFIFO(hspi);

        NRF24L01_CE_High();
        HAL_Delay(1);
        break;
    default:
        break;
  }
}

/**
  * @brief  Get default configuration for NRF24L01+
  * @param  init: Pointer to initialization structure to fill with default values
  */
void NRF24L01_GetDefaultConfig(NRF_InitTypeDef *init)
{
  if (init == NULL) {
  /* User can add his own implementation to report the file name and line number,
ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    return;
  }
    
  /* CONFIG register defaults */
  init->InterruptConfig = NRF_INTERRUPT_TX_DS_DISABLE;  /* Disable TX data sent interrupt */
  init->CRCConfig = NRF_CRC_ENABLE;                     /* Enable CRC */
  init->CRCLength = NRF_CRC_LENGTH_2BYTE;               /* 2-byte CRC */
  init->Mode =NRF_MODE_STANDBY_I;                       /* Standby-I mode */
  
  /* EN_AA register defaults */
  init->AutoAckPipes = NRF_PIPE0_BIT;               /* Auto ACK for pipe 0 only */
  
  /* EN_RXADDR register defaults */
  init->RxPipes = NRF_PIPE0_BIT;                    /* Enable pipe 0 only */
  
  /* SETUP_AW register defaults */
  init->AddrWidth = NRF_ADDR_WIDTH_5BYTES;          /* 5-byte address width */
  
  /* SETUP_RETR register defaults */
  init->AutoRetransDelay = NRF_ARD_500US;           /* 500us retransmit delay */
  init->AutoRetransCount = NRF_ARC_5_RETRANS;       /* Up to 5 retransmits */
  
  /* RF_CH register defaults */
  init->RFChannel = 2;                              /* Channel 2 (2.402GHz) */
  
  /* RF_SETUP register defaults */
  init->DataRate = NRF_DATA_RATE_250KBPS;             /* 1Mbps data rate */
  init->RFPower = NRF_RF_POWER_0DBM;                /* 0dBm output power */
  
  /* DYNPD register defaults */
  init->DynamicPayloadPipes = NRF_PIPE0_BIT;        /* Dynamic payload for pipe 0 */
  
  /* FEATURE register defaults */
  init->Features = NRF_FEATURE_EN_DPL;              /* Enable dynamic payload length */
  
  /* Payload width defaults */
  init->PayloadWidthPipe0 = 32;                     /* 32 bytes for pipe 0 */
  init->PayloadWidthPipe1 = 0;                      /* Pipe 1 not used */
  init->PayloadWidthPipe2 = 0;                      /* Pipe 2 not used */
  init->PayloadWidthPipe3 = 0;                      /* Pipe 3 not used */
  init->PayloadWidthPipe4 = 0;                      /* Pipe 4 not used */
  init->PayloadWidthPipe5 = 0;                      /* Pipe 5 not used */
  
  /* Address defaults */
  uint8_t defaultTxAddr[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  uint8_t defaultRxAddr0[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  uint8_t defaultRxAddr1[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
  
  memcpy(init->TxAddress, defaultTxAddr, 5);
  memcpy(init->RxAddressPipe0, defaultRxAddr0, 5);
  memcpy(init->RxAddressPipe1, defaultRxAddr1, 5);
  init->RxAddressPipe2 = 0xC3;
  init->RxAddressPipe3 = 0xC4;
  init->RxAddressPipe4 = 0xC5;
  init->RxAddressPipe5 = 0xC6;
  
}

/**
  * @brief  Initialize NRF24L01+ module with configuration structure,pull dowm the CE pin
  * @param  hspi: Pointer to SPI handle
  * @param  init: Pointer to initialization structure
  * @note   This function replaces the old NRF24L01_Init function
  */
void NRF24L01_Init(SPI_HandleTypeDef *hspi, NRF_InitTypeDef *init)
{
  if (hspi == NULL || init == NULL) {
    /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	  printf("INIT_ERROR");
    return;
  }
  
  HAL_Delay(100);
  NRF24L01_CE_Low();

  /* Build CONFIG register value from individual configuration fields */
  uint8_t config_value = 0;
  config_value |= init->InterruptConfig;    /* MASK_RX_DR, MASK_TX_DS, MASK_MAX_RT */
  config_value |= init->CRCConfig;          /* EN_CRC */
  config_value |= init->CRCLength;          /* CRCO */
  
  /* Write configuration registers */
  NRF24L01_WriteReg(hspi, NRF_REG_CONFIG, config_value);
  NRF24L01_WriteReg(hspi, NRF_REG_EN_AA, init->AutoAckPipes);
  NRF24L01_WriteReg(hspi, NRF_REG_EN_RXADDR, init->RxPipes);
  NRF24L01_WriteReg(hspi, NRF_REG_SETUP_AW, init->AddrWidth);
  
  /* Build SETUP_RETR register value */
  uint8_t setup_retr_value = init->AutoRetransDelay | init->AutoRetransCount;
  NRF24L01_WriteReg(hspi, NRF_REG_SETUP_RETR, setup_retr_value);
  
  /* Write RF channel */
  NRF24L01_WriteReg(hspi, NRF_REG_RF_CH, init->RFChannel);
  
  /* Build RF_SETUP register value */
  uint8_t rf_setup_value = init->DataRate | init->RFPower;
  NRF24L01_WriteReg(hspi, NRF_REG_RF_SETUP, rf_setup_value);
  
  /* Write dynamic payload and feature registers */
  NRF24L01_WriteReg(hspi, NRF_REG_DYNPD, init->DynamicPayloadPipes);
  NRF24L01_WriteReg(hspi, NRF_REG_FEATURE, init->Features);
  
  /* Clear status register */
  NRF24L01_WriteReg(hspi, NRF_REG_STATUS, 0x70);
  
  /* Flush RX FIFO */
  NRF24L01_FlushRxFIFO(hspi);
  
  /* Write payload widths */
  if (init->PayloadWidthPipe0 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P0, init->PayloadWidthPipe0);
  }
  if (init->PayloadWidthPipe1 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P1, init->PayloadWidthPipe1);
  }
  if (init->PayloadWidthPipe2 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P2, init->PayloadWidthPipe2);
  }
  if (init->PayloadWidthPipe3 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P3, init->PayloadWidthPipe3);
  }
  if (init->PayloadWidthPipe4 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P4, init->PayloadWidthPipe4);
  }
  if (init->PayloadWidthPipe5 > 0) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_PW_P5, init->PayloadWidthPipe5);
  }
  
  /* Write addresses */
  /* Determine actual address length based on AddrWidth */
  uint8_t addr_length = 5;
  if (init->AddrWidth == NRF_ADDR_WIDTH_3BYTES) {
      addr_length = 3;
  } else if (init->AddrWidth == NRF_ADDR_WIDTH_4BYTES) {
      addr_length = 4;
  }
  
  /* Write TX address */
  NRF24L01_WriteMultiReg(hspi, NRF_REG_TX_ADDR, init->TxAddress, addr_length);
  
  /* Write RX addresses for enabled pipes */
  if (init->RxPipes & NRF_PIPE0_BIT) {
      NRF24L01_WriteMultiReg(hspi, NRF_REG_RX_ADDR_P0, init->RxAddressPipe0, addr_length);
  }
  if (init->RxPipes & NRF_PIPE1_BIT) {
      NRF24L01_WriteMultiReg(hspi, NRF_REG_RX_ADDR_P1, init->RxAddressPipe1, addr_length);
  }
  if (init->RxPipes & NRF_PIPE2_BIT) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_ADDR_P2, init->RxAddressPipe2);
  }
  if (init->RxPipes & NRF_PIPE3_BIT) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_ADDR_P3, init->RxAddressPipe3);
  }
  if (init->RxPipes & NRF_PIPE4_BIT) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_ADDR_P4, init->RxAddressPipe4);
  }
  if (init->RxPipes & NRF_PIPE5_BIT) {
      NRF24L01_WriteReg(hspi, NRF_REG_RX_ADDR_P5, init->RxAddressPipe5);
  }
  /* Set initial mode */
  NRF24L01_SetMode(hspi, init->Mode);
}

/**
  * @brief  Transmit data via NRF24L01
  * @param  hspi: Pointer to SPI handle
  * @param  length: Length of data (1-32 bytes)
  * @note   NRF24L01 supports maximum 32 bytes payload in variable length mode
  */
void NRF24L01_WriteTxFIFO(SPI_HandleTypeDef *hspi,uint32_t Timeout)
{
  static uint32_t tickstart;
  static uint8_t CommandByte;

  /* parameter check function */
  if (hspi == NULL || NRF24L01_txbuffer[32] == 0 || NRF24L01_txbuffer[32] > 32) {
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    return;
  }

  /* Wait for the TxFIFO to be available */
  tickstart = HAL_GetTick();
  while(NRF24L01_IsTxFIFOFull(hspi)){
    if((HAL_GetTick() - tickstart ) > Timeout){
      /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
      return;
    }
  };


  /* Write to TxFIFO */
  CommandByte = NRF_CMD_W_TX_PAYLOAD;
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    NRF24L01_CSN_High();
    return;
  }
  if (HAL_SPI_Transmit(hspi, NRF24L01_txbuffer, NRF24L01_txbuffer[32], 100) != HAL_OK) {
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */  
    NRF24L01_CSN_High();
    return;
  }
  NRF24L01_CSN_High();
}

/**
  * @brief  Receive data via NRF24L01
  * @param  hspi: Pointer to SPI handle
  * @note   Buffer contains only the actual received data from rxFIFO
  *         Length parameter stores the number of bytes received
  */
void NRF24L01_ReadRxFIFO(SPI_HandleTypeDef *hspi)
{
  static uint8_t cmd;
  static uint8_t Rx_payload_width;
  static uint8_t *NRF24L01_rxbuffer;

  /* Parameter check function */
  if (hspi == NULL) {
      /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    return ;
  }


  /* Read all available payloads in RX FIFO */
  while(!NRF24L01_IsRxFIFOEmpty(hspi)){
    /* Read pipe num */
    switch (NRF24L01_GetPipeNum(hspi)) {
      case 0:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe0;
          break;
      case 1:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe1;
          break;
      case 2:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe2;
          break;
      case 3:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe3;
          break;
      case 4:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe4;
          break;
      case 5:
          NRF24L01_rxbuffer = NRF24L01_rxbuffer_pipe5;
          break;
      default:
          /* Invalid pipe number, should not happen */
          return ;
    }

    /* Read payload width */
    NRF24L01_CSN_Low();
    cmd = NRF_CMD_R_RX_PL_WID;
    if (HAL_SPI_Transmit(hspi, &cmd, 1, 100) != HAL_OK) {
        NRF24L01_CSN_High();
        /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
        return ;
    }
    if (HAL_SPI_Receive(hspi, &Rx_payload_width, 1, 100) != HAL_OK) {
        NRF24L01_CSN_High();
        /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
        return ;
    }
    NRF24L01_CSN_High();
    
    /* Read the actual payload data */
    NRF24L01_CSN_Low();
    cmd = NRF_CMD_R_RX_PAYLOAD;
    if (HAL_SPI_Transmit(hspi, &cmd, 1, 100) != HAL_OK) {
        NRF24L01_CSN_High();
        /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
        return ;
    }
    if (HAL_SPI_Receive(hspi, NRF24L01_rxbuffer, Rx_payload_width, 100) != HAL_OK) {
        NRF24L01_CSN_High();
        /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
        return ;
    }
    NRF24L01_CSN_High();
    NRF24L01_rxbuffer[32] = Rx_payload_width;  /* Store payload length in the last byte */
  }
}

/**
  * @brief  Get NRF24L01 status register value
  * @param  hspi: Pointer to SPI handle
  * @retval Status register value
  * @note   This function reads the STATUS register (0x07)
  *         The register contains the following flags:
  *         - Bit 6: RX_DR - RX data ready flag
  *         - Bit 5: TX_DS - TX data sent flag
  *         - Bit 4: MAX_RT - Maximum retransmit count reached flag
  *         - Bits 3:1: RX_P_NO - RX pipe number for the payload available in RX FIFO
  */
uint8_t NRF24L01_GetStatus(SPI_HandleTypeDef *hspi)
{
  return NRF24L01_ReadReg(hspi, NRF_REG_STATUS);
}

/**
  * @brief  Get NRF24L01 FIFO status register value
  * @param  hspi: Pointer to SPI handle
  * @retval FIFO status register value
  * @note   This function reads the FIFO_STATUS register (0x17)
  *         The register contains the following flags:
  *         - Bit 6: TX_REUSE - TX reuse flag
  *         - Bit 5: TX_FULL - TX FIFO full flag (FIFO_STATUS register)
  *         - Bit 4: TX_EMPTY - TX FIFO empty flag
  *         - Bit 1: RX_FULL - RX FIFO full flag
  *         - Bit 0: RX_EMPTY - RX FIFO empty flag
  */
uint8_t NRF24L01_GetFIFOStatus(SPI_HandleTypeDef *hspi)
{
  return NRF24L01_ReadReg(hspi, NRF_REG_FIFO_STATUS);
}

/**
  * @brief  Check if RX data is ready (RX_DR flag)
  * @param  hspi: Pointer to SPI handle
  * @retval 1 if RX data is ready, 0 otherwise
  */
uint8_t NRF24L01_IsRxDataReady(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_RX_DR) ? 1 : 0;
}

/**
  * @brief  Check if TX data has been sent (TX_DS flag)
  * @param  hspi: Pointer to SPI handle
  * @retval 1 if TX data has been sent, 0 otherwise
  */
uint8_t NRF24L01_IsTxDataSent(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_TX_DS) ? 1 : 0;
}

/**
  * @brief  Check if maximum retransmit count reached (MAX_RT flag)
  * @param  hspi: Pointer to SPI handle
  * @retval 1 if maximum retransmit count reached, 0 otherwise
  */
uint8_t NRF24L01_IsMaxRetransmit(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_MAX_RT) ? 1 : 0;
}

/**
  * @brief  Get RX pipe number for the payload available in RX FIFO
  * @param  hspi: Pointer to SPI handle
  * @retval Pipe number (0-5) or 7 if RX FIFO is empty
  * @note   Returns the pipe number from which the next available payload was received
  *         Bits 3:1 of status register contain the pipe number
  *         Value 7 indicates RX FIFO is empty
  */
uint8_t NRF24L01_GetRxPipeNumber(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_RX_P_NO) >> 1;
}

/**
  * @brief  Check if TX FIFO is full (TX_FULL flag in STATUS register)
  * @param  hspi: Pointer to SPI handle
  * @retval 1 if TX FIFO is full, 0 otherwise
  * @note   This function checks the TX_FULL flag in STATUS register (bit 0)
  *         For FIFO_STATUS register TX_FULL flag (bit 5), use NRF24L01_GetFIFOStatus function
  */
uint8_t NRF24L01_IsTxFIFOFull(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_TX_FULL) ? 1 : 0;
}

/**
  * @brief  Check if RX FIFO is empty (RX_EMPTY flag in FIFO_STATUS register)
  * @param  hspi: Pointer to SPI handle
  * @retval 1 if RX FIFO is empty, 0 otherwise
  */
uint8_t NRF24L01_IsRxFIFOEmpty(SPI_HandleTypeDef *hspi)
{
  uint8_t fifo_status = NRF24L01_GetFIFOStatus(hspi);
  return (fifo_status & NRF_FIFO_STATUS_RX_EMPTY) ? 1 : 0;
}

/**
  * @brief  Get RX pipe number for the payload available in RX FIFO
  * @retval Pipe number (0-5) or 7 if RX FIFO is empty
  * @note   Returns the pipe number from which the next available payload was received
  *         Bits 3:1 of status register contain the pipe number
  *         Value 7 indicates RX FIFO is empty
  */
uint8_t NRF24L01_GetPipeNum(SPI_HandleTypeDef *hspi)
{
  uint8_t status = NRF24L01_GetStatus(hspi);
  return (status & NRF_STATUS_RX_P_NO) >> 1;
}


/**
  * @brief  Clear specific interrupt flags in status register
  * @param  hspi: Pointer to SPI handle
  * @param  flags: Interrupt flags to clear (bitwise OR of NRF_STATUS_RX_DR, NRF_STATUS_TX_DS, NRF_STATUS_MAX_RT)
  *         This parameter can be a value or bitwise OR of values from @ref NRF24L01_Status_Register_Bits
  *         Only NRF_STATUS_RX_DR, NRF_STATUS_TX_DS, and NRF_STATUS_MAX_RT are valid for clearing
  * @note   Writing 1 to interrupt bits in status register clears them
  *         Example: NRF24L01_ClearInterruptFlags(hspi, NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT)
  */
void NRF24L01_ClearInterruptFlags(SPI_HandleTypeDef *hspi, uint8_t flags)
{
  /* Only clear RX_DR, TX_DS, and MAX_RT bits (bits 6, 5, 4) */
  uint8_t flags_to_clear = flags & (NRF_STATUS_RX_DR | NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
  if (flags_to_clear != 0) {
    NRF24L01_WriteReg(hspi, NRF_REG_STATUS, flags_to_clear);
  }
}



void NRF24L01_PrintPipe0ReceivedData(void)
{
    // 核心判断：接收数据长度大于0（有效数据）
    if(NRF24L01_rxbuffer_pipe0[32] > 0)
    {
        uint8_t data_len = NRF24L01_rxbuffer_pipe0[32]; // 提取接收数据长度
        uint8_t *rx_data = NRF24L01_rxbuffer_pipe0;     // 指向接收数据缓冲区
        
        printf("ASCII格式");
        for(uint8_t i = 0; i < data_len; i++)
        {
            // 只打印可显示ASCII字符（32~126），不可显示的用'.'代替
            if(rx_data[i] >= 32 && rx_data[i] <= 126)
            {
                printf("%c", rx_data[i]);
            }
            else
            {
                printf(".");
            }
        }
        // 可选：打印完成后清空缓冲区长度（避免重复打印同一份数据）
        NRF24L01_rxbuffer_pipe0[32] = 0;
    }
    else
    {
        // 无有效数据时可选择打印提示（或注释掉，减少日志刷屏）
        printf("NRF24L01 管道0无有效接收数据\n");
    }
}

void NRF24L01_data_care()
{
	 char temp_str_buf[33];
	
	 if(NRF24L01_rxbuffer_pipe0[32] > 0)
	 {
		uint8_t data_len = NRF24L01_rxbuffer_pipe0[32]; // 提取接收数据长度
        uint8_t *rx_data = NRF24L01_rxbuffer_pipe0;     // 指向接收数据缓冲区
		memcpy(temp_str_buf, rx_data, data_len);
		// printf("RX RAW: %s", rx_data); 
		temp_str_buf[data_len] = '\0';
		char *token = strtok(temp_str_buf, " ");
        if(token != NULL)  // 解析第一个数
        {
            
            Target_State.target_speed = atof(token);  // atoi自动处理有符号数（负数也能解析）
            token = strtok(NULL, " ");  // 继续分割下一个

            if(token != NULL)  // 解析第二个数
            {
                Target_State.target_roll = atof(token);
                token = strtok(NULL, " ");  // 继续分割下一个

                if(token != NULL)  // 解析第三个数
                {
                    Target_State.target_pitch = atof(token);
                }
            }
        }
	 }
}

// void printf(const char *fmt, ...)
// {
//     char log_buf[LOG_BUFFER_SIZE] = {0};
//     va_list args;          
//     va_start(args, fmt);   
//     vsnprintf(log_buf, LOG_BUFFER_SIZE - 1, fmt, args);
//     va_end(args);          
//     HAL_UART_Transmit(&huart1, (uint8_t *)log_buf, strlen(log_buf), HAL_MAX_DELAY);
// }


// 接收端 · 无发送端 单端自检测试
void NRF24L01_RX_SelfTest_NoSender(void)
{
    uint8_t reg_val;
    uint8_t status_val;

    printf("\n=== NRF24L01 接收端 单端自测开始 ===\r\n");
    HAL_Delay(100);
    reg_val = NRF24L01_ReadReg(&hspi2, 0x00); // 读 CONFIG 寄存器
    printf("上电默认 CONFIG: 0x%02X\r\n", reg_val);
    NRF24L01_WriteReg(&hspi2, 0x05, 66); // 把 RF_CH 写成 66
    reg_val = NRF24L01_ReadReg(&hspi2, 0x05);
    printf("写RF_CH=66后读到: %d\r\n", reg_val);
    NRF_InitTypeDef nrf_init;
    NRF24L01_GetDefaultConfig(&nrf_init);
    nrf_init.RFChannel    = 66;
    nrf_init.Mode         = NRF_MODE_STANDBY_I;
    nrf_init.DataRate     = NRF_DATA_RATE_1MBPS;
    nrf_init.RFPower      = NRF_RF_POWER_0DBM;

    NRF24L01_Init(&hspi2, &nrf_init);
    printf("NRF24L01 初始化完成\r\n");
    NRF24L01_SetMode(&hspi2, NRF_MODE_RX);
    status_val = NRF24L01_GetStatus(&hspi2);
    printf("切到RX模式后 STATUS: 0x%02X\r\n", status_val);
    NRF24L01_FlushRxFIFO(&hspi2);
    printf("清空RX FIFO 完成\r\n");
    printf("\n=== 自测结果总结 ===\r\n");
    printf("1. SPI通信:    %s\r\n", (reg_val == 66) ? "正常" : "异常");
    printf("2. 芯片存活:   %s\r\n", (status_val != 0xFF && status_val != 0x00) ? "正常" : "异常");
    printf("3. 进入RX模式: 已完成\r\n");
    printf("4. 接收端硬件: %s\r\n", (reg_val == 66) ? "可以使用" : "请检查接线/电源");
}
// void NRF24L01_RX_SelfTest_Enhanced(void)
// {
//     uint8_t reg_val;
//     uint8_t status_val;
//     uint8_t test_val = 0xAA;
//     uint8_t spi_ok = 1;
//     uint8_t chip_alive = 1;

//     printf("\n=== NRF24L01 Receiver Enhanced Self-Test Start ===\r\n");
//     HAL_Delay(100);

//     // Test 1: Read multiple default registers to verify SPI read function
//     printf("\n--- Test 1: Read Default Register Values ---\r\n");
//     reg_val = NRF24L01_ReadReg(&hspi2, 0x00); // CONFIG register, default 0x08
//     printf("CONFIG(0x00) Default: 0x%02X (Expected: 0x08)\r\n", reg_val);
//     if(reg_val != 0x08) spi_ok = 0;

//     reg_val = NRF24L01_ReadReg(&hspi2, 0x07); // STATUS register, default 0x0E
//     printf("STATUS(0x07) Default: 0x%02X (Expected: 0x0E)\r\n", reg_val);
//     if(reg_val != 0x0E) spi_ok = 0;

//     reg_val = NRF24L01_ReadReg(&hspi2, 0x05); // RF_CH register, default 0x02
//     printf("RF_CH(0x05) Default: 0x%02X (Expected: 0x02)\r\n", reg_val);
//     if(reg_val != 0x02) spi_ok = 0;

//     reg_val = NRF24L01_ReadReg(&hspi2, 0x06); // RF_SETUP register, default 0x0F
//     printf("RF_SETUP(0x06) Default: 0x%02X (Expected: 0x0F)\r\n", reg_val);
//     if(reg_val != 0x0F) spi_ok = 0;

//     // Test 2: Write register then read back to verify SPI write function
//     printf("\n--- Test 2: SPI Read/Write Verification ---\r\n");
//     NRF24L01_WriteReg(&hspi2, 0x05, 66); // Write RF_CH to 66
//     reg_val = NRF24L01_ReadReg(&hspi2, 0x05);
//     printf("After writing RF_CH=66, read back: %d\r\n", reg_val);
//     if(reg_val != 66) spi_ok = 0;

//     NRF24L01_WriteReg(&hspi2, 0x00, test_val); // Write CONFIG to 0xAA
//     reg_val = NRF24L01_ReadReg(&hspi2, 0x00);
//     printf("After writing CONFIG=0x%02X, read back: 0x%02X\r\n", test_val, reg_val);
//     if(reg_val != test_val) spi_ok = 0;

//     // Restore CONFIG default value
//     NRF24L01_WriteReg(&hspi2, 0x00, 0x08);

//     // Test 3: Chip alive check
//     printf("\n--- Test 3: Chip Alive Check ---\r\n");
//     status_val = NRF24L01_ReadReg(&hspi2, 0x07);
//     printf("Current STATUS: 0x%02X\r\n", status_val);
//     if(status_val == 0xFF || status_val == 0x00) 
//     {
//         chip_alive = 0;
//         printf("WARNING: STATUS register is 0x%02X, chip may not respond or SPI communication failed completely\r\n", status_val);
//     }

//     // Test 4: CE pin level check
//     printf("\n--- Test 4: CE Pin Level Check ---\r\n");
//     GPIO_PinState ce_state = HAL_GPIO_ReadPin(GPIOA, NRF_CE_Pin);
//     printf("Current CE pin level: %s\r\n", ce_state == GPIO_PIN_SET ? "HIGH" : "LOW");

//     // Initialize and switch to RX mode
//     printf("\n--- Initialize and Switch to RX Mode ---\r\n");
//     NRF_InitTypeDef nrf_init;
//     NRF24L01_GetDefaultConfig(&nrf_init);
//     nrf_init.RFChannel    = 66;
//     nrf_init.Mode         = NRF_MODE_STANDBY_I;
//     nrf_init.DataRate     = NRF_DATA_RATE_1MBPS;
//     nrf_init.RFPower      = NRF_RF_POWER_0DBM;

//     NRF24L01_Init(&hspi2, &nrf_init);
//     printf("NRF24L01 initialization completed\r\n");

//     NRF24L01_SetMode(&hspi2, NRF_MODE_RX);
//     printf("Switched to RX mode\r\n");

//     ce_state = HAL_GPIO_ReadPin(GPIOA, NRF_CE_Pin);
//     printf("CE pin level in RX mode: %s (Should be HIGH)\r\n", ce_state == GPIO_PIN_SET ? "HIGH" : "LOW");

//     NRF24L01_FlushRxFIFO(&hspi2);
//     printf("RX FIFO flushed\r\n");

//     // Final result summary
//     printf("\n=== Enhanced Self-Test Result Summary ===\r\n");
//     printf("1. SPI Communication: %s\r\n", spi_ok ? "OK" : "FAILED");
//     printf("2. Chip Alive:        %s\r\n", chip_alive ? "OK" : "FAILED");
//     printf("3. CE Pin:            %s\r\n", ce_state == GPIO_PIN_SET ? "OK (HIGH in RX mode)" : "FAILED (Should be HIGH)");
//     printf("4. Hardware Status:   ");
//     if(spi_ok && chip_alive && ce_state == GPIO_PIN_SET)
//     {
//         printf("OK, ready for communication test\r\n");
//     }
//     else if(!spi_ok)
//     {
//         printf("FAILED, please check SPI configuration, CSN pin wiring and driver functions\r\n");
//     }
//     else if(!chip_alive)
//     {
//         printf("FAILED, please check power supply, VCC wiring and the chip itself\r\n");
//     }
//     else
//     {
//         printf("FAILED, please check CE pin wiring and SetMode function implementation\r\n");
//     }
//     printf("===========================================\r\n");
// }
