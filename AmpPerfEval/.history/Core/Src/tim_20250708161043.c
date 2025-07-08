/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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
/* Includes ------------------------------------------------------------------*/
#include "tim.h"

/* USER CODE BEGIN 0 */
// ȫ�ֱ��������ڴ洢��ǰ������
float g_current_Fs = 1000.0f;  // Ĭ��1kHz
/* USER CODE END 0 */

TIM_HandleTypeDef htim2;

/* TIM2 init function */
void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspInit 0 */

  /* USER CODE END TIM2_MspInit 0 */
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();
  /* USER CODE BEGIN TIM2_MspInit 1 */

  /* USER CODE END TIM2_MspInit 1 */
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspDeInit 0 */

  /* USER CODE END TIM2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM2_CLK_DISABLE();
  /* USER CODE BEGIN TIM2_MspDeInit 1 */

  /* USER CODE END TIM2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* --------------- �������� PSC/ARR ----------------- */
static void tim2_set_fs(uint32_t Fs_target)
{
    uint32_t best_psc = 0, best_arr = 0;
    float    best_err = 1.0f;

    /* ���������ȴ��� PSC����΢�� ARR */
    for (uint32_t psc = 0; psc < 65536 && psc < TIM_CLK_HZ / Fs_target; ++psc)
    {
        uint32_t prod = TIM_CLK_HZ / Fs_target / (psc + 1);
        if (prod == 0) break;
        uint32_t arr  = prod - 1;
        if (arr > 0xFFFF) continue;

        float Fs_real = (float)TIM_CLK_HZ / ((psc + 1u) * (arr + 1u));
        float err     = fabsf(Fs_real - (float)Fs_target) / Fs_target;
        if (err < best_err) {
            best_err  = err;
            best_psc  = psc;
            best_arr  = arr;
            if (err < FS_ERR_MAX) break;          // �Ѿ��㹻׼
        }
    }

    /* д�Ĵ��� */
    __HAL_TIM_DISABLE(&htim2);
    __HAL_TIM_SET_PRESCALER (&htim2, best_psc);
    __HAL_TIM_SET_AUTORELOAD(&htim2, best_arr);
    __HAL_TIM_SET_COUNTER   (&htim2, 0);
    __HAL_TIM_ENABLE(&htim2);
}

/* ----------------------------------------------------------
 * ���ݸ�������Ƶ�� f0_hz (Hz)  �Զ�ѡ������� Fs��
 * ���� TIM2  (PSC / ARR) ��������ʵ�ʲ����� (Hz)
 * 
 * ��Ҫ��ÿ��ͨ���Ĳ����� = ��ʱ������Ƶ��
 * ��ʱ��ÿ�δ�������һ��3ͨ��ɨ�裬ÿ��ͨ�����õ�һ������
 * 
 * Լ����
 *   - Nyquist:  Fs �� 2.5 �� f0
 *   - �ֱ���:   Fs / FFT_SIZE �� 0.001 �� f0   (0.1 %)
 *   - Ӳ��Լ��: ��ʱ���������ڱ��� �� ADC���3ͨ��ɨ���ʱ��
 *   - ��ȫ����: Fs �� 10kHz����ֹ��Ƶ�źŲ����쳣
 * ��Լ����ͻ������������ Nyquist ��Ӳ��Լ�����ֱ����ɲ�ֵ������
 * ----------------------------------------------------------*/
