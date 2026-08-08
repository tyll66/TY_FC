/**
  ******************************************************************************
  * 作者：哔哩哔哩Up主"阿枫的手工铺"
  * 用途：开源免费，用户可以任意查看、使用和修改，并应用到自己的项目之中
  * 许可证：MIT许可证
  * 许可证详情：请查看项目根目录下的LICENSE文件
  * 声明：若二次转载需要注明出处及作者信息，
  *       程序版权归“阿枫的手工铺”所有，任何人或组织不得将其据为己有
  * 
  * 程序名称：			   	  NRF24L01+ 模块驱动程序
  * 程序创建时间：			  2026.02.06
  * 当前程序版本：			  V1.0
  * 当前版本发布时间：		2026.02.06
  * 
  * 如果您发现程序中的漏洞，或者有更好的建议和意见，欢迎发送邮件到：afengdeshougongpu@163.com
  ******************************************************************************

  ******************************************************************************
  * @brief NRF24L01+ 2.4GHz无线收发器驱动函数使用说明
  *
  * 1. 硬件接口
  *    - 使用硬件SPI接口（SPI），但片选引脚（CSN）采用软件控制
  
  * 2.软件接口
  *    - 驱动函数基于STM32 HAL库的SPI阻塞式传输函数HAL_SPI_Transmit/Receive实现，Timeout=100ms

  * 3. 相关引脚宏定义（已添加到NRF24L01.h中，请勿更改）
  *        #define NRF_CSN_Pin GPIO_PIN_4      // PA4 - 片选引脚，推挽输出模式（初始化为高电平）
  *        #define NRF_CE_Pin GPIO_PIN_1       // PB1 - 使能引脚，推挽输出模式（初始化为低电平）
  *        #define NRF_IRQ_Pin GPIO_PIN_0      // PB0 - 中断引脚，外部中断模式（下降沿触发）
  *
  * 4. 主要使用函数
  *    - 初始化函数：
  *        void NRF24L01_Init(SPI_HandleTypeDef *hspi, NRF_InitTypeDef *init);
  *        void NRF24L01_GetDefaultConfig(NRF_InitTypeDef *init);
  *    - NRF24L01结构体初始化参数：
  *        NRF24L01_GetDefaultConfig()函数默认初始化值：
  *        - InterruptConfig: NRF_INTERRUPT_TX_DS_DISABLE (禁用TX数据发送中断)
  *        - CRCConfig: NRF_CRC_ENABLE (启用CRC校验)
  *        - CRCLength: NRF_CRC_LENGTH_2BYTE (2字节CRC)
  *        - Mode: NRF_MODE_STANDBY_I (待机-I模式)
  *        - AutoAckPipes: NRF_PIPE0_BIT (仅管道0自动应答)
  *        - RxPipes: NRF_PIPE0_BIT (仅启用管道0)
  *        - AddrWidth: NRF_ADDR_WIDTH_5BYTES (5字节地址宽度)
  *        - AutoRetransDelay: NRF_ARD_500US (500us重传延迟)
  *        - AutoRetransCount: NRF_ARC_5_RETRANS (最多5次重传)
  *        - RFChannel: 2 (通道2，2.402GHz)
  *        - DataRate: NRF_DATA_RATE_1MBPS (1Mbps数据速率)
  *        - RFPower: NRF_RF_POWER_0DBM (0dBm输出功率)
  *        - DynamicPayloadPipes: NRF_PIPE0_BIT (管道0动态载荷)
  *        - Features: NRF_FEATURE_EN_DPL (启用动态载荷长度)
  *        - PayloadWidthPipe0: 32 (管道0载荷宽度32字节，此参数在启用动态载荷时无效)
  *        - PayloadWidthPipe1-5: 0 (管道1-5未使用)
  *        - TxAddress: {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}//自动重发模式下TxAddress必须与RxAddressPipe0相同，否则无法接收ACK包
  *        - RxAddressPipe0: {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}
  *        - RxAddressPipe1: {0xC2, 0xC2, 0xC2, 0xC2, 0xC2}
  *        - RxAddressPipe2: 0xC3
  *        - RxAddressPipe3: 0xC4
  *        - RxAddressPipe4: 0xC5
  *        - RxAddressPipe5: 0xC6
  *    - 模式控制：
  *        void NRF24L01_SetMode(SPI_HandleTypeDef *hspi, NRF_ModeTypeDef mode);
  *    - 数据传输（需要开启对应的模式）：
  *        void NRF24L01_WriteTxFIFO(SPI_HandleTypeDef *hspi,uint32_t Timeout);
  *        uint8_t NRF24L01_ReadRxFIFO(SPI_HandleTypeDef *hspi);
  * 
  * 5. 数据缓冲区
  *    - 发送缓冲区：NRF24L01_txbuffer[33] - 32字节数据 + 1字节长度信息
  *                 NRF24L01_rxbuffer_pipe0[33];
  *                 NRF24L01_rxbuffer_pipe1[33];  
  *                 NRF24L01_rxbuffer_pipe2[33];
  *                 NRF24L01_rxbuffer_pipe3[33];  
  *                 NRF24L01_rxbuffer_pipe4[33];  
  *                 NRF24L01_rxbuffer_pipe5[33];  
  *
  * 6. 中断回调函数模板（请复制到main.c中）
  *    - 函数功能：将接收到的数据存储到NRF24L01_rxbuffer_pipe0~5中，并将数据长度存储在NRF24L01_rxbuffer_pipe0~5[32]中
  *    - 中断回调函数模板已在此文件末尾提供
  *
  * 7. 使用示例（接收模式）
  *    @code
  *    // 1. 定义初始化结构体
  *    NRF_InitTypeDef nrf_init;
  *    
  *    // 2. 获取默认配置
  *    NRF24L01_GetDefaultConfig(&nrf_init);
  *    
  *    // 3. 修改配置（可选，一般不用修改）
  *    nrf_init.RFChannel = 40;           // 2.440GHz
  *    nrf_init.DataRate = NRF_DATA_RATE_2MBPS;
  *    nrf_init.RFPower = NRF_RF_POWER_0DBM;
  *    
  *    // 4. 初始化NRF24L01
  *    NRF24L01_Init(&hspi1, &nrf_init);
  *    
  *    // 5. 设置为接收模式
  *    NRF24L01_SetMode(&hspi1, NRF_MODE_RX);
  *    
  *    // 7. 接收数据（接收到数据->产生接收中断->最终调用到中断回调函数）
  *    // 数据接收后存储在NRF24L01_rxbuffer_pipe0~5中
  *    // 数据长度存储在NRF24L01_rxbuffer_pipe0~5[32]中
  *    @endcode
  *
  * 8. 使用示例（发送模式）
  *    @code
  *    // 1. 定义初始化结构体
  *    NRF_InitTypeDef nrf_init;
  *    
  *    // 2. 获取默认配置
  *    NRF24L01_GetDefaultConfig(&nrf_init);
  *    
  *    // 3. 修改配置（可选，一般不用修改）
  *    nrf_init.RFChannel = 40;           // 2.440GHz
  *    nrf_init.DataRate = NRF_DATA_RATE_2MBPS;
  *    nrf_init.RFPower = NRF_RF_POWER_0DBM;
  *    
  *    // 4. 初始化NRF24L01
  *    NRF24L01_Init(&hspi1, &nrf_init);  
  *    
  *    // 5. 设置为发送模式
  *    NRF24L01_SetMode(&hspi1, NRF_MODE_TX);
  *    
  *    // 6. 填充发送缓冲区
  *    memcpy(NRF24L01_txbuffer, data_to_send, data_length);
  *    NRF24L01_txbuffer[32] = data_length; // 设置发送数据长度
  *    
  *    // 7. 发送数据
  *    NRF24L01_WriteTxFIFO(&hspi1, 100);
  *    @endcode
  *
  * 9. 注意事项
  *    - 发送前确保设置为TX模式，接收前确保设置为RX模式
  *    - 发送数据前需要填充NRF24L01_txbuffer缓冲区，数据格式:32字节数据 + 1字节长度信息
  *    - 接收数据后从NRF24L01_rxbuffer读取，数据格式：32字节数据 + 1字节长度信息
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NRF24L01_H
#define __NRF24L01_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  NRF24L01+ interrupt configuration
  * @note   Each value is exclusive and corresponds to specific interrupt mask bits
  */
