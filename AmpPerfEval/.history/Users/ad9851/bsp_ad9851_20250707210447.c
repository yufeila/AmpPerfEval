#include "./ad9851/bsp_ad9851.h"
#include <stdio.h>

//***************************************************//
//              ad9851复位(串口模式)                 //
//---------------------------------------------------//
void ad9851_reset_serial(void)
{
    // 拉低时钟与更新引脚
    digitalL(AD9851_W_CLK_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    digitalL(AD9851_FQ_UP_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin

    // reset信号:先拉低,再拉高,最后再拉低
    digitalL(AD9851_RESET_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin
    digitalH(AD9851_RESET_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin
    digitalL(AD9851_RESET_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin

    // w_clk信号:拉低->拉高->拉低
    digitalL(AD9851_W_CLK_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    digitalH(AD9851_W_CLK_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    digitalL(AD9851_W_CLK_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin

    // fq_up信号:拉低->拉高->拉低
    digitalL(AD9851_FQ_UP_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
    digitalH(AD9851_FQ_UP_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
    digitalL(AD9851_FQ_UP_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
}

//***************************************************//
//          向ad9851中写命令与数据(串口)             //
//---------------------------------------------------//
void ad9851_wr_serial(uint8_t w0, double frequence)
{
    unsigned char i, w;
    long int y;
    double x;

    // 计算频率的HEX值(30MHz * 6倍频 = 180MHz)
    x = 4294967295.0 / 180.0;  // 系统主频为180MHz
    frequence = frequence / 1000000.0;  // 单位换算为MHz
    frequence = frequence * x;  // 计算频率控制字
    y = (long int)frequence;

    // 写w4数据(LSB)
    w = (y >> 0);
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(AD9851_BIT_DATA_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(AD9851_BIT_DATA_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }

        digitalH(AD9851_W_CLK_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
        digitalL(AD9851_W_CLK_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    }

    // 写w3数据
    w = (y >> 8);
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(AD9851_BIT_DATA_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(AD9851_BIT_DATA_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }

        digitalH(AD9851_W_CLK_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
        digitalL(AD9851_W_CLK_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    }

    // 写w2数据
    w = (y >> 16);
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(GPIOB, GPIO_PIN_9);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(GPIOB, GPIO_PIN_9);  // AD9851_BIT_DATA_Pin
        }

        digitalH(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
        digitalL(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
    }

    // 写w1数据
    w = (y >> 24);
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(GPIOB, GPIO_PIN_9);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(GPIOB, GPIO_PIN_9);  // AD9851_BIT_DATA_Pin
        }

        digitalH(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
        digitalL(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
    }

    // 写w0数据（控制字）
    w = w0;
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(GPIOB, GPIO_PIN_9);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(GPIOB, GPIO_PIN_9);  // AD9851_BIT_DATA_Pin
        }

        digitalH(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
        digitalL(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
    }

    // fq_up信号：拉高->拉低，移入使能
    digitalH(GPIOB, GPIO_PIN_7);  // AD9851_FQ_UP_Pin
    digitalL(GPIOB, GPIO_PIN_7);  // AD9851_FQ_UP_Pin
}


//***************************************************//
//              ad9851复位(并口模式)                 //
//---------------------------------------------------//
void ad9851_reset_parallel(void)
{
    // 并口模式复位序列
    // 先设置所有控制引脚为低电平
    digitalL(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
    digitalL(GPIOB, GPIO_PIN_7);  // AD9851_FQ_UP_Pin
    
    // reset信号:先拉低,再拉高,最后再拉低
    digitalL(GPIOB, GPIO_PIN_8);  // AD9851_RESET_Pin
    digitalH(GPIOB, GPIO_PIN_8);  // AD9851_RESET_Pin
    digitalL(GPIOB, GPIO_PIN_8);  // AD9851_RESET_Pin
}

//***************************************************//
//          向ad9851中写命令与数据(并口)             //
//---------------------------------------------------//
void ad9851_wr_parallel(uint8_t w0, double frequence)
{
    unsigned char i;
    long int y;
    double x;
    uint8_t freq_bytes[4];

    // 计算频率的HEX值(30MHz * 6倍频 = 180MHz)
    x = 4294967295.0 / 180.0;  // 修正为180MHz
    frequence = frequence / 1000000.0;
    frequence = frequence * x;
    y = (long int)frequence;

    // 将32位频率数据分解为4个字节
    freq_bytes[0] = (y >> 0) & 0xFF;   // W4 (LSB)
    freq_bytes[1] = (y >> 8) & 0xFF;   // W3
    freq_bytes[2] = (y >> 16) & 0xFF;  // W2
    freq_bytes[3] = (y >> 24) & 0xFF;  // W1

    // 发送W4-W1数据(4个字节的频率调谐字)
    for(i = 0; i < 4; i++)
    {
        // 产生写时钟脉冲
        digitalH(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
        digitalL(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
    }
    
    // 发送W0控制字
    digitalH(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin
    digitalL(GPIOB, GPIO_PIN_6);  // AD9851_W_CLK_Pin

    // fq_up信号：拉高->拉低，移入使能
    digitalH(GPIOB, GPIO_PIN_7);  // AD9851_FQ_UP_Pin
    digitalL(GPIOB, GPIO_PIN_7);  // AD9851_FQ_UP_Pin
}


//***************************************************//
//                   初始化函数                  //
//---------------------------------------------------//
// 输入：mode  AD9851_SERIAL_MODE 串口    AD9851_PARALLEL_MODE 并口
// FD：AD9851_FD_DISABLE：不倍频 AD9851_FD_ENABLE：6倍频
void AD9851_Init(uint8_t mode, uint8_t FD)
{
    // GPIO配置已在MX_GPIO_Init()中完成，这里只需要初始化引脚状态
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);  // AD9851_W_CLK_Pin
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);  // AD9851_FQ_UP_Pin
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);  // AD9851_RESET_Pin
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);  // AD9851_BIT_DATA_Pin
    
    // 根据模式选择复位方式
    if(mode == AD9851_SERIAL_MODE)
    {
        ad9851_reset_serial();
    }
    else if(mode == AD9851_PARALLEL_MODE)
    {
        ad9851_reset_parallel();
    }
    
    // 设置测试频率1kHz
    AD9851_Setfq(mode, FD, 2000000);
}

//***************************************************//
//                   设置频率函数                    //
//---------------------------------------------------//
void AD9851_Setfq(uint8_t mode, uint8_t FD, double frequence)
{
    uint8_t control_word = 0x00;
    
    // 根据倍频设置构造控制字(bit0为6倍频控制位)
    if(FD == AD9851_FD_ENABLE)
        control_word |= 0x01;  // 设置6倍频位(根据AD9851数据手册，bit0控制6倍频)
    
    // 根据工作模式调用相应的写入函数
    if(mode == AD9851_SERIAL_MODE)
    {
        ad9851_wr_serial(control_word, frequence);
    }
    else if(mode == AD9851_PARALLEL_MODE)
    {
        ad9851_wr_parallel(control_word, frequence);
    }
}

/**
 * @brief 设置AD9851输出频率 (简化版本，用于频率响应测量)
 * @param frequency 目标频率 (Hz)
 * @retval None
 */
void AD9851_Set_Frequency(float frequency)
{
    // 使用串口模式，启用6倍频
    uint8_t mode = AD9851_SERIAL_MODE;
    uint8_t FD = AD9851_FD_ENABLE;
    
    // 转换为double类型调用原有函数
    double freq_double = (double)frequency;
    
    // 调用原有的设置频率函数
    AD9851_Setfq(mode, FD, freq_double);
    
    // 添加小延时确保频率设置完成
    HAL_Delay(1);
    
}


