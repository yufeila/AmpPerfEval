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
 * 约束：
 *   - Nyquist:  Fs ≥ 2.5 × f0
 *   - 分辨率:   Fs / FFT_SIZE ≤ 0.001 × f0   (0.1 %)
 * 若两约束冲突，则优先满足 Nyquist，分辨率由 3-点 sinc 插值补偿。
 * ----------------------------------------------------------*/
float Tim2_Config_AutoFs(float f0_hz)
{
    /* 1. 约束区间 */
    float Fs_min = 2.5f * f0_hz;              // 防混叠
    float Fs_max = 0.001f * f0_hz * FFT_SIZE; // 0.1 % 分辨率

    /* 2. 选择 Fs_target */
    float Fs_target;
    if (Fs_min <= Fs_max) {
        /* 取 2 的幂，且 ≥ Fs_min，同时时尽量 ≤ Fs_max */
        uint32_t pow2 = 32;                   // 最低 32 Hz
        while (pow2 < Fs_min) pow2 <<= 1;
        Fs_target = (pow2 <= Fs_max) ? (float)pow2 : Fs_min;
    } else {
        Fs_target = Fs_min;                   // 只能保 Nyquist
    }

    /* 3. 计算并写 PSC / ARR */
    tim2_set_fs((uint32_t)(Fs_target + 0.5f));

    /* 4. 更新全局变量供 FFT 使用 */
    g_current_Fs = Fs_target;

    /* 5. 把实际采样率返回给调用者 */
    return Fs_target;
}


/* USER CODE END 1 */
