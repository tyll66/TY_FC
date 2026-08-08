#include "main.h"
#include "NRF24L01.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "ssd1306.h"

#define LOG_BUFFER_SIZE  512
#define OLED_LOG_ROWS 4
#define OLED_LOG_COLS 21
static char oled_log_buf[OLED_LOG_ROWS][OLED_LOG_COLS + 1] = {0};
static uint8_t oled_log_row_idx = 0;

/* Public global data buffers ,32 bytes data + 1 byte data_length*/
uint8_t NRF24L01_txbuffer[33]; 
uint8_t NRF24L01_rxbuffer_pipe0[33];
uint8_t NRF24L01_rxbuffer_pipe1[33];
uint8_t NRF24L01_rxbuffer_pipe2[33];
uint8_t NRF24L01_rxbuffer_pipe3[33];
uint8_t NRF24L01_rxbuffer_pipe4[33];
uint8_t NRF24L01_rxbuffer_pipe5[33];


// 地址长度（默认5字节，NRF24L01+推荐用5字节，兼容性最好）
#define TX_ADR_WIDTH    5
#define RX_ADR_WIDTH    5

// 发送/接收地址（必须完全一致，自定义即可，比如0x01,0x02,0x03,0x04,0x05）
uint8_t TX_ADDRESS[TX_ADR_WIDTH] = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t RX_ADDRESS[RX_ADR_WIDTH] = {0x01, 0x02, 0x03, 0x04, 0x05};


void NRF_LOGI(const char *fmt, ...);
static void OLED_Log_Clear(void);

/* CE and CSN pin control functions (static, internal use only) */
void NRF24L01_CE_High(void)
{
    HAL_GPIO_WritePin(GPIOA, NRF_CE_Pin, GPIO_PIN_SET);
}

void NRF24L01_CE_Low(void)
{
    HAL_GPIO_WritePin(GPIOA, NRF_CE_Pin, GPIO_PIN_RESET);
}

void NRF24L01_CSN_High(void)
{
  HAL_GPIO_WritePin(GPIOA, NRF_CSN_Pin, GPIO_PIN_SET);
} 

void NRF24L01_CSN_Low(void)
{
  HAL_GPIO_WritePin(GPIOA, NRF_CSN_Pin, GPIO_PIN_RESET);
}

/* Register read/write functions */
/**
  * @brief  Read NRF24L01 register(s)
  * @param  hspi: Pointer to SPI handle
  * @param  reg: Register address to read
  */