typedef enum
{
    NRF_INTERRUPT_RX_DR_DISABLE  = 0x40,  /*!< Mask RX data ready interrupt (MASK_RX_DR = 1) */
    NRF_INTERRUPT_TX_DS_DISABLE  = 0x20,  /*!< Mask TX data sent interrupt (MASK_TX_DS = 1) */
    NRF_INTERRUPT_MAX_RT_DISABLE = 0x10,  /*!< Mask max retransmit interrupt (MASK_MAX_RT = 1) */
    NRF_INTERRUPT_ALL_ENABLE     = 0x00,  /*!< Enable all interrupts (all mask bits = 0) */
} NRF_InterruptConfigTypeDef;

/**
  * @brief  NRF24L01+ CRC configuration
  * @note   These values are exclusive and correspond to the EN_CRC bit (bit 3) of the CONFIG register
  */
typedef enum
{
    NRF_CRC_DISABLE = 0x00,  /*!< CRC disabled (EN_CRC = 0) */
    NRF_CRC_ENABLE  = 0x08,  /*!< CRC enabled (EN_CRC = 1) */
} NRF_CRCConfigTypeDef;

/**
  * @brief  NRF24L01+ CRC length configuration
  * @note   These values are exclusive and correspond to the CRCO bit (bit 2) of the CONFIG register
  */
