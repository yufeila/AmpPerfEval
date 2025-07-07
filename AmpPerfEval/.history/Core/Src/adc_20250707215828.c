/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
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
#include "adc.h"

/* USER CODE BEGIN 0 */
// ADC DMA������ - ʹ��volatileȷ���������ɼ���
volatile uint16_t adc_buffer[BUF_SIZE] __attribute__((section(".bss")));
volatile uint8_t ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE; 

// ���Ա��� - ��Keil Watch���ڲ鿴
volatile uint32_t debug_ch2_avg = 0, debug_ch4_avg = 0, debug_ch6_avg = 0;
volatile uint32_t debug_ch2_range = 0, debug_ch4_range = 0, debug_ch6_range = 0;
volatile uint32_t debug_buffer_ready_count = 0;
/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* ADC1 init function */
void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};
  ADC_InjectionConfTypeDef sConfigInjected = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configures for the selected ADC injected channel its corresponding rank in the sequencer and its sample time
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_2;
  sConfigInjected.InjectedRank = 1;
  sConfigInjected.InjectedNbrOfConversion = 3;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_3CYCLES;
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_NONE;
  sConfigInjected.ExternalTrigInjecConv = ADC_INJECTED_SOFTWARE_START;
  sConfigInjected.AutoInjectedConv = DISABLE;
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.InjectedOffset = 0;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configures for the selected ADC injected channel its corresponding rank in the sequencer and its sample time
  */
  sConfigInjected.InjectedRank = 2;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configures for the selected ADC injected channel its corresponding rank in the sequencer and its sample time
  */
  sConfigInjected.InjectedRank = 3;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */
  // ����ADC1�Ĵ�������ж�
  __HAL_DMA_ENABLE_IT(&hdma_adc1, DMA_IT_TC);
  // ע�⣺���ڳ�ʼ��ʱ����ADC+DMA����Ϊ�ֶ�����
  // ����ADC1ת����DMA���˽������������ֶ�����
  /* USER CODE END ADC1_Init 2 */

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PA2     ------> ADC1_IN2
    PA4     ------> ADC1_IN4
    PA6     ------> ADC1_IN6
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_NORMAL;
    hdma_adc1.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(adcHandle,DMA_Handle,hdma_adc1);

  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PA2     ------> ADC1_IN2
    PA4     ------> ADC1_IN4
    PA6     ------> ADC1_IN6
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_6);

    /* ADC1 DMA DeInit */
    HAL_DMA_DeInit(adcHandle->DMA_Handle);
  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* ADC DMA transfer complete callback */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  if(hadc->Instance == ADC1)
  {
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_FULL;
    debug_buffer_ready_count++;  // ���ԣ�ͳ��DMA��ɴ���
    // Normalģʽ�£�DMA������ɺ��Զ�ֹͣ
    // ��������Ҫ���������ݺ���������ADC+DMA
  }
}

/* ���ADC���ݲɼ����Ժ��� */
void Test_ADC_Data_Collection(void)
{
    // ǿ��������Ա���
    debug_ch2_avg = debug_ch4_avg = debug_ch6_avg = 0;
    debug_ch2_range = debug_ch4_range = debug_ch6_range = 0;
    debug_buffer_ready_count = 0;
    
    // ��ջ�����
    for(int i = 0; i < BUF_SIZE; i++) {
        adc_buffer[i] = 0;
    }
    
    // ����һ��ADC�ɼ�
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    HAL_TIM_Base_Start(&htim2);
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE) == HAL_OK)
    {
        // �ȴ��ɼ����
        uint32_t timeout = 0;
        while (ADC_BufferReadyFlag != BUFFER_READY_FLAG_FULL && timeout < 2000)
        {
            HAL_Delay(1);
            timeout++;
        }
        
        // ֹͣ�ɼ�
        HAL_ADC_Stop_DMA(&hadc1);
        HAL_TIM_Base_Stop(&htim2);
        
        // ��������
        if(ADC_BufferReadyFlag == BUFFER_READY_FLAG_FULL) {
            uint32_t sum_ch2 = 0, sum_ch4 = 0, sum_ch6 = 0;
            uint32_t min_ch2 = 4095, min_ch4 = 4095, min_ch6 = 4095;
            uint32_t max_ch2 = 0, max_ch4 = 0, max_ch6 = 0;
            
            // ����ǰ300�������� (100��x3ͨ��)
            for(int i = 0; i < 100 && (3*i+2) < BUF_SIZE; i++) {
                uint16_t val_ch2 = adc_buffer[3*i];
                uint16_t val_ch4 = adc_buffer[3*i+1]; 
                uint16_t val_ch6 = adc_buffer[3*i+2];
                
                sum_ch2 += val_ch2; sum_ch4 += val_ch4; sum_ch6 += val_ch6;
                if(val_ch2 < min_ch2) min_ch2 = val_ch2; if(val_ch2 > max_ch2) max_ch2 = val_ch2;
                if(val_ch4 < min_ch4) min_ch4 = val_ch4; if(val_ch4 > max_ch4) max_ch4 = val_ch4;
                if(val_ch6 < min_ch6) min_ch6 = val_ch6; if(val_ch6 > max_ch6) max_ch6 = val_ch6;
            }
            
            // ���µ��Ա���
            debug_ch2_avg = sum_ch2/100; debug_ch4_avg = sum_ch4/100; debug_ch6_avg = sum_ch6/100;
            debug_ch2_range = max_ch2 - min_ch2; debug_ch4_range = max_ch4 - min_ch4; debug_ch6_range = max_ch6 - min_ch6;
        }
    }
}
