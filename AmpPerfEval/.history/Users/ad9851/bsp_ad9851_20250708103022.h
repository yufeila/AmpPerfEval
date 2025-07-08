/*
 * @Author: 杨宇菲 17786321727@163.com
 * @Date: 2025-06-30 15:51:28
 * @LastEditors: yyf 17786321727@163.com
 * @LastEditTime: 2025-07-08 10:30:22
 * @FilePath: \AmpPerfEval\Users\ad9851\bsp_ad9851.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __BSP_AD9851_H
#define __BSP_AD9851_H

/* includes */
#include "stm32f4xx_hal.h"
#include "main.h"  // 包含引脚定义
#include <stdint.h>

//***************************************************//
//              引脚与板上引脚对照关系表             //
//---------------------------------------------------//
// GPIOB_PIN_4: AD9851_RESET_Pin (复位)
// GPIOB_PIN_5: AD9851_BIT_DATA_Pin (数据位)
// GPIOB_PIN_6: AD9851_W_CLK_Pin (写时钟)
// GPIOB_PIN_7: AD9851_FQ_UP_Pin (频率更新)


#define AD9851_W_CLK_PIN GPIO_PIN_6
#define AD9851_W_CLK_GPIO_PORT GPIOB
#define AD9851_FQ_UP_PIN GPIO_PIN_7
#define AD9851_FQ_UP_GPIO_PORT GPIOB
#define AD9851_RESET_PIN GPIO_PIN_4
#define AD9851_RESET_GPIO_PORT GPIOB
#define AD9851_BIT_DATA_PIN GPIO_PIN_5
#define AD9851_BIT_DATA_GPIO_PORT GPIOB


/* GPIO Pin Definitions */
#define digitalH(port, pin)  HAL_GPIO_WritePin((port), (pin), GPIO_PIN_SET)
#define digitalL(port, pin)  HAL_GPIO_WritePin((port), (pin), GPIO_PIN_RESET)
// 如果main.h中已经定义了这些宏，直接使用即可

/* AD9851 Mode Definitions */
#define AD9851_SERIAL_MODE      0    // 串口模式
#define AD9851_PARALLEL_MODE    1    // 并口模式

/* Frequency Doubler Definitions */
#define AD9851_FD_DISABLE       0    // 不倍频
#define AD9851_FD_ENABLE        1    // 6倍频使能

/* Prototypes */
void ad9851_reset_serial(void);
void ad9851_reset_parallel(void);
void ad9851_wr_serial(uint8_t w0, double frequence);
void ad9851_wr_parallel(uint8_t w0, double frequence);
void AD9851_Init(uint8_t mode, uint8_t FD);
void AD9851_Setfq(uint8_t mode, uint8_t FD, double frequence);
void AD9851_Set_Frequency(float frequency);

#endif /* __BSP_AD9851_H */

