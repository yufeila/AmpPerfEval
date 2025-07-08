/*
 * @Author: yyf 17786321727@163.com
 * @Date: 2025-07-07 13:03:52
 * @LastEditors: yyf 17786321727@163.com
 * @LastEditTime: 2025-07-07 15:58:02
 * @FilePath: /AmpPerfEval/Core/Inc/tim.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"


/* USER CODE BEGIN Includes */
#include <math.h>
#include "stm32f4xx_hal.h"
/* USER CODE END Includes */

extern TIM_HandleTypeDef htim2;

/* USER CODE BEGIN Private defines */
#define TIM_CLK_HZ      84000000U     // TIM2 clock (APB1*2)
/* 允许的定时器误差（0.05%） */
#define FS_ERR_MAX      0.0005f     

/* ADC多通道扫描相关参数 */
#define ADC_CHANNELS    3             // ADC通道数
#define ADC_CLK_HZ      21000000U     // ADC时钟频率 (84MHz/4 = 21MHz)
#define ADC_SAMPLE_TIME_CYCLES  15U   // 采样时间周期数 (15个周期，提高高频稳定性)
#define ADC_CONV_TIME_CYCLES   12.5f  // 转换时间周期数 (12.5个周期，STM32固定)
#define ADC_TOTAL_CYCLES_PER_CH (ADC_SAMPLE_TIME_CYCLES + ADC_CONV_TIME_CYCLES) // 每通道总周期数 = 27.5
#define ADC_TOTAL_CYCLES_ALL_CH (ADC_TOTAL_CYCLES_PER_CH * ADC_CHANNELS) // 所有通道总周期数 = 82.5

/* 
 * 计算示例（更新为15周期采样时间）:
 * 单通道转换时间 = 27.5 / 21MHz ≈ 1.31μs
 * 3通道序列时间 = 82.5 / 21MHz ≈ 3.93μs  
 * 最大采样率 = 1 / 3.93μs ≈ 254kHz
 * 
 * 性能等级:
 * - 保守配置: 200kHz (留足裕量，推荐用于高精度测量)
 * - 标准配置: 250kHz (接近理论最大值)  
 * - 极限配置: 254kHz (理论最大值，仅在必要时使用)
 */
#define ADC_SCAN_TIME_US ((float)ADC_TOTAL_CYCLES_ALL_CH / ADC_CLK_HZ * 1000000.0f)
/* 计算最大采样率 (Hz) - 定时器触发周期必须≥ADC扫描时间 */
#define TIM_MAX_TRIGGER_FREQ (1000000.0f / ADC_SCAN_TIME_US)

/* 推荐的采样率配置 */
#define SAMPLING_RATE_CONSERVATIVE  200000U   // 200kHz，保守配置，高稳定性
#define SAMPLING_RATE_STANDARD      400000U   // 400kHz，标准高速配置  
#define SAMPLING_RATE_MAXIMUM       ((uint32_t)TIM_MAX_TRIGGER_FREQ)  // 理论最大值

/* 采样率限制 */
#define SAMPLING_RATE_MIN           10000U    // 最小采样率10kHz，防止低频信号采样异常
#define SAMPLING_RATE_SAFE_MIN      20000U    // 推荐最小采样率20kHz，更安全的下限

#ifndef __HAL_TIM_GET_PRESCALER
#define __HAL_TIM_GET_PRESCALER(__HANDLE__) ((__HANDLE__)->Instance->PSC)
#endif

#ifndef __HAL_TIM_GET_AUTORELOAD
#define __HAL_TIM_GET_AUTORELOAD(__HANDLE__) ((__HANDLE__)->Instance->ARR)
#endif

/* USER CODE END Private defines */

void MX_TIM2_Init(void);

/* USER CODE BEGIN Prototypes */
float Tim2_Config_AutoFs(float f0_hz);
extern float g_current_Fs;  // 当前采样率
float Tim2_Config_Channel_Fs(float target_fs); // 直接配置采样率
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

