#include "./ad9851/bsp_ad9851.h"
#include <stdio.h>

//***************************************************//
//              ad9851复位(串口模式)                 //
//---------------------------------------------------//
void ad9851_reset_serial(void)
{
    // 拉低时钟与更新引脚
    digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    digitalL(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin

    // reset信号:先拉低,再拉高,最后再拉低
    digitalL(AD9851_RESET_GPIO_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin
    digitalH(AD9851_RESET_GPIO_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin
    digitalL(AD9851_RESET_GPIO_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin

    // w_clk信号:拉低->拉高->拉低
    digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    digitalH(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin

    // fq_up信号:拉低->拉高->拉低
    digitalL(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
    digitalH(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
    digitalL(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
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
            digitalH(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }

        digitalH(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
        digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    }

    // 写w3数据
    w = (y >> 8);
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }

        digitalH(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
        digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    }

    // 写w2数据
    w = (y >> 16);
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }

        digitalH(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
        digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    }

    // 写w1数据
    w = (y >> 24);
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }

        digitalH(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
        digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    }

    // 写w0数据（控制字）
    w = w0;
    for(i = 0; i < 8; i++)
    {
        if((w >> i) & 0x01)
        {
            digitalH(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }
        else
        {
            digitalL(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN);  // AD9851_BIT_DATA_Pin
        }

        digitalH(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
        digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    }

    // fq_up信号：拉高->拉低，移入使能
    digitalH(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
    digitalL(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
}


//***************************************************//
//              ad9851复位(并口模式)                 //
//---------------------------------------------------//
void ad9851_reset_parallel(void)
{
    // 并口模式复位序列
    // 先设置所有控制引脚为低电平
    digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    digitalL(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
    
    // reset信号:先拉低,再拉高,最后再拉低
    digitalL(AD9851_RESET_GPIO_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin
    digitalH(AD9851_RESET_GPIO_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin
    digitalL(AD9851_RESET_GPIO_PORT, AD9851_RESET_PIN);  // AD9851_RESET_Pin
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
        digitalH(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
        digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    }
    
    // 发送W0控制字
    digitalH(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin
    digitalL(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN);  // AD9851_W_CLK_Pin

    // fq_up信号：拉高->拉低，移入使能
    digitalH(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
    digitalL(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN);  // AD9851_FQ_UP_Pin
}


//***************************************************//
//                   初始化函数                  //
//---------------------------------------------------//
// 输入：mode  AD9851_SERIAL_MODE 串口    AD9851_PARALLEL_MODE 并口
// FD：AD9851_FD_DISABLE：不倍频 AD9851_FD_ENABLE：6倍频
void AD9851_Init(uint8_t mode, uint8_t FD)
{
    // GPIO配置已在MX_GPIO_Init()中完成，这里只需要初始化引脚状态
    HAL_GPIO_WritePin(AD9851_W_CLK_GPIO_PORT, AD9851_W_CLK_PIN, GPIO_PIN_RESET);  // AD9851_W_CLK_Pin
    HAL_GPIO_WritePin(AD9851_FQ_UP_GPIO_PORT, AD9851_FQ_UP_PIN, GPIO_PIN_RESET);  // AD9851_FQ_UP_Pin
    HAL_GPIO_WritePin(AD9851_RESET_GPIO_PORT, AD9851_RESET_PIN, GPIO_PIN_RESET);  // AD9851_RESET_Pin
    HAL_GPIO_WritePin(AD9851_BIT_DATA_GPIO_PORT, AD9851_BIT_DATA_PIN, GPIO_PIN_RESET);  // AD9851_BIT_DATA_Pin
    
    // 根据模式选择复位方式
    if(mode == AD9851_SERIAL_MODE)
    {
        ad9851_reset_serial();
    }
    else if(mode == AD9851_PARALLEL_MODE)
    {
        ad9851_reset_parallel();
    }
    
    // 设置测试频率10kHz
    AD9851_Setfq(mode, FD, 10000);
}

//***************************************************//
//                   设置频率函数                    //
//---------------------------------------------------//
void AD9851_Setfq(uint8_t mode, uint8_t FD, double frequence)
{
    uint8_t control_word = AD9851_CTRL_NORMAL;  // 默认正常工作模式
    
    // 根据倍频设置构造控制字
    if(FD == AD9851_FD_ENABLE)
        control_word |= AD9851_FD_ENABLE_BIT;  // 设置6倍频位
    
    // 确保Power-Down位(D7)为0，防止进入休眠模式
    control_word &= ~AD9851_POWER_DOWN_D7;  // 清除D7位，确保正常工作
    
    // 检查控制字是否正确
    AD9851_Check_Control_Word(control_word);
    
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
/**
 * @brief 强制退出AD9851休眠模式
 * @retval None
 */
void AD9851_Exit_Power_Down(void)
{
    // 发送一个正常的控制字，确保D7位为0
    uint8_t control_word = AD9851_CTRL_NORMAL;  // 正常工作模式，D7=0
    
    // 使用串口模式发送控制字
    ad9851_wr_serial(control_word, 1000000.0);  // 1MHz测试频率
    
    HAL_Delay(5);  // 等待芯片稳定
    
    #if AD9851_DEBUG_ENABLE
    printf("AD9851: Exit Power-Down mode, Control Word: 0x%02X\r\n", control_word);
    #endif
}

/**
 * @brief 检查AD9851控制字是否正确设置
 * @param control_word 控制字
 * @retval 1: 正常, 0: 可能有问题
 */
uint8_t AD9851_Check_Control_Word(uint8_t control_word)
{
    // 检查D7位是否为0（Power-Down位）
    if(control_word & AD9851_POWER_DOWN_D7)
    {
        #if AD9851_DEBUG_ENABLE
        printf("AD9851 WARNING: Power-Down bit (D7) is set! Control Word: 0x%02X\r\n", control_word);
        #endif
        return 0;  // 有问题
    }
    
    #if AD9851_DEBUG_ENABLE
    printf("AD9851: Control Word OK: 0x%02X\r\n", control_word);
    #endif
    
    return 1;  // 正常
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


