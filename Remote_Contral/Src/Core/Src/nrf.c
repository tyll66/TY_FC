#include "nrf.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ssd1306.h"

#define LOG_BUFFER_SIZE  512
#define OLED_LOG_ROWS 4
#define OLED_LOG_COLS 21


//static char oled_log_buf[OLED_LOG_ROWS][OLED_LOG_COLS + 1] = {0};
//static uint8_t oled_log_row_idx = 0;

void NRF_LOGI(const char *fmt, ...);
uint8_t NRF24_SPI_RW(uint8_t data);
uint8_t NRF24_ReadReg(uint8_t reg);
void NRF24_WriteReg(uint8_t reg, uint8_t val);
uint8_t NRF24_Test_SPI_Connection(void);
uint8_t NRF24_Test_Tx_Mode(void);
uint8_t NRF24_Test_Rx_Mode(void);
void NRF24_Full_SelfTest(void);


uint8_t NRF24_SPI_RW(uint8_t data)
{
  uint8_t rx_data;
  HAL_SPI_TransmitReceive(&hspi1, &data, &rx_data, 1, 100);
  return rx_data;
}

// 读取NRF24寄存器值
uint8_t NRF24_ReadReg(uint8_t reg)
{
  uint8_t reg_val;
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET); // 拉低CSN，开始通信
  NRF24_SPI_RW(reg & 0x1F); // 发送寄存器地址（读操作，最高位0）
  reg_val = NRF24_SPI_RW(NRF_CMD_NOP); // 读取寄存器值
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET); // 拉高CSN，结束通信
  return reg_val;
}


void NRF24_WriteReg(uint8_t reg, uint8_t val)
{
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
  NRF24_SPI_RW(reg | 0x20); // 发送寄存器地址（写操作，最高位1）
  NRF24_SPI_RW(val); // 写入值
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);
}

uint8_t NRF24_Test_SPI_Connection(void)
{
  uint8_t status_reg = NRF24_ReadReg(NRF_REG_STATUS);
  // 正常情况下，STATUS寄存器初始值不为0xFF也不为0x00（通常是0x0E或0x0F）
  // 如果返回0xFF/0x00，说明SPI通信失败（接线错、模块没供电、模块损坏）
  if(status_reg == 0xFF || status_reg == 0x00)
  {
    return 1; // 异常
  }
  NRF_LOGI("STATUS：0x%02X（0x00/0xFF）\r\n", status_reg); // 替换printf
  return 0; // 正常
}

/**
 * @brief 发射端自测（单端）
 * @return 0=发射状态正常，1=异常
 */
uint8_t NRF24_Test_Tx_Mode(void)
{
  // 1. 配置为发射模式
  NRF24_WriteReg(NRF_REG_CONFIG, 0x0E); // PWR_UP=1，PRIM_RX=0（发射模式），CRC使能
  HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_SET); // 拉高CE，使能发射

  // 2. 延迟10ms，让模块进入发射状态
  HAL_Delay(10);

  // 3. 读取发射状态
  uint8_t status = NRF24_ReadReg(NRF_REG_STATUS);
  uint8_t fifo_status = NRF24_ReadReg(NRF_REG_FIFO_STATUS);

  NRF_LOGI("STATUS：0x%02X | FIFO_STATUS：0x%02X\r\n", status, fifo_status); 

  // 正常判断：STATUS的TX_DS（bit5）/MAX_RT（bit4）应为0，FIFO_TX_EMPTY（bit4）应为1（无数据时）
  if((status & 0x30) == 0 && (fifo_status & 0x10) == 0x10)
  {
    return 0; // 发射状态正常
  }
  return 1; // 异常
}


uint8_t NRF24_Test_Rx_Mode(void)
{
  // 1. 配置为接收模式
  NRF24_WriteReg(NRF_REG_CONFIG, 0x0F); // PWR_UP=1，PRIM_RX=1（接收模式），CRC使能
  HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_SET); // 拉高CE，开始监听

  // 2. 延迟10ms，让模块进入接收状态
  HAL_Delay(10);

  // 3. 读取接收状态
  uint8_t status = NRF24_ReadReg(NRF_REG_STATUS);
  uint8_t fifo_status = NRF24_ReadReg(NRF_REG_FIFO_STATUS);

  NRF_LOGI("STATUS：0x%02X | FIFO_STATUS：0x%02X\r\n", status, fifo_status);

  // 正常判断：STATUS的RX_DR（bit6）应为0，FIFO_RX_EMPTY（bit0）应为1（无数据时）
  if((status & 0x40) == 0 && (fifo_status & 0x01) == 0x01)
  {
    return 0; // 接收状态正常
  }
  return 1; // 异常
}

