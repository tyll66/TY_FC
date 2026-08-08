#include "adc.h"
#include "ssd1306.h"
#include <stdarg.h>
#include <string.h> 

#define LOG_BUFFER_SIZE  512
#define OLED_LOG_ROWS 4
#define OLED_LOG_COLS 21
static char oled_log_buf[OLED_LOG_ROWS][OLED_LOG_COLS + 1] = {0};
static uint8_t oled_log_row_idx = 0;

typedef struct {
  uint16_t adc_value;    // 原始ADC值（12位：0~4095）
  float voltage;         // 对应电压值（0~3.3V，默认参考电压3.3V）
  float offset_percent;  // 摇杆偏移百分比（-100%~+100%，以中间值2048为中心）
} PS2_Joystick_Axis;

PS2_Joystick_Axis ps2_x, ps2_y, ps2_z, ps2_key;


void PS2_Joystick_Read(void)
{
  uint16_t adc_buf[2] = {0};  // 仅存储X/Y轴（通道0/1）的ADC值
//  float ref_voltage = 3.8f;   // ADC参考电压
  uint16_t mid_value = 2048;  // 12位ADC中间值（摇杆中立位置）

  /* 1. 启动ADC转换（软件触发） */
  HAL_ADC_Start(&hadc1);

  /* 2. 等待转换完成（超时时间100ms） */
  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
  {
    /* 3. 读取X/Y轴ADC值（仅通道0/1） */
    adc_buf[0] = HAL_ADC_GetValue(&hadc1);  // 通道0（X轴）
    adc_buf[1] = HAL_ADC_GetValue(&hadc1);  // 通道1（Y轴）

    /* 4. 转换X轴数据（仅保留ADC值和偏移百分比，简化显示） */
    ps2_x.adc_value = adc_buf[0];
    ps2_x.offset_percent = ((float)(ps2_x.adc_value - mid_value) / mid_value) * 100.0f;

    /* 5. 转换Y轴数据 */
    ps2_y.adc_value = adc_buf[1];
    ps2_y.offset_percent = ((float)(ps2_y.adc_value - mid_value) / mid_value) * 100.0f;
  }

  /* 6. 停止ADC转换 */
  HAL_ADC_Stop(&hadc1);
}

/**
 * @brief  打印PS2摇杆数据（需配置串口，如USART1）
 * @param  无
 * @retval 无
 */
void ADC_LOGI(const char *fmt, ...)
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
	HAL_Delay(100);
}


void PS2_Joystick_Show_OLED(void)
{
    // 清空日志行索引，每次显示从第一行开始（避免滚动混乱）
    oled_log_row_idx = 0;
    
    // 第一行：X轴ADC值 + 偏移百分比（格式简洁，适配21列）
    ADC_LOGI("X: %4d (%.1f%%)", ps2_x.adc_value, ps2_x.offset_percent);
    // 第二行：Y轴ADC值 + 偏移百分比
    ADC_LOGI("Y: %4d (%.1f%%)", ps2_y.adc_value, ps2_y.offset_percent);
    // 第三/四行留空（也可添加提示文字）
    ADC_LOGI("                ");
    ADC_LOGI("PS2 Joystick    ");
}