typedef enum
{
    NRF_CRC_LENGTH_1BYTE = 0x00,  /*!< 1-byte CRC (CRCO = 0) */
    NRF_CRC_LENGTH_2BYTE = 0x04,  /*!< 2-byte CRC (CRCO = 1) */
} NRF_CRCLengthTypeDef;

/**
  * @brief  NRF24L01+ power control
  * @note   These values are exclusive and correspond to the PWR_UP bit (bit 1) of the CONFIG register
  *         NRF_POWER_UP represents standby-I mode (PWR_UP = 1, CE = 0)
  */
typedef enum
{
    NRF_POWER_DOWN = 0x00,  /*!< Power down mode (PWR_UP = 0) */
    NRF_POWER_UP   = 0x02,  /*!< Standby-I mode (PWR_UP = 1, CE = 0) */
} NRF_PowerControlTypeDef;

/**
  * @brief  NRF24L01+ primary mode selection
  * @note   These values are exclusive and correspond to the PRIM_RX bit (bit 0) of the CONFIG register
  */
typedef enum
{
    NRF_PRIMARY_TX = 0x00,  /*!< Primary transmitter (PRIM_RX = 0) */
    NRF_PRIMARY_RX = 0x01,  /*!< Primary receiver (PRIM_RX = 1) */
} NRF_PrimaryModeTypeDef;

/**
  * @brief  NRF24L01+ address width configuration
  * @note   These values are exclusive and correspond to the AW bits (bits 1:0) of the SETUP_AW register
  */
typedef enum
{
    NRF_ADDR_WIDTH_3BYTES = 0x01,  /*!< 3-byte address width (AW = 01) */
    NRF_ADDR_WIDTH_4BYTES = 0x02,  /*!< 4-byte address width (AW = 10) */
    NRF_ADDR_WIDTH_5BYTES = 0x03,  /*!< 5-byte address width (AW = 11) */
} NRF_AddrWidthTypeDef;

/**
  * @brief  NRF24L01+ auto retransmit delay configuration
  * @note   These values are exclusive and correspond to the ARD bits (bits 7:4) of the SETUP_RETR register
  *         Delay = (ARD value + 1) * 250μs
  */
typedef enum
{
    NRF_ARD_250US   = 0x00,  /*!< 250μs delay (ARD = 0000) */
    NRF_ARD_500US   = 0x10,  /*!< 500μs delay (ARD = 0001) */
    NRF_ARD_750US   = 0x20,  /*!< 750μs delay (ARD = 0010) */
    NRF_ARD_1000US  = 0x30,  /*!< 1000μs delay (ARD = 0011) */
    NRF_ARD_1250US  = 0x40,  /*!< 1250μs delay (ARD = 0100) */
    NRF_ARD_1500US  = 0x50,  /*!< 1500μs delay (ARD = 0101) */
    NRF_ARD_1750US  = 0x60,  /*!< 1750μs delay (ARD = 0110) */
    NRF_ARD_2000US  = 0x70,  /*!< 2000μs delay (ARD = 0111) */
    NRF_ARD_2250US  = 0x80,  /*!< 2250μs delay (ARD = 1000) */
    NRF_ARD_2500US  = 0x90,  /*!< 2500μs delay (ARD = 1001) */
    NRF_ARD_2750US  = 0xA0,  /*!< 2750μs delay (ARD = 1010) */
    NRF_ARD_3000US  = 0xB0,  /*!< 3000μs delay (ARD = 1011) */
    NRF_ARD_3250US  = 0xC0,  /*!< 3250μs delay (ARD = 1100) */
    NRF_ARD_3500US  = 0xD0,  /*!< 3500μs delay (ARD = 1101) */
    NRF_ARD_3750US  = 0xE0,  /*!< 3750μs delay (ARD = 1110) */
    NRF_ARD_4000US  = 0xF0,  /*!< 4000μs delay (ARD = 1111) */
} NRF_AutoRetransDelayTypeDef;