// ************************* 主测试入口 *************************
void NRF24_Full_SelfTest(void)
{
	
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);
  // CE拉低，进入待机态
  HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_RESET);
  // 延时10ms，确保电平稳定（模块响应电平变化需要微秒级，延时更稳妥）
  HAL_Delay(10);
	
  NRF_LOGI("NRF24L01+ TEST");

  // 第一步：测试SPI通信
  if(NRF24_Test_SPI_Connection() != 0)
  {
    NRF_LOGI("SPI FAILD");
    return;
  }
  NRF_LOGI("SPI RIGHT");

  // 第二步：测试发射模式
  if(NRF24_Test_Tx_Mode() != 0)
  {
    NRF_LOGI("TX FAILD");
  }
  else
  {
    NRF_LOGI("TX RIGHT");
  }

  // 先复位CE引脚
  HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_RESET);
  HAL_Delay(50);

  // 第三步：测试接收模式
  if(NRF24_Test_Rx_Mode() != 0)
  {
    NRF_LOGI("RX FAILD");
  }
  else
  {
   NRF_LOGI("RX RIGHT");
  }

  NRF_LOGI("NRF24L01+ TEST FINISH");
  
}

/**
 * @brief 向NRF24寄存器写入多字节数据（用于写地址/载荷）
 * @param reg 寄存器地址
 * @param pBuf 数据缓冲区
 * @param len 数据长度
 */
void NRF24_WriteRegMulti(uint8_t reg, uint8_t *pBuf, uint8_t len)
{
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
    NRF24_SPI_RW(reg | 0x20);  // 写操作标志（最高位1）
    for(uint8_t i=0; i<len; i++)
    {
        NRF24_SPI_RW(pBuf[i]); // 逐字节写入
    }
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);
}

uint8_t NRF24_Tx_Init(uint8_t *tx_addr)
{
    // 1. 待机模式（CE拉低）
    HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);

    // 2. 清空TX/RX FIFO（避免残留数据）
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
    NRF24_SPI_RW(NRF_CMD_FLUSH_TX);
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);

    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
    NRF24_SPI_RW(NRF_CMD_FLUSH_RX);
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);

    // 3. 配置自动应答（开启通道0）
    NRF24_WriteReg(NRF_REG_EN_AA, 0x01);        // 开启通道0自动应答
    NRF24_WriteReg(NRF_REG_EN_RXADDR, 0x01);     // 使能通道0（接收应答用）

    // 4. 配置地址宽度（5字节）
    NRF24_WriteReg(NRF_REG_SETUP_AW, NRF_ADDR_WIDTH-2); // 0x03=5字节，0x02=4字节，0x01=3字节

    // 5. 配置自动重传（3次重传，1250us延迟）
    NRF24_WriteReg(NRF_REG_SETUP_RETR, (NRF_RETR_DELAY<<4) | NRF_RETR_COUNT);

    // 6. 配置RF频道和速率（2Mbps，0dBm功率）
    NRF24_WriteReg(NRF_REG_RF_CH, NRF_RF_CHANNEL);       // 设置频道
    NRF24_WriteReg(NRF_REG_RF_SETUP, 0x06);              // 0dBm功率，2Mbps速率

    // 7. 设置发送地址和接收地址0（接收应答需要）
    NRF24_WriteRegMulti(NRF_REG_TX_ADDR, tx_addr, NRF_ADDR_WIDTH);
    NRF24_WriteRegMulti(NRF_REG_RX_ADDR_P0, tx_addr, NRF_ADDR_WIDTH);

    // 8. 设置有效载荷长度（通道0）
    NRF24_WriteReg(NRF_REG_RX_PW_P0, NRF_PAYLOAD_LEN);

    // 9. 配置为主发射模式（PWR_UP=1，PRIM_RX=0，CRC=2字节）
    NRF24_WriteReg(NRF_REG_CONFIG, 0x0E); // 0000 1110：CRC使能(2字节)，发射模式，上电

    // 10. 延迟等待模块稳定
    HAL_Delay(10);

    // 检查STATUS寄存器，验证初始化
    uint8_t status = NRF24_ReadReg(NRF_REG_STATUS);
    if(status == 0xFF || status == 0x00)
    {
        NRF_LOGI("TX INIT FAIL");
        return 1;
    }
    NRF_LOGI("TX INIT OK");
    return 0;
}


