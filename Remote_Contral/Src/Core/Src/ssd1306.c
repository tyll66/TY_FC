/*
 * @Author: Snitro
 * @Date: 2021-02-23 17:42:08
 * @LastEditors: Snitro
 * @LastEditTime: 2021-02-24 15:30:45
 * @Description: SSD1306驱动
 */

#include "ssd1306.h"

uint8_t OLED_buffer[4][128];    // 缓冲区128x32为4
uint8_t OLED_buffer_flag[128];  // 刷新标志
#ifdef OLED_USING_HARDWARE_I2C
I2C_HandleTypeDef OLED_hi2c;
#endif

/**
 * @description: 发送语句至OLED
 * @param {uint8_t} arg DataReg 发送数据
 *                      CmdReg  发送指令
 * @param {uint8_t} data    1字节数据
 * @return {HAL_StatusTypeDef} HAL 状态
 */
HAL_StatusTypeDef OLED_Write_Byte(uint8_t arg, uint8_t data) {
#ifdef OLED_USING_HARDWARE_I2C
    return HAL_I2C_Mem_Write(&OLED_hi2c, OLED_Addr, arg, I2C_MEMADD_SIZE_8BIT,
                             &data, 1, 0xff);
#endif
}

/**
 * @description:  以硬件I2C方式初始化OLED
 * @param {I2C_HandleTypeDef} hi2c 硬件i2c句柄
 * @return {*}
 */
#ifdef OLED_USING_HARDWARE_I2C
void OLED_Init(I2C_HandleTypeDef hi2c) {
    OLED_hi2c = hi2c;
#endif
    OLED_Write_Byte(CmdReg, OLED_OFF);  //关闭显示器
    OLED_Write_Byte(CmdReg, 0x20);  //设置内存寻址模式Set Memory Addressing Mode
    // 00,水平寻址模式 01,垂直寻址模式 02,页面寻址模式(复位)
    OLED_Write_Byte(CmdReg, 0x01);
    OLED_Write_Byte(CmdReg, 0x81);  //设置对比度
    OLED_Write_Byte(CmdReg, 0xff);  //对比度,数值越大对比度越高
    OLED_Write_Byte(CmdReg, 0xc8);  //扫描方向 不上下翻转Com scan direction
    OLED_Write_Byte(CmdReg, 0xa1);  //设置段重新映射 不左右翻转set segment remap
    OLED_Write_Byte(CmdReg, 0xa8);  //设置多路复用比(1-64)
    OLED_Write_Byte(CmdReg, 0x1f);  	//设定值1/32  1/32 duty
    OLED_Write_Byte(CmdReg, 0xd3);  //设置显示偏移 set display offset
    OLED_Write_Byte(CmdReg, 0x00);  //
    OLED_Write_Byte(CmdReg, 0xd5);  //设置osc分区 set osc division
    OLED_Write_Byte(CmdReg, 0x80);  //
    OLED_Write_Byte(CmdReg, 0xd8);  //关闭区域颜色模式 set area color mode off
    OLED_Write_Byte(CmdReg, 0x05);  //
    OLED_Write_Byte(CmdReg, 0xd9);  //设置预充电期 Set Pre-Charge Period
    OLED_Write_Byte(CmdReg, 0xf1);  //
    OLED_Write_Byte(CmdReg, 0xda);  //设置com引脚配置 set com pin configuartion
    OLED_Write_Byte(CmdReg, 0x02);  //
    OLED_Write_Byte(CmdReg, 0xdb);  //设置vcomh set Vcomh
    OLED_Write_Byte(CmdReg, 0x30);  //
    OLED_Write_Byte(CmdReg, 0x8d);  //设置电源泵启用 set charge pump enable
    OLED_Write_Byte(CmdReg, 0x14);  //
    OLED_Write_Byte(CmdReg, 0xa4);  //设置全局显示  bit0，1白，0黑
#ifdef OLED_INVERSE_COLOR
    OLED_Write_Byte(CmdReg, 0xa7);  //反相显示
#endif
#ifndef OLED_INVERSE_COLOR
    OLED_Write_Byte(CmdReg, 0xa6);  //正常显示
#endif
    OLED_Fill(0x00);                      //清屏
    OLED_Write_Byte(CmdReg, OLED_ON);  //打开oled面板 turn on oled panel
}