/**
  * @brief  NRF24L01+ auto retransmit count configuration
  * @note   These values are exclusive and correspond to the ARC bits (bits 3:0) of the SETUP_RETR register
  *         0 = auto retransmit disabled, 1-15 = number of retransmits
  */
typedef enum
{
    NRF_ARC_DISABLE   = 0x00,  /*!< Auto retransmit disabled (ARC = 0000) */
    NRF_ARC_1_RETRANS = 0x01,  /*!< Up to 1 retransmit (ARC = 0001) */
    NRF_ARC_2_RETRANS = 0x02,  /*!< Up to 2 retransmits (ARC = 0010) */
    NRF_ARC_3_RETRANS = 0x03,  /*!< Up to 3 retransmits (ARC = 0011) */
    NRF_ARC_4_RETRANS = 0x04,  /*!< Up to 4 retransmits (ARC = 0100) */
    NRF_ARC_5_RETRANS = 0x05,  /*!< Up to 5 retransmits (ARC = 0101) */
    NRF_ARC_6_RETRANS = 0x06,  /*!< Up to 6 retransmits (ARC = 0110) */
    NRF_ARC_7_RETRANS = 0x07,  /*!< Up to 7 retransmits (ARC = 0111) */
    NRF_ARC_8_RETRANS = 0x08,  /*!< Up to 8 retransmits (ARC = 1000) */
    NRF_ARC_9_RETRANS = 0x09,  /*!< Up to 9 retransmits (ARC = 1001) */
    NRF_ARC_10_RETRANS = 0x0A, /*!< Up to 10 retransmits (ARC = 1010) */
    NRF_ARC_11_RETRANS = 0x0B, /*!< Up to 11 retransmits (ARC = 1011) */
    NRF_ARC_12_RETRANS = 0x0C, /*!< Up to 12 retransmits (ARC = 1100) */
    NRF_ARC_13_RETRANS = 0x0D, /*!< Up to 13 retransmits (ARC = 1101) */
    NRF_ARC_14_RETRANS = 0x0E, /*!< Up to 14 retransmits (ARC = 1110) */
    NRF_ARC_15_RETRANS = 0x0F, /*!< Up to 15 retransmits (ARC = 1111) */
} NRF_AutoRetransCountTypeDef;

/**
  * @brief  NRF24L01+ RF data rate configuration
  * @note   These values are exclusive and correspond to the RF_DR_LOW and RF_DR_HIGH bits of the RF_SETUP register
  *         Encoding: [RF_DR_LOW, RF_DR_HIGH]
  */
typedef enum
{
    NRF_DATA_RATE_1MBPS   = 0x00,  /*!< 1Mbps (RF_DR_LOW=0, RF_DR_HIGH=0) */
    NRF_DATA_RATE_2MBPS   = 0x08,  /*!< 2Mbps (RF_DR_LOW=0, RF_DR_HIGH=1) */
    NRF_DATA_RATE_250KBPS = 0x20,  /*!< 250kbps (RF_DR_LOW=1, RF_DR_HIGH=0) */
} NRF_DataRateTypeDef;

/**
  * @brief  NRF24L01+ RF output power configuration
  * @note   These values are exclusive and correspond to the RF_PWR bits (bits 2:1) of the RF_SETUP register
  */
typedef enum
{
    NRF_RF_POWER_N18DBM = 0x00,  /*!< -18dBm output power (RF_PWR = 00) */
    NRF_RF_POWER_N12DBM = 0x02,  /*!< -12dBm output power (RF_PWR = 01) */
    NRF_RF_POWER_N6DBM  = 0x04,  /*!< -6dBm output power (RF_PWR = 10) */
    NRF_RF_POWER_0DBM   = 0x06,  /*!< 0dBm output power (RF_PWR = 11) */
} NRF_RFPowerTypeDef;

/**
  * @brief  NRF24L01+ operation modes
  * @note   These are complete operation modes combining PWR_UP, PRIM_RX and CE control
  */