uint8_t NRF24_Send_Data(uint8_t *tx_buf, uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    uint8_t status = 0;

	uint8_t rf_ch = NRF24_ReadReg(NRF_REG_RF_CH);
	NRF_LOGI("RF_CH: %d", rf_ch); // 正常应输出76，若不是则SPI写失败
	
    // 1. 检查TX FIFO是否满
    uint8_t fifo_status = NRF24_ReadReg(NRF_REG_FIFO_STATUS);
	
	uint8_t config = NRF24_ReadReg(NRF_REG_CONFIG);
	NRF_LOGI("CONFIG: 0x%02X", config);
	
    if(fifo_status & 0x20) // TX_FIFO_FULL位（bit5）为1表示满
    {
        NRF_LOGI("TX FIFO FULL");
        // 清空FIFO后重试
        HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
        NRF24_SPI_RW(NRF_CMD_FLUSH_TX);
        HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);
        return 2;
    }

    // 2. 写入数据到TX FIFO
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
    NRF24_SPI_RW(NRF_CMD_W_TX_PAYLOAD); // 写入发送载荷指令
    for(uint8_t i=0; i<NRF_PAYLOAD_LEN; i++)
    {
        NRF24_SPI_RW(tx_buf[i]); // 逐字节写入数据
    }
	
	fifo_status = NRF24_ReadReg(NRF_REG_FIFO_STATUS);
	NRF_LOGI("FIFO write: 0x%02X", fifo_status);
	
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);

    // 3. 拉高CE触发发送（至少10us，这里保持到发送完成）
    HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_SET);
    HAL_Delay(1); // 确保CE拉高时间足够

    // 4. 等待发送完成或超时
    while(1)
    {
        status = NRF24_ReadReg(NRF_REG_STATUS);
		
        NRF_LOGI("status: 0x%02X", status); 
        // 发送成功（TX_DS位，bit5置1）
        if(status & 0x20)
        {
            HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_RESET);
            // 清除TX_DS标志（写1清除）
            NRF24_WriteReg(NRF_REG_STATUS, 0x20);
            NRF_LOGI("SEND OK: %s", tx_buf);
            return 0;
        }

        // 重传超时（MAX_RT位，bit4置1）
        if(status & 0x10)
        {
            HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_RESET);
            // 清除MAX_RT标志，清空TX FIFO
            NRF24_WriteReg(NRF_REG_STATUS, 0x10);
            HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
            NRF24_SPI_RW(NRF_CMD_FLUSH_TX);
            HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);
            NRF_LOGI("SEND TIMEOUT");
            return 1;
        }

        // 超时退出
        if(HAL_GetTick() - start_tick > timeout_ms)
        {
            HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_RESET);
            NRF_LOGI("SEND OVERTIME");
            return 1;
        }
    }
}


//static void OLED_Log_Clear(void)
//{
//    memset(oled_log_buf, 0, sizeof(oled_log_buf));
//    oled_log_row_idx = 0;
//    OLED_Fill(0x00); // 清屏（也可使用OLED_Fill(0x00)清全屏）
//}

//void NRF_LOGI(const char *fmt, ...)
//{
//    char log_buf[LOG_BUFFER_SIZE] = {0};
//    va_list args;          
//    va_start(args, fmt);   
//    // 格式化日志内容（截断过长内容）
//    vsnprintf(log_buf, LOG_BUFFER_SIZE - 1, fmt, args);
//    va_end(args);          

//    // 1. 处理日志换行和屏幕滚动
//    if (oled_log_row_idx >= OLED_LOG_ROWS)
//    {
//        // 满屏后上移所有行，清空最后一行
//        for (uint8_t i = 1; i < OLED_LOG_ROWS; i++)
//        {
//            strncpy(oled_log_buf[i-1], oled_log_buf[i], OLED_LOG_COLS);
//        }
//        memset(oled_log_buf[OLED_LOG_ROWS - 1], 0, OLED_LOG_COLS + 1);
//        oled_log_row_idx = OLED_LOG_ROWS - 1;
//    }

//    // 2. 写入当前行缓存（截断过长字符）
//    strncpy(oled_log_buf[oled_log_row_idx], log_buf, OLED_LOG_COLS);
//    
//    // 3. 清屏并重新绘制所有日志行
//    OLED_Fill(0x00); // 清空上半屏（日志显示区）
//    for (uint8_t i = 0; i < OLED_LOG_ROWS; i++)
//    {
//        // 逐行绘制：x=0, y=i*8（每行8像素）
//        OLED_DrawStr(0, i * 8, (uint8_t *)oled_log_buf[i]);
//    }

//    // 4. 刷新OLED（仅刷新上半屏，减少刷屏）
//    OLED_Refresh();
//    
//    // 5. 移动到下一行
//    oled_log_row_idx++;
//	HAL_Delay(1000);
//}