/**
 * @description: 清空缓存，并更新屏幕
 * @param {*}
 * @return {*}
 */
void OLED_Fill(uint8_t data){
    uint16_t i;
    OLED_SetPos(0, 0);

    for (i = 0; i < 4 * OLED_Width; i++) OLED_Write_Byte(DataReg, data);

    //更新缓存
    memset(OLED_buffer, data, sizeof(OLED_buffer));
    memset(OLED_buffer_flag, 0, sizeof(OLED_buffer_flag));
}

/**
 * @description: 设置垂直寻址模式下坐标
 * @param {uint8_t} x 起始列地址
 * @param {uint8_t} up 起始页地址
 * @param {uint8_t} down   结束页地址
 * @return {*}
 */
static void OLED_SetVerticalPos(uint8_t x, uint8_t up, uint8_t down) {
	//限制页数为3
	up = (up > 3) ? 3 : up;
    down = (down > 3) ? 3 : down;
    OLED_Write_Byte(CmdReg, 0x22);  //设置页地址
    OLED_Write_Byte(CmdReg, up);    //起始
    OLED_Write_Byte(CmdReg, down);  //结束
    OLED_Write_Byte(CmdReg, 0x21);  //设置列地址
    OLED_Write_Byte(CmdReg, x);     //起始
    OLED_Write_Byte(CmdReg, 0x7f);  //结束
}

/**
 * @description: 设置垂直寻址模式下坐标，保持结束页地址为结尾
 * @param {uint8_t} x 起始列地址
 * @param {uint8_t} page 起始页地址
 * @return {*}
 */
void OLED_SetPos(uint8_t x, uint8_t page) {
    OLED_SetVerticalPos(x, page, 0x07);
}

/**
 * @description: 区域刷新
 * @param {uint8_t} l   起始列地址
 * @param {uint8_t} r   结束列地址
 * @return {*}
 */
static void OLED_Area_Refresh(uint8_t l, uint8_t r) {
	//限制页数为3
    uint8_t count = 0, i, up = 0, down = 3, x, page;

    for (i = l; i <= r; i++) count |= OLED_buffer_flag[i];

    while (!(count & (1 << up))) up++;

    while (!(count & (1 << down))) down--;

    OLED_SetVerticalPos(l, up, down);

    for (x = l; x <= r; x++)
        for (page = up; page <= down; page++)
            OLED_Write_Byte(DataReg, OLED_buffer[page][x]);

    memset(OLED_buffer_flag + l, 0, r - l + 1);
}

/**
 * @description: 全屏刷新
 * @param {*}
 * @return {*}
 */
void OLED_Refresh() {
    uint8_t l = 0, r;

    while (l <= OLED_Width) {
        while (l <= OLED_Width && !OLED_buffer_flag[l]) l++;

        r = l;
        while (r <= OLED_Width && OLED_buffer_flag[r]) r++;
        r--;

        if (l <= OLED_Width) OLED_Area_Refresh(l, r);

        l = r + 1;
    }
}

//只刷新上半部分的
void OLED_Refresh_Upper_Half(void)
{
    uint8_t l = 0, r;
    // 只处理上半部分（页0、页1），忽略页2、页3的刷新标志
    while (l <= OLED_Width)
    {
        // 找到需要刷新的起始列（仅判断页0、页1的标志）
        while (l <= OLED_Width && !(OLED_buffer_flag[l] & 0x03)) l++; // 0x03 = bit0+bit1，对应上半部分两页
        
        r = l;
        // 找到需要刷新的结束列（仅判断页0、页1的标志）
        while (r <= OLED_Width && (OLED_buffer_flag[r] & 0x03)) r++;
        r--;

        if (l <= OLED_Width)
        {
            // 手动指定刷新范围：列l~r，页0~1（上半部分）
            uint8_t x, page;
            OLED_SetVerticalPos(l, 0, 1); // 限定上下页为0和1（上半部分）
            
            // 只写入上半部分缓存数据
            for (x = l; x <= r; x++)
                for (page = 0; page <= 1; page++)
                    OLED_Write_Byte(DataReg, OLED_buffer[page][x]);
            
            // 仅清除上半部分的刷新标志（bit0、bit1），保留下半部分标志
            for (x = l; x <= r; x++)
                OLED_buffer_flag[x] &= 0xFC; // 0xFC = 11111100，清除bit0、bit1，保留bit2、bit3
        }

        l = r + 1;
    }
}


