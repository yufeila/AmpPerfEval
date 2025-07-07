/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.h
  * @brief   This file contains all the function prototypes for
  *          the adc.c file
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
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
// 缓存区状态定义
#define BUFFER_READY_FLAG_NONE      0
#define BUFFER_READY_FLAG_FULL      1

// FFT相关参数定义
#define ADC_CHANNELS 3
#define BUF_SIZE FFT_SIZE * ADC_CHANNELS


/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc1;
extern volatile uint16_t adc_buffer[BUF_SIZE];
extern volatile uint8_t ADC_BufferReadyFlag;

// 调试变量声明 - 供Keil Watch窗口查看
extern volatile uint32_t debug_ch2_avg, debug_ch4_avg, debug_ch6_avg;
extern volatile uint32_t debug_ch2_range, debug_ch4_range, debug_ch6_range;
extern volatile uint32_t debug_buffer_ready_count;

/* USER CODE BEGIN Private defines */
#define Start_ADC_DMA   HAL_ADC_Start_DMA(&hadc1, adc_buffer, BUF_SIZE)
/* USER CODE END Private defines */

void MX_ADC1_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */

