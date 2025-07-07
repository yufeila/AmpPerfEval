/*
 * @Author: yyf 17786321727@163.com
 * @Date: 2025-07-07 13:03:52
 * @LastEditors: yyf 17786321727@163.com
 * @LastEditTime: 2025-07-07 15:05:58
 * @FilePath: /AmpPerfEval/Users/key/key.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __KEY_H
#define __KEY_H

#include "stm32f4xx_hal.h"

#define KEY0_PIN GPIO_PIN_8
#define KEY0_PORT GPIOB

#define KEY1_PIN GPIO_PIN_9
#define KEY1_PORT GPIOB

#define RELAY_PIN GPIO_PIN_12
#define RELAY_PORT GPIOB

#define RELAY_ON HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_SET)

#define BASIC_MEASUREMENT_STATE           1
#define MEASURE_R_OUT_FLAG               2
#define SWEEP_FREQ_RESPONSE_FLAG         3

// 外部变量声明
extern uint8_t basic_measurement;
extern uint8_t sweep_freq_response;
extern uint8_t measure_r_out;

// 函数声明
void KEY_Scan(void);

#endif /* key.h */