//只填充上半部分
void OLED_Fill_Upper_Half(uint8_t data)
{
    uint16_t i;
    // 1. 设置坐标范围：仅上半部分（列0~127，页0~1）
    OLED_SetVerticalPos(0, 0, 1); // 起始列0，起始页0，结束页1（上半部分）

    // 2. 向OLED屏幕上半部分写入填充数据（2页*128列=256个字节）
    for (i = 0; i < 2 * OLED_Width; i++) 
    {
        OLED_Write_Byte(DataReg, data);
    }

    // 3. 只清空/填充缓存的上半部分（第0、1页），保留第2、3页缓存数据
    memset(OLED_buffer[0], data, OLED_Width); // 填充第0页缓存
    memset(OLED_buffer[1], data, OLED_Width); // 填充第1页缓存

    // 4. 仅标记上半部分需要刷新（或清除对应标志），保留下半部分标志
    for (i = 0; i < OLED_Width; i++)
    {
        // 清除上半部分（bit0、bit1）的刷新标志，下半部分（bit2、bit3）标志保留
        OLED_buffer_flag[i] &= 0xFC; // 0xFC = 11111100，仅保留bit2、bit3
    }
}

// 只刷新下半部分（页2、页3，对应y=16~31），完全不影响上半部分
void OLED_Refresh_Lower_Half(void)
{
    uint8_t l = 0, r;
    // 只处理下半部分（页2、页3），忽略页0、页1的刷新标志
    while (l <= OLED_Width)
    {
        // 找到需要刷新的起始列（仅判断页2、页3的标志，0x0C = bit2+bit3）
        while (l <= OLED_Width && !(OLED_buffer_flag[l] & 0x0C)) l++; 
        
        r = l;
        // 找到需要刷新的结束列（仅判断页2、页3的标志）
        while (r <= OLED_Width && (OLED_buffer_flag[r] & 0x0C)) r++;
        r--;

        if (l <= OLED_Width)
        {
            // 手动指定刷新范围：列l~r，页2~3（下半部分）
            uint8_t x, page;
            OLED_SetVerticalPos(l, 2, 3); // 限定上下页为2和3（下半部分）
            
            // 只写入下半部分缓存数据
            for (x = l; x <= r; x++)
                for (page = 2; page <= 3; page++)
                    OLED_Write_Byte(DataReg, OLED_buffer[page][x]);
            
            // 仅清除下半部分（bit2、bit3）的刷新标志，保留上半部分标志
            for (x = l; x <= r; x++)
                OLED_buffer_flag[x] &= 0x03; // 0x03 = 00000011，清除bit2、bit3，保留bit0、bit1
        }

        l = r + 1;
    }
}

void OLED_Refresh_Time_Area(void)
{
    uint8_t x, page;
    // 1. 限定时间区域的坐标范围：x=32~42（"HH:MM:SS"占11个字符，每个字符6列，共32+6*5=62，可按需调整）
    //    y=16~23 → 对应页2
    uint8_t time_x_start = 32;
    uint8_t time_x_end = 32 + 6*5; // 5个分隔符+数字，足够容纳"HH:MM:SS"
    uint8_t time_page_start = 2;
    uint8_t time_page_end = 2;     // 时间字符串高度8像素，仅占用页2

    // 2. 设置OLED坐标到时间区域
    OLED_SetVerticalPos(time_x_start, time_page_start, time_page_end);

    // 3. 仅写入时间区域的缓存数据
    for (x = time_x_start; x <= time_x_end; x++)
        for (page = time_page_start; page <= time_page_end; page++)
            OLED_Write_Byte(DataReg, OLED_buffer[page][x]);

    // 4. 清除时间区域对应的刷新标志
    for (x = time_x_start; x <= time_x_end; x++)
        OLED_buffer_flag[x] &= ~(0x0C); // 仅清除时间区域的下半部分标志
}
/**
 * @description: 提取指定长度的二进制数据
 * @param {uint8_t} *data 提取源
 * @param {uint8_t} start 起始位置
 * @param {uint8_t} size  提取长度
 * @return {uint8_t}      最长为8位的结果
 */
static uint8_t getInt8Data(uint8_t *data, uint8_t start, uint8_t size) {
    uint8_t ret = 0, delta;

    ret = data[start / 8] >> (start % 8);
    delta = 8 - (start % 8);
    size -= delta;
    start += delta;

    if (((int8_t)size) <= 0)
        return ret & (0xFF >> (-((int8_t)size)));
    else
        return ret | (getInt8Data(data, start, size) << delta);
}

/**
 * @description: 从指定像素点向下绘制不定长一列
 * @param {uint8_t} x   像素点横坐标
 * @param {uint8_t} y   像素点纵坐标
 * @param {uint8_t} *data   绘制数据源
 * @param {uint8_t} size    绘制长度
 * @param {uint8_t} bool    布尔计算类型
 * @return {HAL_StatusTypeDef}  HAL状态
 */
HAL_StatusTypeDef OLED_BOOL_DrawColumn(uint8_t x, uint8_t y, uint8_t *data,
                                       uint8_t size, uint8_t bool) {
    if (x >= OLED_Width || y + size > OLED_High) return HAL_ERROR;

    uint8_t pos = 0;
    while (size > 0) {
        uint8_t data_n = OLED_buffer[y / 8][x], delta;

        if (bool == OLED_BOOL_Replace)
            if (8 - (y % 8) >= size) {
                data_n = data_n - (data_n & ((0xff & (0xff << (8 - size))) >>
                                             (8 - size - (y % 8))));
                data_n |= getInt8Data(data, pos, size) << (y % 8);
                delta = size;
            } else {
                data_n = data_n - (data_n & (0xff << (y % 8)));
                data_n |= getInt8Data(data, pos, 8 - (y % 8)) << (y % 8);
                delta = 8 - (y % 8);
            }
        else if (bool == OLED_BOOL_ADD)
            if (8 - (y % 8) >= size) {
                data_n |= getInt8Data(data, pos, size) << (y % 8);
                delta = size;
            } else {
                data_n |= getInt8Data(data, pos, 8 - (y % 8)) << (y % 8);
                delta = 8 - (y % 8);
            }
        else if (bool == OLED_BOOL_Subtract)
            if (8 - (y % 8) >= size) {
                data_n &= ~(getInt8Data(data, pos, size) << (y % 8));
                delta = size;
            } else {
                data_n &= ~(getInt8Data(data, pos, 8 - (y % 8)) << (y % 8));
                delta = 8 - (y % 8);
            }

        if (data_n != OLED_buffer[y / 8][x])
            OLED_buffer_flag[x] |= 1 << (y / 8);
        OLED_buffer[y / 8][x] = data_n;

        size -= delta;
        y += delta;
        pos += delta;
    }

    return HAL_OK;
}

/**
 * @description: 从指定像素点向下绘制不定长一列
 * @param {uint8_t} x   像素点横坐标
 * @param {uint8_t} y   像素点纵坐标
 * @param {uint8_t} *data   绘制数据源
 * @param {uint8_t} size    绘制长度
 * @return {HAL_StatusTypeDef}  HAL状态
 */
HAL_StatusTypeDef OLED_DrawColumn(uint8_t x, uint8_t y, uint8_t *data,
                                  uint8_t size) {
    return OLED_BOOL_DrawColumn(x, y, data, size, OLED_BOOL_Replace);
}

/**
 * @description: 以指定像素点为左上角，绘制单个字符
 * @param {uint8_t} x   横坐标
 * @param {uint8_t} y   纵坐标
 * @param {uint8_t} c   字符
 * @param {uint8_t} bool    布尔计算类型
 * @return {HAL_StatusTypeDef}  HAL 状态
 */
HAL_StatusTypeDef OLED_BOOL_DrawChar(uint8_t x, uint8_t y, uint8_t c,
                                     uint8_t bool) {
    return OLED_BOOL_DrawBMP(x, y, 6, 8, ((uint8_t *)ASCII) + 6 * (c - 32),
                             bool);
}

/**
 * @description: 以指定像素点为左上角，绘制单个字符
 * @param {uint8_t} x   横坐标
 * @param {uint8_t} y   纵坐标
 * @param {uint8_t} c   字符
 * @return {HAL_StatusTypeDef}  HAL 状态
 */