typedef enum
{
    NRF_MODE_POWER_DOWN,  /*!< Power down mode (PWR_UP = 0) */
    NRF_MODE_STANDBY_I,   /*!< Standby-I mode (PWR_UP = 1, CE = 0) */
    NRF_MODE_RX,          /*!< RX mode (PWR_UP = 1, PRIM_RX = 1, CE = 1) */
    NRF_MODE_TX,          /*!< TX mode (PWR_UP = 1, PRIM_RX = 0, CE = 1) */
} NRF_ModeTypeDef;

/** @defgroup NRF24L01_Data_Pipe_Bit_Masks NRF24L01 Data Pipe Bit Masks
  * @brief NRF24L01+ data pipe bit masks
  * @{
  */
#define NRF_PIPE0_BIT 0x01  /*!< Data pipe 0 bit mask */
#define NRF_PIPE1_BIT 0x02  /*!< Data pipe 1 bit mask */
#define NRF_PIPE2_BIT 0x04  /*!< Data pipe 2 bit mask */
#define NRF_PIPE3_BIT 0x08  /*!< Data pipe 3 bit mask */
#define NRF_PIPE4_BIT 0x10  /*!< Data pipe 4 bit mask */
#define NRF_PIPE5_BIT 0x20  /*!< Data pipe 5 bit mask */
/**
  * @}
  */

/** @defgroup NRF24L01_Feature_Configuration NRF24L01 Feature Configuration
  * @brief NRF24L01+ feature configuration bit masks
  * @{
  */
#define NRF_FEATURE_EN_DYN_ACK  0x01  /*!< Enable W_TX_PAYLOAD_NOACK command */
#define NRF_FEATURE_EN_ACK_PAY  0x02  /*!< Enable payload with ACK */
#define NRF_FEATURE_EN_DPL      0x04  /*!< Enable dynamic payload length */
/**
  * @}
  */

/**
  * @brief  NRF24L01+ initialization structure
  * @note   This structure contains all configuration parameters for NRF24L01+
  *         Each field corresponds to specific register configuration
  */