static uint8_t NRF24L01_ReadReg(SPI_HandleTypeDef *hspi, uint8_t reg)
{
  if ( hspi == NULL) {
    /* User can add his own implementation to report the file name and line number*/
    return 0xFF;
  }

  static uint8_t CommandByte;
  static uint8_t RegisterData;
  CommandByte = NRF_CMD_R_REGISTER | (reg & 0x1F);
  
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
    /* User can add his own implementation to report the file name and line number*/
    NRF24L01_CSN_High();
    return 0xFF;
  }
  if(HAL_SPI_Receive(hspi, &RegisterData, 1, 100) != HAL_OK) {
    /* User can add his own implementation to report the file name and line number*/
    NRF24L01_CSN_High();
    return 0xFF;  
  }
  NRF24L01_CSN_High();

  return RegisterData;
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
        /* User can add his own implementation to report the file name and line number*/
        NRF24L01_CSN_High();
        return;
    }
    if (HAL_SPI_Transmit(hspi, &Vlaue, 1, 100) != HAL_OK) {
        /* User can add his own implementation to report the file name and line number*/
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
//      NRF24L01_CSN_High();
//      /* User can add his own implementation to report the file name and line number,
//    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//      return;
//  }
//  if (HAL_SPI_Receive(hspi, data, length, 100) != HAL_OK) {
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
      NRF24L01_CSN_High();
      /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
      return;
  }
  if (HAL_SPI_Transmit(hspi, data, length, 100) != HAL_OK) {
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
    
  uint8_t CommandByte = NRF_CMD_FLUSH_RX;
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
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
    
  uint8_t CommandByte = NRF_CMD_FLUSH_TX;
    
  NRF24L01_CSN_Low();
  if (HAL_SPI_Transmit(hspi, &CommandByte, 1, 100) != HAL_OK) {
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
        HAL_Delay(10);
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
  //NRF24L01_CE_Low();
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
  NRF24L01_CE_High();
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
//  return NRF24L01_ReadReg(hspi, NRF_REG_STATUS);
	uint8_t status;
    uint8_t cmd = 0xFF;
	NRF24L01_CE_Low();
    NRF24L01_CSN_Low();
    HAL_SPI_TransmitReceive(hspi, &cmd, &status, 1, 100);
    NRF24L01_CSN_High();
	NRF24L01_CE_High();

    return status;
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
	NRF_LOGI("STATUS: 0x%02X", status); 
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

// 接收端 · 无发送端 单端自检测试
void NRF24L01_RX_SelfTest_NoSender(void)
{
    uint8_t reg_val;
    uint8_t status_val;

    // 1. 上电延时
    HAL_Delay(100);

    // -------------------------------------------------------------------------
    // 第一步：读芯片默认寄存器，判断SPI是否通、芯片是否活着
    // -------------------------------------------------------------------------
    reg_val = NRF24L01_ReadReg(&hspi1, 0x00); // 读 CONFIG 寄存器
    NRF_LOGI("CONFIG: 0x%02X\r\n", reg_val);

    // 正常NRF24L01上电默认是 0x08 或 0x09
    // 如果你读到 0x00 / 0xFF → SPI 或接线有问题


    // -------------------------------------------------------------------------
    // 第二步：写一个寄存器，再读回来（最稳的SPI测试）
    // -------------------------------------------------------------------------
    NRF24L01_WriteReg(&hspi1, 0x05, 66); // 把 RF_CH 写成 66
    reg_val = NRF24L01_ReadReg(&hspi1, 0x05);
    NRF_LOGI("RF_CH=66: %d\r\n", reg_val);

    // 结果 = 66 → SPI 100% 正常


    // -------------------------------------------------------------------------
    // 第三步：使用你自己的驱动正常初始化
    // -------------------------------------------------------------------------
    NRF_InitTypeDef nrf_init;
    NRF24L01_GetDefaultConfig(&nrf_init);

    // 接收端配置
    nrf_init.RFChannel    = 66;
    nrf_init.Mode         = NRF_MODE_STANDBY_I;
    nrf_init.DataRate     = NRF_DATA_RATE_1MBPS;
    nrf_init.RFPower      = NRF_RF_POWER_0DBM;

    NRF24L01_Init(&hspi1, &nrf_init);
    NRF_LOGI("NRF24L01initfinish\r\n");


    // -------------------------------------------------------------------------
    // 第四步：切换到 RX 接收模式（自测核心）
    // -------------------------------------------------------------------------
    NRF24L01_SetMode(&hspi1, NRF_MODE_RX);
    status_val = NRF24L01_GetStatus(&hspi1);
    NRF_LOGI("RXSTATUS: 0x%02X\r\n", status_val);

	
    // -------------------------------------------------------------------------
    // 第五步：清空RX FIFO（测试命令能否执行）
    // -------------------------------------------------------------------------
    NRF24L01_FlushRxFIFO(&hspi1);
    NRF_LOGI("RX FIFO\r\n");


    // -------------------------------------------------------------------------
    // 最终结论
    // -------------------------------------------------------------------------
    NRF_LOGI("SPI:    %s\r\n", (reg_val == 66) ? "y" : "n");
    NRF_LOGI("chip:   %s\r\n", (status_val != 0xFF && status_val != 0x00) ? "y" : "n");
    NRF_LOGI("RX: ok\r\n");
    NRF_LOGI("hand: %s\r\n", (reg_val == 66) ? "y" : "n");
}


/**
 * @brief NRF24L01+ 无接收端发送自测函数
 * @note 核心逻辑：配置发送模式→写入测试数据→检测MAX_RT（重传超时），验证发送功能
 */
//void NRF24L01_TX_SelfTest_NoReceiver(void)
//{
//    uint8_t reg_val;
//    uint8_t reg_vall;
//    uint8_t reg_vall0;
//    uint8_t reg_vall1;
//    uint8_t status_val;
//    uint8_t fifo_status;
//    uint32_t tick_start;
//    
//    // 1. 上电延时，确保芯片稳定启动（200ms足够覆盖芯片复位时序）
//	NRF24L01_CE_Low();
//    NRF24L01_CSN_High();
//    HAL_Delay(1000);
//    NRF_LOGI("TX Self Test Start");
//    
//    // -------------------------------------------------------------------------
//    // 第一步：读取默认寄存器，判断SPI通信是否正常、芯片是否存活
//    // -------------------------------------------------------------------------
//    // 读取SETUP_RETR寄存器（默认值0x15）
//    reg_val = NRF24L01_ReadReg(&hspi1, NRF_REG_SETUP_RETR);
//    NRF_LOGI("SETUP_RETR: 0x%02X", reg_val);
//    
//    // 读取CONFIG寄存器（默认值0x08/0x09）
//    reg_val = NRF24L01_ReadReg(&hspi1, NRF_REG_CONFIG);
//    NRF_LOGI("CONFIG(default): 0x%02X", reg_val);
//    // 异常判断：读到0x00/0xFF说明SPI硬件通信故障
//    if(reg_val == 0x00 || reg_val == 0xFF)
//    {
//        NRF_LOGI("error");
//        return; // 通信异常，提前退出
//    }

//    // -------------------------------------------------------------------------
//    // 第二步：验证SPI读写功能（写RF_CH再读回，最可靠的硬件测试）
//    // -------------------------------------------------------------------------
//    NRF24L01_WriteReg(&hspi1, NRF_REG_RF_CH, 66); // 写入RF频道66
//    reg_val = NRF24L01_ReadReg(&hspi1, NRF_REG_RF_CH);
//    NRF_LOGI("RF_CH: %d (expect 66)", reg_val);
//    // SPI通信验证：读回值≠66则通信异常
//    if(reg_val != 66)
//    {
//        NRF_LOGI("Write Failed!");
//        return;
//    }

//    // -------------------------------------------------------------------------
//    // 第三步：初始化NRF24L01为基础发送模式配置
//    // -------------------------------------------------------------------------
//     NRF_InitTypeDef nrf_init;
//    // 先获取驱动默认配置（符合头文件说明的默认值）
//    NRF24L01_GetDefaultConfig(&nrf_init);

//    // 基于默认配置修改TX自测所需参数（仅修改差异项）
//    nrf_init.InterruptConfig    = NRF_INTERRUPT_ALL_ENABLE;  // 启用所有中断（检测MAX_RT/TX_DS）
//    nrf_init.Mode               = NRF_MODE_STANDBY_I;        // 初始待机I模式
//    nrf_init.RFChannel          = 66;                        // 射频频道66（2466MHz）
//    nrf_init.DataRate           = NRF_DATA_RATE_250KBPS;     // 250kbps低速更稳定
//    nrf_init.RFPower            = NRF_RF_POWER_0DBM;         // 0dBm最大功率（头文件值0x06）
//    nrf_init.AutoRetransDelay   = NRF_ARD_500US;             // 500us重传延时（匹配默认）
//    nrf_init.AutoRetransCount   = NRF_ARC_5_RETRANS;         // 5次重传（匹配默认）
//    nrf_init.CRCLength          = NRF_CRC_LENGTH_2BYTE;      // 2字节CRC（头文件枚举名）
//	nrf_init.AutoAckPipes       = NRF_PIPE0_BIT;     // 必须开启PIPE0自动应答
//    nrf_init.RxPipes            = NRF_PIPE0_BIT;
//    // 地址配置：沿用默认的TX/RX地址（0xAA,0xBB,0xCC,0xDD,0xEE），无需修改
//    // 动态载荷：沿用默认（PIPE0启用动态载荷）

//    // 调用驱动初始化函数（一次性写入所有配置）
//    NRF24L01_Init(&hspi1, &nrf_init);
//    NRF_LOGI("NRF24L01 Init Complete (Match Driver Naming)");


//    // -------------------------------------------------------------------------
//    // 可选：验证初始化后的关键寄存器（保留验证逻辑，删除手动配置）
//    // -------------------------------------------------------------------------
//    uint8_t en_aa_val = NRF24L01_ReadReg(&hspi1, NRF_REG_EN_AA);
//    NRF_LOGI("EN_AA: 0x%02X (expect 0x01)", en_aa_val);
//    
//    uint8_t en_rxaddr_val = NRF24L01_ReadReg(&hspi1, NRF_REG_EN_RXADDR);
//    NRF_LOGI("EN_RXADDR: 0x%02X (expect 0x01)", en_rxaddr_val);
//    
//    uint8_t rf_setup_val = NRF24L01_ReadReg(&hspi1, NRF_REG_RF_SETUP);
//    NRF_LOGI("RF_SETUP: 0x%02X (expect 0x26)", rf_setup_val); // 250kbps(0x20)+0dBm(0x06)=0x26
//    
//    uint8_t rx_pw_p0_val = NRF24L01_ReadReg(&hspi1, NRF_REG_RX_PW_P0);
//    NRF_LOGI("RX_PW_P0: %d (expect 32)", rx_pw_p0_val);
//	
//	reg_vall = NRF24L01_ReadReg(&hspi1, NRF_REG_SETUP_RETR);
//    NRF_LOGI("SETUP_RETR: 0x%02X", reg_vall);
//    
//    // 读取CONFIG寄存器（默认值0x08/0x09）
//    reg_vall = NRF24L01_ReadReg(&hspi1, NRF_REG_CONFIG);
//    NRF_LOGI("CONFIG(default): 0x%02X", reg_vall);
//	
//    // -------------------------------------------------------------------------
//    // 第五步：切换到TX发送模式（自测核心步骤）
//    // -------------------------------------------------------------------------
//    NRF24L01_SetMode(&hspi1, NRF_MODE_TX);
//    status_val = NRF24L01_GetStatus(&hspi1);
//    NRF_LOGI("STATUS 0x%02X", status_val);
//	
//    // 验证TX模式配置：PRIM_RX bit0=0表示TX模式
//    reg_vall = NRF24L01_ReadReg(&hspi1, NRF_REG_CONFIG);
//    NRF_LOGI("Mode: %s", (reg_vall & 0x01) ? "RX" : "TX");
//	
//    // -------------------------------------------------------------------------
//    // 第六步：清空TX FIFO，避免旧数据干扰测试
//    // -------------------------------------------------------------------------
//    NRF24L01_FlashTxFIFO(&hspi1);
//    fifo_status = NRF24L01_GetFIFOStatus(&hspi1);
//    NRF_LOGI("Flush: 0x%02X", fifo_status);
//    // 清空后TX_EMPTY(bit4)应=1，TX_FULL(bit5)=0

//    // -------------------------------------------------------------------------
//    // 第七步：填充测试数据到TX缓冲区，验证FIFO写入功能
//    // -------------------------------------------------------------------------
//    // 填充32字节测试数据（0x10~0x2F，无越界风险）
//    memset(NRF24L01_txbuffer, 0, sizeof(NRF24L01_txbuffer));
//    for(uint8_t i=0; i<32; i++)
//    {
//        NRF24L01_txbuffer[i] = i + 0x10; // 测试数据：0x10~0x2F
//    }
//    NRF24L01_txbuffer[32] = 32; 
//    
//    // 写入TX FIFO（超时1000ms）
//    NRF24L01_WriteTxFIFO(&hspi1, 1000);
//    HAL_Delay(10); // 等待FIFO写入生效
//    fifo_status = NRF24L01_GetFIFOStatus(&hspi1);
//    
//    // 检测是否触发发射（TX_EMPTY置1表示数据已发射）
//    uint8_t fifo_ever_empty = 0;
//    uint32_t check_start = HAL_GetTick();
//	uint8_t data_written = 1;
//	//(!(fifo_status & NRF_FIFO_STATUS_TX_EMPTY)) ? 1 : 0;
//	if(data_written == 1){
//    while((HAL_GetTick() - check_start) < 500)
//    {
//        fifo_status = NRF24L01_GetFIFOStatus(&hspi1);
//        if((fifo_status & NRF_FIFO_STATUS_TX_EMPTY) == NRF_FIFO_STATUS_TX_EMPTY)
//        {
//            fifo_ever_empty = 1;
//            break;
//        }
//    }
//    // 兜底检测：状态位触发也判定为发射成功
//    if(fifo_ever_empty == 0)
//    {
//        uint8_t status = NRF24L01_GetStatus(&hspi1);
//        if(status & (NRF_STATUS_MAX_RT | NRF_STATUS_TX_DS))
//        {
//            fifo_ever_empty = 1;
//        }
//    }
//}
//    NRF_LOGI("Data Sented?: %s", fifo_ever_empty ? "YES" : "NO");
//    NRF_LOGI("After Write: 0x%02X", fifo_status);

//    // -------------------------------------------------------------------------
//    // 第八步：检测发送状态（核心自测逻辑：无接收端应触发MAX_RT）
//    // -------------------------------------------------------------------------
//    tick_start = HAL_GetTick();
//    uint8_t tx_done = 0, tx_timeout = 0, max_rt = 0;
//    
//    // 等待500ms检测状态（覆盖重传超时的最大时长）
//    while((HAL_GetTick() - tick_start) < 100) 
//    {
//        // 实时读取最新的STATUS寄存器（关键！）
//        status_val = NRF24L01_GetStatus(&hspi1);
//        
//        if(status_val & NRF_STATUS_TX_DS) { tx_done = 1; break; }
//        if(status_val & NRF_STATUS_MAX_RT) { max_rt = 1; break; } // 检测到MAX_RT立即退出
//        
//        HAL_Delay(1);
//    }
//    
//    // 判定超时：500ms未检测到任何状态
//    tx_timeout = ((HAL_GetTick() - tick_start) >= 100) ? 1 : 0;

//    // -------------------------------------------------------------------------
//    // 第九步：输出自测结果（关键判断依据）
//    // -------------------------------------------------------------------------
//    NRF_LOGI("Test Result");
//    NRF_LOGI("SPI: %s", (reg_val == 66) ? "OK" : "FAIL");
//    NRF_LOGI("Chip: %s", (status_val != 0xFF && status_val != 0x00) ? "OK" : "FAIL");
//    NRF_LOGI("FIFO Write: %s", (!(fifo_status & NRF_FIFO_STATUS_TX_EMPTY)) ? "OK" : "FAIL");
//    NRF_LOGI("TX Done: %s", tx_done ? "YES" : "NO");
//    NRF_LOGI("MAX_RT: %s", max_rt ? "YES" : "NO");
//    NRF_LOGI("TX Timeout: %s", tx_timeout ? "FAIL" : "NO");
//    
//    // 最终结论：SPI正常+芯片存活+FIFO写入成功+MAX_RT触发 → 发送功能正常
//    if((reg_val == 66) && (status_val != 0xFF) && (!(fifo_status & NRF_FIFO_STATUS_TX_EMPTY)) && max_rt)
//    {
//        NRF_LOGI("TX Function OK");
//    }
//    else
//    {
//        NRF_LOGI("TX Function FAIL");
//    }
//    
//    // -------------------------------------------------------------------------
//    // 第十步：清理状态，恢复初始状态
//    // -------------------------------------------------------------------------
//    NRF24L01_ClearInterruptFlags(&hspi1, NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
//    NRF24L01_FlashTxFIFO(&hspi1);
//    NRF24L01_SetMode(&hspi1, NRF_MODE_POWER_DOWN); // 进入掉电模式，降低功耗
//    NRF_LOGI("TX Self Test End ");
//}


//void NRF24L01_TX_SelfTest_NoReceiver(void)
//{
//    uint8_t reg_val, status_val, fifo_status;
//    uint32_t start_tick, timeout_ms = 100;
//    uint8_t pwr_up, prim_rx, arc, tx_empty, max_rt = 0, tx_timeout = 0;
//    
//    // 1. 硬件复位
//    NRF24L01_CE_Low();
//    NRF24L01_CSN_High();
//    HAL_Delay(1000);
//    NRF_LOGI("=== TX Self Test (Debug Version) ===");

//    // 2. 验证SPI通信
//    reg_val = NRF24L01_ReadReg(&hspi1, NRF_REG_RF_CH);
//    NRF24L01_WriteReg(&hspi1, NRF_REG_RF_CH, 66);
//    reg_val = NRF24L01_ReadReg(&hspi1, NRF_REG_RF_CH);
//    if(reg_val != 66) { NRF_LOGI("SPI FAIL"); return; }
//    NRF_LOGI("SPI: OK");

//    // 3. 初始化芯片（强制上电+TX模式）
//    NRF_InitTypeDef nrf_init;
//    NRF24L01_GetDefaultConfig(&nrf_init);
//    nrf_init.Mode = NRF_MODE_STANDBY_I;
//    nrf_init.InterruptConfig = NRF_INTERRUPT_ALL_ENABLE;
//    nrf_init.AutoRetransCount = NRF_ARC_5_RETRANS;
//    nrf_init.AutoAckPipes = NRF_PIPE0_BIT;
//    NRF24L01_Init(&hspi1, &nrf_init);

//    // 4. 验证核心配置（关键！）
//    reg_val = NRF24L01_ReadReg(&hspi1, NRF_REG_CONFIG);
//    pwr_up = (reg_val >> 1) & 0x01;
//    prim_rx = reg_val & 0x01;
//    NRF_LOGI("CONFIG PWR_UP: %d (1=OK)", pwr_up);
//    NRF_LOGI("CONFIG PRIM_RX: %d (0=OK)", prim_rx);
//    
//    reg_val = NRF24L01_ReadReg(&hspi1, NRF_REG_EN_AA);
//    NRF_LOGI("EN_AA: 0x%02X (0x01=OK)", reg_val);
//    
//    reg_val = NRF24L01_ReadReg(&hspi1, NRF_REG_SETUP_RETR);
//    arc = reg_val & 0x0F;
//    NRF_LOGI("SETUP_RETR ARC: %d (5=OK)", arc);

//    // 5. 清空并写入FIFO
//    NRF24L01_FlashTxFIFO(&hspi1);
//    memset(NRF24L01_txbuffer, 0, sizeof(NRF24L01_txbuffer));
//    for(uint8_t i=0; i<32; i++) { NRF24L01_txbuffer[i] = i + 0x10; }
//    NRF24L01_txbuffer[32] = 32;
//    NRF24L01_WriteTxFIFO(&hspi1, 1000);
//    HAL_Delay(10);
//    
//    // 验证FIFO是否有数据
//    fifo_status = NRF24L01_GetFIFOStatus(&hspi1);
//    tx_empty = (fifo_status >> 4) & 0x01;
//    NRF_LOGI("TX FIFO Empty: %d (0=OK)", tx_empty);
//    if(tx_empty) { NRF_LOGI("FIFO Write FAIL"); return; }

//    // 6. 启动发送（FIFO有数据后置高CE）
//    NRF_LOGI("Start TX (CE High)");
//    NRF24L01_CE_High();
//    HAL_Delay(1); // 保持CE高电平

//    // 7. 检测MAX_RT
//    start_tick = HAL_GetTick();
//    while(1) {
//        status_val = NRF24L01_GetStatus(&hspi1);
//        // 检测MAX_RT
//        if(status_val & NRF_STATUS_MAX_RT) {
//            max_rt = 1;
//            NRF_LOGI("MAX_RT Triggered!");
//            break;
//        }
//        // 检测超时
//        if((HAL_GetTick() - start_tick) >= timeout_ms) {
//            tx_timeout = 1;
//            NRF_LOGI("TX Timeout!");
//            break;
//        }
//        HAL_Delay(1);
//    }
//    NRF24L01_CE_Low(); // 停止发送

//    // 8. 输出结果
//    NRF_LOGI("=== Final Result ===");
//    NRF_LOGI("MAX_RT: %s", max_rt ? "YES" : "NO");
//    NRF_LOGI("TX Timeout: %s", tx_timeout ? "YES" : "NO");
//    NRF_LOGI("TX Function: %s", (max_rt) ? "OK" : "FAIL");

//    // 9. 清理
//    NRF24L01_ClearInterruptFlags(&hspi1, NRF_STATUS_MAX_RT);
//    NRF24L01_FlashTxFIFO(&hspi1);
//    NRF24L01_SetMode(&hspi1, NRF_MODE_POWER_DOWN);
//    NRF_LOGI("Test End");
//}

void NRF24L01_SimpleSend(uint8_t *data, uint8_t len)
{

    NRF24L01_FlashTxFIFO(&hspi1);
    NRF24L01_ClearInterruptFlags(&hspi1, NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);

    memset(NRF24L01_txbuffer, 0, sizeof(NRF24L01_txbuffer)); // 清空缓冲区
    memcpy(NRF24L01_txbuffer, data, len);                    // 拷贝待发送数据
    NRF24L01_txbuffer[32] = len;                             // 第33字节存储数据长度

    NRF24L01_WriteTxFIFO(&hspi1, 100); 
    uint32_t start_tick = HAL_GetTick();
//    uint8_t send_success = 0;
//    uint8_t max_retrans = 0;

//    while((HAL_GetTick() - start_tick) < 10)
//    {
//		NRF_LOGI("RECYCLING");
//        // 检测发送成功（TX_DS标志置位：接收端已应答）
//        if(NRF24L01_IsTxDataSent(&hspi1))
//        {
//            send_success = 1;
//            NRF24L01_ClearInterruptFlags(&hspi1, NRF_STATUS_TX_DS); // 清空成功标志
//            break;
//        }

//        // 检测最大重传（MAX_RT标志置位：接收端未应答，重传5次失败）
//        if(NRF24L01_IsMaxRetransmit(&hspi1))
//        {
//            max_retrans = 1;
//            NRF24L01_ClearInterruptFlags(&hspi1, NRF_STATUS_MAX_RT); // 清空失败标志
//            NRF24L01_FlashTxFIFO(&hspi1); // 清空TX FIFO，避免残留
//            break;
//        }

//        HAL_Delay(1); // 降低CPU占用
//    }

//    // 7. 输出发送结果
//    if(send_success)
//    {
//        NRF_LOGI("sendsuccess：%d", len);
//    }
//    else if(max_retrans)
//    {
//        NRF_LOGI("resendenough");
//    }
//    else
//    {
//        NRF_LOGI("overtime");
//    }
}

//static void OLED_Log_Clear(void)
//{
//    memset(oled_log_buf, 0, sizeof(oled_log_buf));
//    oled_log_row_idx = 0;
//    OLED_Fill(0x00); // 清屏（也可使用OLED_Fill(0x00)清全屏）
//}

void NRF_LOGI(const char *fmt, ...)
{
    char log_buf[LOG_BUFFER_SIZE] = {0};
    va_list args;          
    va_start(args, fmt);   
    // 格式化日志内容（截断过长内容）
    vsnprintf(log_buf, LOG_BUFFER_SIZE - 1, fmt, args);
    va_end(args);          

    // 1. 处理日志换行和屏幕滚动
    if (oled_log_row_idx >= OLED_LOG_ROWS)
    {
        // 满屏后上移所有行，清空最后一行
        for (uint8_t i = 1; i < OLED_LOG_ROWS; i++)
        {
            strncpy(oled_log_buf[i-1], oled_log_buf[i], OLED_LOG_COLS);
        }
        memset(oled_log_buf[OLED_LOG_ROWS - 1], 0, OLED_LOG_COLS + 1);
        oled_log_row_idx = OLED_LOG_ROWS - 1;
    }

    // 2. 写入当前行缓存（截断过长字符）
    strncpy(oled_log_buf[oled_log_row_idx], log_buf, OLED_LOG_COLS);
    
    // 3. 清屏并重新绘制所有日志行
    OLED_Fill(0x00); // 清空上半屏（日志显示区）
    for (uint8_t i = 0; i < OLED_LOG_ROWS; i++)
    {
        // 逐行绘制：x=0, y=i*8（每行8像素）
        OLED_DrawStr(0, i * 8, (uint8_t *)oled_log_buf[i]);
    }

    // 4. 刷新OLED（仅刷新上半屏，减少刷屏）
    OLED_Refresh();
    
    // 5. 移动到下一行
    oled_log_row_idx++;
	HAL_Delay(10);
}