HAL_StatusTypeDef OLED_DrawChar(uint8_t x, uint8_t y, uint8_t c) {
    return OLED_BOOL_DrawBMP(x, y, 6, 8, ((uint8_t *)ASCII) + 6 * (c - 32),
                             OLED_BOOL_Replace);
}
/**
 * @description: 以指定像素点为左上角，绘制字符串
 * @param {uint8_t} x   横坐标
 * @param {uint8_t} y   纵坐标
 * @param {uint8_t} *str    字符串
 * @param {uint8_t} bool    布尔计算类型
 * @return {HAL_StatusTypeDef}  HAL 状态
 */
HAL_StatusTypeDef OLED_BOOL_DrawStr(uint8_t x, uint8_t y, uint8_t *str,
                                    uint8_t bool) {
    uint16_t i = 0;

    uint8_t ret = 0;

    while (str[i] != '\0') {
        if (x + 5 >= OLED_Width) {
            x = 0;
            y += 8;
        }

        ret |= OLED_BOOL_DrawChar(x, y, str[i], bool);

        x += 6;
        i++;
    }

    return ret ? HAL_ERROR : HAL_OK;
}

/**
 * @description: 以指定像素点为左上角，绘制字符串
 * @param {uint8_t} x   横坐标
 * @param {uint8_t} y   纵坐标
 * @param {uint8_t} *str    字符串
 * @return {HAL_StatusTypeDef}  HAL 状态
 */
HAL_StatusTypeDef OLED_DrawStr(uint8_t x, uint8_t y, uint8_t *str) {
    return OLED_BOOL_DrawStr(x, y, str, OLED_BOOL_Replace);
}

/**
 * @description: 绘制位图
 * @param {uint8_t} x   横坐标
 * @param {uint8_t} y   纵坐标
 * @param {uint8_t} width   宽度
 * @param {uint8_t} high    高度
 * @param {uint8_t} *data   数据
 * @param {uint8_t} bool    布尔计算类型
 * @return {HAL_StatusTypeDef}  HAL状态
 */
HAL_StatusTypeDef OLED_BOOL_DrawBMP(uint8_t x, uint8_t y, uint8_t width,
                                    uint8_t high, uint8_t *data, uint8_t bool) {
    uint8_t ret = 0;
    while (width > 0) {
        ret |= OLED_BOOL_DrawColumn(x, y, data, high, bool);
        data += (high + 7) / 8;
        width--;
        x++;
    }

    return ret ? HAL_ERROR : HAL_OK;
}

/**
 * @description: 绘制位图
 * @param {uint8_t} x   横坐标
 * @param {uint8_t} y   纵坐标
 * @param {uint8_t} width   宽度
 * @param {uint8_t} high    高度
 * @param {uint8_t} *data   数据
 * @return {HAL_StatusTypeDef}  HAL状态
 */
HAL_StatusTypeDef OLED_DrawBMP(uint8_t x, uint8_t y, uint8_t width,
                               uint8_t high, uint8_t *data) {
    return OLED_BOOL_DrawBMP(x, y, width, high, data, OLED_BOOL_Replace);
}

// 次方计算辅助函数
uint32_t OLED_Pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while(n--)
        result *= m;
    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len)
{
    uint8_t t, i;
    uint8_t enshow = 0;

    for(i = 0; i < len; i++)
    {
        t = (num / OLED_Pow(10, len - i - 1)) % 10;

        if(enshow == 0 && i < (len - 1))
        {
            if(t == 0)
            {
                OLED_DrawChar(x + 6 * i, y, ' ');
                continue;
            }
            else
                enshow = 1;
        }
        OLED_DrawChar(x + 6 * i, y, t + '0');
    }
}


void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t int_len, uint8_t dec_len)
{
    char buf[20] = {0};   // 关键：初始化为0，确保没有脏数据
    char format[10];

    // 格式化：例如 2位整数 + 2位小数
    sprintf(format, "%%%d.%df", int_len, dec_len);
    sprintf(buf, format, num);

    // ==============================================
    // 🔥 核心修复：先把这一行的位置清空（用空格覆盖）
    // ==============================================
    uint8_t total_len = int_len + dec_len + 1;  // 整数+小数+小数点
    for(uint8_t i=0; i<total_len; i++)
    {
        OLED_DrawChar(x + 6*i, y, ' ');
    }

    // 然后再显示新数字
    OLED_DrawStr(x, y, (uint8_t *)buf);
}