typedef struct
{
    /* CONFIG register (0x00) settings - each field is exclusive */
    NRF_InterruptConfigTypeDef InterruptConfig;  /*!< Interrupt configuration
                                                      This parameter can be a value of @ref NRF_InterruptConfigTypeDef */
    
    NRF_CRCConfigTypeDef CRCConfig;              /*!< CRC enable configuration
                                                      This parameter can be a value of @ref NRF_CRCConfigTypeDef */
    
    NRF_CRCLengthTypeDef CRCLength;              /*!< CRC length configuration
                                                      This parameter can be a value of @ref NRF_CRCLengthTypeDef */
    
    NRF_ModeTypeDef Mode;          /*!< Operation mode configuration
                                                      This parameter can be a value of @ref NRF_ModeTypeDef */
    
    /* EN_AA register (0x01) settings - bit mask for auto ACK enabled pipes */
    uint8_t AutoAckPipes;                        /*!< Pipes with auto acknowledgment enabled                                           
                                                      Use bitwise OR (|) to combine multiple NRF_PIPEx_BIT values
                                                      Example: NRF_PIPE0_BIT | NRF_PIPE1_BIT enables auto ACK for both pipe 0 and pipe 1 
                                                      This parameter is a bit mask of @ref NRF24L01_Data_Pipe_Bit_Masks */

    /* EN_RXADDR register (0x02) settings - bit mask for enabled receive pipes */
    uint8_t RxPipes;                             /*!< Enabled receive pipes
                                                      Use bitwise OR (|) to combine multiple NRF_PIPEx_BIT values
                                                      Example: NRF_PIPE0_BIT | NRF_PIPE1_BIT enables both pipe 0 and pipe 1 for reception 
                                                      This parameter is a bit mask of @ref NRF24L01_Data_Pipe_Bit_Masks */
    /* SETUP_AW register (0x03) settings */
    NRF_AddrWidthTypeDef AddrWidth;              /*!< Address width configuration
                                                      This parameter can be a value of @ref NRF_AddrWidthTypeDef */
    
    /* SETUP_RETR register (0x04) settings */
    NRF_AutoRetransDelayTypeDef AutoRetransDelay; /*!< Auto retransmit delay configuration
                                                      This parameter can be a value of @ref NRF_AutoRetransDelayTypeDef */
    
    NRF_AutoRetransCountTypeDef AutoRetransCount; /*!< Auto retransmit count configuration
                                                      This parameter can be a value of @ref NRF_AutoRetransCountTypeDef */
    
    /* RF_CH register (0x05) settings */
    uint8_t RFChannel;                           /*!< RF channel frequency (0-125)
                                                      Frequency = 2400 + RFChannel (MHz)
                                                      This parameter must be set to a value between 0 and 125 */
    
    /* RF_SETUP register (0x06) settings */
    NRF_DataRateTypeDef DataRate;                /*!< RF data rate configuration
                                                      This parameter can be a value of @ref NRF_DataRateTypeDef */
    
    NRF_RFPowerTypeDef RFPower;                  /*!< RF output power configuration
                                                      This parameter can be a value of @ref NRF_RFPowerTypeDef */
    
    /* DYNPD register (0x1C) settings - bit mask for dynamic payload pipes */
    uint8_t DynamicPayloadPipes;                 /*!< Pipes with dynamic payload enabled                                          
                                                      Use bitwise OR (|) to combine multiple NRF_PIPEx_BIT values
                                                      Example: NRF_PIPE0_BIT | NRF_PIPE1_BIT enables dynamic payload for both pipe 0 and pipe 1 
                                                      This parameter is a bit mask of @ref NRF24L01_Data_Pipe_Bit_Masks */
    
    /* FEATURE register (0x1D) settings - bit mask for enabled features */
    uint8_t Features;                            /*!< Enabled features
                                                      Use bitwise OR (|) to combine multiple NRF_FEATURE_xxx values
                                                      Example: NRF_FEATURE_EN_DPL | NRF_FEATURE_EN_ACK_PAY enables both dynamic payload and ACK with payload 
                                                      This parameter is a bit mask of @ref NRF24L01_Feature_Configuration */
    
    /* Payload width settings for each pipe (0-32 bytes) 
       These correspond to RX_PW_Px registers (0x11-0x16) */
    uint8_t PayloadWidthPipe0;                   /*!< Payload width for pipe 0 (1-32 bytes)
                                                      This value is written to RX_PW_P0 register (0x11)
                                                      Set to 0 if pipe 0 is not used */
    uint8_t PayloadWidthPipe1;                   /*!< Payload width for pipe 1 (1-32 bytes)
                                                      This value is written to RX_PW_P1 register (0x12)
                                                      Set to 0 if pipe 1 is not used */
    uint8_t PayloadWidthPipe2;                   /*!< Payload width for pipe 2 (1-32 bytes)
                                                      This value is written to RX_PW_P2 register (0x13)
                                                      Set to 0 if pipe 2 is not used */
    uint8_t PayloadWidthPipe3;                   /*!< Payload width for pipe 3 (1-32 bytes)
                                                      This value is written to RX_PW_P3 register (0x14)
                                                      Set to 0 if pipe 3 is not used */
    uint8_t PayloadWidthPipe4;                   /*!< Payload width for pipe 4 (1-32 bytes)
                                                      This value is written to RX_PW_P4 register (0x15)
                                                      Set to 0 if pipe 4 is not used */
    uint8_t PayloadWidthPipe5;                   /*!< Payload width for pipe 5 (1-32 bytes)
                                                      This value is written to RX_PW_P5 register (0x16)
                                                      Set to 0 if pipe 5 is not used */
    
    /* Address settings 
       These correspond to TX_ADDR and RX_ADDR_Px registers (0x0A-0x10) */
    uint8_t TxAddress[5];                        /*!< Transmit address (1-5 bytes, depending on AddrWidth)
                                                      This value is written to TX_ADDR register (0x10) */
    uint8_t RxAddressPipe0[5];                   /*!< Receive address for pipe 0 (1-5 bytes, depending on AddrWidth)
                                                      This value is written to RX_ADDR_P0 register (0x0A) */
    uint8_t RxAddressPipe1[5];                   /*!< Receive address for pipe 1 (1-5 bytes, depending on AddrWidth)
                                                      This value is written to RX_ADDR_P1 register (0x0B) */
    uint8_t RxAddressPipe2;                      /*!< Receive address LSB for pipe 2 (LSB only, MSB = pipe 1 address)
                                                      This value is written to RX_ADDR_P2 register (0x0C) */
    uint8_t RxAddressPipe3;                      /*!< Receive address LSB for pipe 3 (LSB only, MSB = pipe 1 address)
                                                      This value is written to RX_ADDR_P3 register (0x0D) */
    uint8_t RxAddressPipe4;                      /*!< Receive address LSB for pipe 4 (LSB only, MSB = pipe 1 address)
                                                      This value is written to RX_ADDR_P4 register (0x0E) */
    uint8_t RxAddressPipe5;                      /*!< Receive address LSB for pipe 5 (LSB only, MSB = pipe 1 address)
                                                      This value is written to RX_ADDR_P5 register (0x0F) */
} NRF_InitTypeDef;

