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
/* ����Ķ�ʱ����0.05%�� */
#define FS_ERR_MAX      0.0005f     

/* ADC��ͨ��ɨ����ز��� */
#define ADC_CHANNELS    3             // ADCͨ����
#define ADC_CLK_HZ      21000000U     // ADCʱ��Ƶ�� (84MHz/4 = 21MHz)
#define ADC_SAMPLE_TIME_CYCLES  15U   // ����ʱ�������� (15������)
#define ADC_CONV_TIME_CYCLES   12U    // ת��ʱ�������� (12������)
#define ADC_TOTAL_CYCLES_PER_CH (ADC_SAMPLE_TIME_CYCLES + ADC_CONV_TIME_CYCLES) // ÿͨ����������
#define ADC_TOTAL_CYCLES_ALL_CH (ADC_TOTAL_CYCLES_PER_CH * ADC_CHANNELS) // ����ͨ����������

/* ����ADCɨ��һ�ֵ���Сʱ�� (΢��) */
#define ADC_SCAN_TIME_US ((float)ADC_TOTAL_CYCLES_ALL_CH / ADC_CLK_HZ * 1000000.0f)
/* ������������ (Hz) - ��ʱ���������ڱ����ADCɨ��ʱ�� */
#define TIM_MAX_TRIGGER_FREQ (1000000.0f / ADC_SCAN_TIME_US)
/* USER CODE END Private defines */

void MX_TIM2_Init(void);

/* USER CODE BEGIN Prototypes */
float Tim2_Config_AutoFs(float f0_hz);
extern float g_current_Fs;  // ��ǰ������
float Tim2_Config_Channel_Fs(float target_fs); // ֱ�����ò�����
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

