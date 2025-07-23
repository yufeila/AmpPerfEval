/*
 * @Author: yyf 17786321727@163.com
 * @Date: 2025-07-07 13:03:52
 * @LastEditors: yyf 17786321727@163.com
 * @LastEditTime: 2025-07-23 20:16:39
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

// 故障检测控制按键 PA0
#define KEY_UP_PIN GPIO_PIN_0
#define KEY_UP_PORT GPIOA

// 系统状态定义
#define BASIC_MEASUREMENT_STATE           1
#define SWEEP_FREQ_RESPONSE_STATE         2
#define FAULT_DETECTION_STATE             3

// 外部变量声明
extern uint8_t basic_measurement_flag;      // 基本测量标志位
extern uint8_t sweep_freq_response_flag;    // 扫频响应测量标志位
extern uint8_t measure_r_out_flag;          // 输出阻抗测量标志位
extern uint8_t current_system_state;        // 当前系统状态
ex

// 函数声明
void KEY_Scan(void);

#endif /* key.h */