/* Exported constants --------------------------------------------------------*/

/** @defgroup NRF24L01_Register_Map NRF24L01 Register Map
  * @brief NRF24L01+ register addresses
  * @{
  */
#define NRF_REG_CONFIG          0x00  /*!< Configuration register */
#define NRF_REG_EN_AA           0x01  /*!< Enable 'Auto Acknowledgment' function */
#define NRF_REG_EN_RXADDR       0x02  /*!< Enabled RX addresses */
#define NRF_REG_SETUP_AW        0x03  /*!< Setup of address widths */
#define NRF_REG_SETUP_RETR      0x04  /*!< Setup of automatic retransmission */
#define NRF_REG_RF_CH           0x05  /*!< RF channel */
#define NRF_REG_RF_SETUP        0x06  /*!< RF setup register */
#define NRF_REG_STATUS          0x07  /*!< Status register */
#define NRF_REG_OBSERVE_TX      0x08  /*!< Transmit observe register */
#define NRF_REG_RPD             0x09  /*!< Received power detector */
#define NRF_REG_RX_ADDR_P0      0x0A  /*!< Receive address data pipe 0 */
#define NRF_REG_RX_ADDR_P1      0x0B  /*!< Receive address data pipe 1 */
#define NRF_REG_RX_ADDR_P2      0x0C  /*!< Receive address data pipe 2 */
#define NRF_REG_RX_ADDR_P3      0x0D  /*!< Receive address data pipe 3 */
#define NRF_REG_RX_ADDR_P4      0x0E  /*!< Receive address data pipe 4 */
#define NRF_REG_RX_ADDR_P5      0x0F  /*!< Receive address data pipe 5 */
#define NRF_REG_TX_ADDR         0x10  /*!< Transmit address */
#define NRF_REG_RX_PW_P0        0x11  /*!< Number of bytes in RX payload in data pipe 0 */
#define NRF_REG_RX_PW_P1        0x12  /*!< Number of bytes in RX payload in data pipe 1 */
#define NRF_REG_RX_PW_P2        0x13  /*!< Number of bytes in RX payload in data pipe 2 */
#define NRF_REG_RX_PW_P3        0x14  /*!< Number of bytes in RX payload in data pipe 3 */
#define NRF_REG_RX_PW_P4        0x15  /*!< Number of bytes in RX payload in data pipe 4 */
#define NRF_REG_RX_PW_P5        0x16  /*!< Number of bytes in RX payload in data pipe 5 */
#define NRF_REG_FIFO_STATUS     0x17  /*!< FIFO status register */
#define NRF_REG_DYNPD           0x1C  /*!< Enable dynamic payload length */
#define NRF_REG_FEATURE         0x1D  /*!< Feature register */
/**
  * @}
  */

/** @defgroup NRF24L01_Commands NRF24L01 Commands
  * @brief NRF24L01+ SPI commands
  * @{
  */
#define NRF_CMD_R_REGISTER      0x00  /*!< Read command and status registers */
#define NRF_CMD_W_REGISTER      0x20  /*!< Write command and status registers */
#define NRF_CMD_R_RX_PAYLOAD    0x61  /*!< Read RX-payload: 1 - 32 bytes */
#define NRF_CMD_W_TX_PAYLOAD    0xA0  /*!< Write TX-payload: 1 - 32 bytes */
#define NRF_CMD_FLUSH_TX        0xE1  /*!< Flush TX FIFO */
#define NRF_CMD_FLUSH_RX        0xE2  /*!< Flush RX FIFO */
#define NRF_CMD_REUSE_TX_PL     0xE3  /*!< Reuse last transmitted payload */
#define NRF_CMD_R_RX_PL_WID     0x60  /*!< Read RX payload width for the top R_RX_PAYLOAD in the RX FIFO */
#define NRF_CMD_W_ACK_PAYLOAD   0xA8  /*!< Write payload to be transmitted together with ACK packet */
#define NRF_CMD_W_TX_PAYLOAD_NOACK 0xB0  /*!< Write TX payload with no ACK */
#define NRF_CMD_NOP             0xFF  /*!< No operation */
/**
  * @}
  */

/** @defgroup NRF24L01_Status_Register_Bits NRF24L01 Status Register Bits
  * @brief NRF24L01+ status register bit definitions
  * @{
  */