float Tim2_Config_AutoFs(float f0_hz)
{
    /* 1. ������С�����ʣ�������� */
    float Fs_min = 2.5f * f0_hz;              // �����������2.5���ź�Ƶ��
    
    /* 2. ������������ʣ�Ƶ�ʷֱ��ʣ� */
    float Fs_ideal = 0.001f * f0_hz * ; // 0.1%�ֱ��ʣ�FFT_SIZE=2048
    
    /* 3. Ӳ��Լ�� */
    float Fs_hw_max = TIM_MAX_TRIGGER_FREQ;    // Ӳ����󴥷�Ƶ��
    
    /* 4. ǿ��Ӧ�ð�ȫ���������� */
    if (Fs_min < SAMPLING_RATE_MIN) {
        Fs_min = SAMPLING_RATE_MIN;            // ��С10kHz����ֹ��Ƶ�쳣
    }

    /* 5. ѡ����ʵĲ����� */
    float Fs_target;
    
    // ���Ȳ��ԣ���Ƶʱ���ȱ�֤���������Ƶʱ��˷ֱ���
    if (f0_hz <= 20000.0f) {
        // ��Ƶ�źţ�����ƽ������ʺͷֱ���
        if (Fs_ideal >= Fs_min && Fs_ideal <= Fs_hw_max) {
            // ����������ֱ��������������
            Fs_target = Fs_ideal;
        } else {
            // �ֱ��������޷����㣬ѡ����С���ò�����
            Fs_target = (Fs_min <= Fs_hw_max) ? Fs_min : Fs_hw_max;
        }
    } else {
        // ��Ƶ�źţ�>20kHz�������ȱ�֤���������߲�����
        // ʹ�ø��ߵĲ����ʱ�������߲�������
        float Fs_high = 5.0f * f0_hz;          // ��Ƶʱʹ��5��������
        if (Fs_high > Fs_hw_max) {
            Fs_high = Fs_hw_max;               // ��Ӳ������
        }
        Fs_target = (Fs_high >= Fs_min) ? Fs_high : Fs_min;
    }
    
    /* 6. ���հ�ȫ��� */
    if (Fs_target < SAMPLING_RATE_MIN) {
        Fs_target = SAMPLING_RATE_SAFE_MIN;   // ʹ���Ƽ��İ�ȫ����20kHz
    }
    if (Fs_target > Fs_hw_max) {
        Fs_target = Fs_hw_max;                // Ӳ��Լ������
    }

    /* 6. ���ö�ʱ������Ƶ�ʣ����ڵ�ͨ�������ʣ� */
    tim2_set_fs((uint32_t)(Fs_target + 0.5f));

    /* 7. ����ʵ�ʲ����� */
    uint32_t actual_psc = __HAL_TIM_GET_PRESCALER(&htim2);
    uint32_t actual_arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    float actual_fs = (float)TIM_CLK_HZ / ((actual_psc + 1u) * (actual_arr + 1u));

    /* 8. ����ȫ�ֱ����� FFT ʹ�� */
    g_current_Fs = actual_fs;

    /* 9. �����������Ƶ��������������� */
    if (f0_hz > 25000.0f) {
        float freq_resolution = actual_fs / 2048.0f; // FFT_SIZE=2048
        printf("FS_CONFIG: f0=%.1fHz, Fs_target=%.0fHz, Fs_actual=%.0fHz, Resolution=%.1fHz\r\n",
               f0_hz, Fs_target, actual_fs, freq_resolution);
    }

    /* 10. ����ʵ�ʲ����� */
    return actual_fs;
}


/* ----------------------------------------------------------
 * ֱ�����ò����ʣ�����ʵ�ʴﵽ�Ĳ�����
 * ���룺target_fs - Ŀ������� (Hz)
 * ���أ�ʵ�ʲ����� (Hz)
 * ע�⣺������ = ��ʱ������Ƶ�ʣ�ÿ�δ���ɨ��3��ͨ��
 * ----------------------------------------------------------*/
float Tim2_Config_Channel_Fs(float target_fs)
{
    /* 1. ������������ */
    if (target_fs < SAMPLING_RATE_MIN) {
        target_fs = SAMPLING_RATE_SAFE_MIN;  // ʹ���Ƽ��İ�ȫ����20kHz
    }
    
    /* 2. ���Ӳ��Լ�� */
    if (target_fs > TIM_MAX_TRIGGER_FREQ) {
        target_fs = TIM_MAX_TRIGGER_FREQ;
    }

    /* 3. ���ö�ʱ������Ƶ�� */
    tim2_set_fs((uint32_t)(target_fs + 0.5f));

    /* 4. ����ʵ�ʲ����� */
    uint32_t actual_psc = __HAL_TIM_GET_PRESCALER(&htim2);
    uint32_t actual_arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    float actual_fs = (float)TIM_CLK_HZ / ((actual_psc + 1u) * (actual_arr + 1u));

    /* 5. ����ȫ�ֱ����� FFT ʹ�� */
    g_current_Fs = actual_fs;

    /* 6. ����ʵ�ʲ����� */
    return actual_fs;
}

/* USER CODE END 1 */
