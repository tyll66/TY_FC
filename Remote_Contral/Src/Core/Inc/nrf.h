#ifndef __NRF_H
#define __NRF_H
#include "main.h"


// 引脚定义（对应上面的接线）
#define NRF24_CE_PIN    GPIO_PIN_12
#define NRF24_CE_PORT   GPIOA
#define NRF24_CSN_PIN   GPIO_PIN_15
#define NRF24_CSN_PORT  GPIOA


#define NRF_REG_STATUS      0x07    // 状态寄存器
#define NRF_REG_FIFO_STATUS 0x17    // FIFO状态寄存器
#define NRF_CMD_NOP         0xFF    // 空指令（读取状态寄存器）
#define NRF_CMD_TX_MODE     0x20    // 发射模式配置
#define NRF_CMD_RX_MODE     0x30    // 接收模式配置

#define NRF_REG_CONFIG      0x00    // 配置寄存器
#define NRF_REG_EN_AA       0x01    // 自动应答使能
#define NRF_REG_EN_RXADDR   0x02    // 接收地址使能
#define NRF_REG_SETUP_AW    0x03    // 地址宽度设置
#define NRF_REG_SETUP_RETR  0x04    // 自动重传设置
#define NRF_REG_RF_CH       0x05    // RF频道
#define NRF_REG_RF_SETUP    0x06    // RF设置
#define NRF_REG_STATUS      0x07    // 状态寄存器
#define NRF_REG_TX_ADDR     0x10    // 发送地址
#define NRF_REG_RX_ADDR_P0  0x0A    // 接收地址0（与发送地址匹配）
#define NRF_REG_RX_PW_P0    0x11    // 接收数据宽度（通道0）
#define NRF_REG_DYNPD       0x1C    // 动态有效载荷长度

#define NRF_CMD_R_RX_PAYLOAD 0x61   // 读取接收载荷
#define NRF_CMD_W_TX_PAYLOAD 0xA0   // 写入发送载荷
#define NRF_CMD_FLUSH_TX     0xE1   // 清空TX FIFO
#define NRF_CMD_FLUSH_RX     0xE2   // 清空RX FIFO
#define NRF_CMD_NOP          0xFF   // 空指令（读取状态）

#define NRF_ADDR_WIDTH      5       // 地址宽度：3/4/5字节（推荐5字节）
#define NRF_PAYLOAD_LEN     8       // 有效载荷长度：1~32字节（推荐8字节）
#define NRF_RF_CHANNEL      76      // 通信频道：0~125（避开WiFi频道，推荐70~80）
#define NRF_RETR_COUNT      3       // 自动重传次数：0~15
#define NRF_RETR_DELAY      5       // 重传延迟：0~15（对应250~4000us，5=1250us）

//void NRF_LOGI(const char *fmt, ...);
uint8_t NRF24_SPI_RW(uint8_t data);
uint8_t NRF24_ReadReg(uint8_t reg);
void NRF24_WriteReg(uint8_t reg, uint8_t val);
uint8_t NRF24_Test_SPI_Connection(void);
uint8_t NRF24_Test_Tx_Mode(void);
uint8_t NRF24_Test_Rx_Mode(void);
void NRF24_Full_SelfTest(void);
uint8_t NRF24_Send_Data(uint8_t *tx_buf, uint32_t timeout_ms);
uint8_t NRF24_Tx_Init(uint8_t *tx_addr);
void NRF24_WriteRegMulti(uint8_t reg, uint8_t *pBuf, uint8_t len);


#endif