#define NRF_STATUS_RX_DR        0x40  /*!< Data ready RX FIFO interrupt */
#define NRF_STATUS_TX_DS        0x20  /*!< Data sent TX FIFO interrupt */
#define NRF_STATUS_MAX_RT       0x10  /*!< Maximum number of TX retransmits interrupt */
#define NRF_STATUS_RX_P_NO      0x0E  /*!< Data pipe number for the payload available for reading from RX_FIFO */
#define NRF_STATUS_TX_FULL      0x01  /*!< TX FIFO full flag */
/**
  * @}
  */

/** @defgroup NRF24L01_FIFO_Status_Register_Bits NRF24L01 FIFO Status Register Bits
  * @brief NRF24L01+ FIFO status register bit definitions (Register 0x17)
  * @{
  */
#define NRF_FIFO_STATUS_TX_REUSE  0x40  /*!< TX reuse flag (bit 6) */
#define NRF_FIFO_STATUS_TX_FULL   0x20  /*!< TX FIFO full flag (bit 5) */
#define NRF_FIFO_STATUS_TX_EMPTY  0x10  /*!< TX FIFO empty flag (bit 4) */
#define NRF_FIFO_STATUS_RX_FULL   0x02  /*!< RX FIFO full flag (bit 1) */
#define NRF_FIFO_STATUS_RX_EMPTY  0x01  /*!< RX FIFO empty flag (bit 0) */
/**
  * @}
  */


#define NRF_CSN_Pin GPIO_PIN_1      // PA4 - 片选引脚，推挽输出模式（初始化为高电平）
#define NRF_CE_Pin GPIO_PIN_0       // PB1 - 使能引脚，推挽输出模式（初始化为低电平）
#define NRF_IRQ_Pin GPIO_PIN_1      // PB0 - 中断引脚，外部中断模式（下降沿触发）

/* Global data packet buffers for NRF24L01 ,32 bytes data + 1 byte data_length*/
extern uint8_t NRF24L01_txbuffer[33];       
extern uint8_t NRF24L01_rxbuffer_pipe0[33];
extern uint8_t NRF24L01_rxbuffer_pipe1[33];  
extern uint8_t NRF24L01_rxbuffer_pipe2[33];
extern uint8_t NRF24L01_rxbuffer_pipe3[33];  
extern uint8_t NRF24L01_rxbuffer_pipe4[33];  
extern uint8_t NRF24L01_rxbuffer_pipe5[33];  

/* Init Functions */
void NRF24L01_Init(SPI_HandleTypeDef *hspi, NRF_InitTypeDef *init);
void NRF24L01_GetDefaultConfig(NRF_InitTypeDef *init);

/* Mode Control Function */
void NRF24L01_SetMode(SPI_HandleTypeDef *hspi, NRF_ModeTypeDef mode);

/* Data Transmit and Receive Functions */
void NRF24L01_WriteTxFIFO(SPI_HandleTypeDef *hspi,uint32_t Timeout);
void NRF24L01_ReadRxFIFO(SPI_HandleTypeDef *hspi);

/* FIFO Control Functions */
void NRF24L01_FlushRxFIFO(SPI_HandleTypeDef *hspi);
void NRF24L01_FlashTxFIFO(SPI_HandleTypeDef *hspi);

/* Status Register Check Functions */
uint8_t NRF24L01_GetStatus(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_GetFIFOStatus(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsRxDataReady(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsTxDataSent(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsMaxRetransmit(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_GetRxPipeNumber(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsTxFIFOFull(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_IsRxFIFOEmpty(SPI_HandleTypeDef *hspi);
uint8_t NRF24L01_GetPipeNum(SPI_HandleTypeDef *hspi);
void NRF24L01_ClearInterruptFlags(SPI_HandleTypeDef *hspi, uint8_t flags);
void NRF24L01_PrintPipe0ReceivedData(void);
void NRF24L01_data_care(void);
void NRF24L01_RX_SelfTest_NoSender(void);
void NRF24L01_RX_SelfTest_Enhanced(void);



typedef struct
{
    float last_speed;
    float target_speed;   // 目标速度
    float target_roll;    // 目标横滚角 / 翻滚角，单位：°
    float target_pitch;   // 目标俯仰角，单位：°
} Target_State_t;

extern Target_State_t Target_State;



#ifdef __cplusplus
}
#endif

#endif /* __NRF24L01_H */
