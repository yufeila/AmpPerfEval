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
/* USER CODE END Includes */

extern TIM_HandleTypeDef htim2;

/* USER CODE BEGIN Private defines */
#define TIM_CLK_HZ      84000000U     // TIM2 clock (APB1*2)
/* 允许的定时器误差（0.05%） */
#define FS_ERR_MAX      0.0005f     

/* ADC多通道扫描相关参数 */
#define ADC_CHANNELS    3             // ADC通道数
#define ADC_CLK_HZ      21000000U     // ADC时钟频率 (84MHz/4 = 21MHz)
#define ADC_SAMPLE_TIME_CYCLES  15U   // 采样时间周期数 (15个周期)
#define ADC_CONV_TIME_CYCLES   12U    // 转换时间周期数 (12个周期)
#define ADC_TOTAL_CYCLES_PER_CH (ADC_SAMPLE_TIME_CYCLES + ADC_CONV_TIME_CYCLES) // 每通道总周期数
#define ADC_TOTAL_CYCLES_ALL_CH (ADC_TOTAL_CYCLES_PER_CH * ADC_CHANNELS) // 所有通道总周期数

/* 计算ADC扫描一轮的最小时间 (微秒) */
#define ADC_SCAN_TIME_US ((float)ADC_TOTAL_CYCLES_ALL_CH / ADC_CLK_HZ * 1000000.0f)
/* 计算最大采样率 (Hz) - 定时器触发周期必须≥ADC扫描时间 */
#define TIM_MAX_TRIGGER_FREQ (1000000.0f / ADC_SCAN_TIME_US)
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

