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
// 全局变量，用于存储当前采样率
float g_current_Fs = 1000.0f;  // 默认1kHz
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

/* --------------- 计算最优 PSC/ARR ----------------- */
static void tim2_set_fs(uint32_t Fs_target)
{
    uint32_t best_psc = 0, best_arr = 0;
    float    best_err = 1.0f;

    /* 简单搜索：先粗算 PSC，再微调 ARR */
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
            if (err < FS_ERR_MAX) break;          // 已经足够准
        }
    }

    /* 写寄存器 */
    __HAL_TIM_DISABLE(&htim2);
    __HAL_TIM_SET_PRESCALER (&htim2, best_psc);
    __HAL_TIM_SET_AUTORELOAD(&htim2, best_arr);
    __HAL_TIM_SET_COUNTER   (&htim2, 0);
    __HAL_TIM_ENABLE(&htim2);
}

/* ----------------------------------------------------------
 * 根据给定基波频率 f0_hz (Hz)  自动选择采样率 Fs，
 * 配置 TIM2  (PSC / ARR) ，并返回实际采样率 (Hz)
 * 
 * 重要：每个通道的采样率 = 定时器触发频率
 * 定时器每次触发启动一轮3通道扫描，每个通道都得到一个样本
 * 
 * 约束：
 *   - Nyquist:  Fs ≥ 2.5 × f0
 *   - 分辨率:   Fs / FFT_SIZE ≤ 0.001 × f0   (0.1 %)
 *   - 硬件约束: 定时器触发周期必须 ≥ ADC完成3通道扫描的时间
 *   - 安全下限: Fs ≥ 10kHz，防止低频信号采样异常
 * 若约束冲突，则优先满足 Nyquist 和硬件约束，分辨率由插值补偿。
 * ----------------------------------------------------------*/
float Tim2_Config_AutoFs(float f0_hz)
{
    /* 1. 约束区间 */
    float Fs_min = 2.5f * f0_hz;              // 防混叠
    float Fs_max = 0.001f * f0_hz * FFT_SIZE; // 0.1 % 分辨率
    float Fs_hw_max = TIM_MAX_TRIGGER_FREQ;    // 硬件约束：最大触发频率
    
    /* 2. 强制应用安全采样率下限 */
    if (Fs_min < SAMPLING_RATE_MIN) {
        Fs_min = SAMPLING_RATE_MIN;            // 最小10kHz，防止低频异常
    }

    /* 3. 应用硬件约束 */
    if (Fs_max > Fs_hw_max) {
        Fs_max = Fs_hw_max;
    }

    /* 4. 选择 Fs_target */
    float Fs_target;
    if (Fs_min <= Fs_max) {
        /* 取 2 的幂，且 ≥ Fs_min，同时尽量 ≤ Fs_max */
        uint32_t pow2 = 32;                   // 最低 32 Hz
        while (pow2 < Fs_min) pow2 <<= 1;
        Fs_target = (pow2 <= Fs_max) ? (float)pow2 : Fs_min;
    } else {
        Fs_target = Fs_min;                   // 只能保 Nyquist 和安全下限
        if (Fs_target > Fs_hw_max) {
            Fs_target = Fs_hw_max;            // 硬件约束优先
        }
    }
    
    /* 5. 最终安全检查：确保采样率不会过低 */
    if (Fs_target < SAMPLING_RATE_MIN) {
        Fs_target = SAMPLING_RATE_SAFE_MIN;   // 使用推荐的安全下限20kHz
    }

    /* 6. 配置定时器触发频率（等于单通道采样率） */
    tim2_set_fs((uint32_t)(Fs_target + 0.5f));

    /* 7. 计算实际采样率 */
    uint32_t actual_psc = __HAL_TIM_GET_PRESCALER(&htim2);
    uint32_t actual_arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    float actual_fs = (float)TIM_CLK_HZ / ((actual_psc + 1u) * (actual_arr + 1u));

    /* 8. 更新全局变量供 FFT 使用 */
    g_current_Fs = actual_fs;

    /* 9. 返回实际采样率 */
    return actual_fs;
}


/* ----------------------------------------------------------
 * 直接配置采样率，返回实际达到的采样率
 * 输入：target_fs - 目标采样率 (Hz)
 * 返回：实际采样率 (Hz)
 * 注意：采样率 = 定时器触发频率，每次触发扫描3个通道
 * ----------------------------------------------------------*/
float Tim2_Config_Channel_Fs(float target_fs)
{
    /* 1. 检查硬件约束 */
    if (target_fs > TIM_MAX_TRIGGER_FREQ) {
        target_fs = TIM_MAX_TRIGGER_FREQ;
    }

    /* 2. 配置定时器触发频率 */
    tim2_set_fs((uint32_t)(target_fs + 0.5f));

    /* 3. 计算实际采样率 */
    uint32_t actual_psc = __HAL_TIM_GET_PRESCALER(&htim2);
    uint32_t actual_arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    float actual_fs = (float)TIM_CLK_HZ / ((actual_psc + 1u) * (actual_arr + 1u));

    /* 4. 更新全局变量供 FFT 使用 */
    g_current_Fs = actual_fs;

    /* 5. 返回实际采样率 */
    return actual_fs;
}

/* USER CODE END 1 */
