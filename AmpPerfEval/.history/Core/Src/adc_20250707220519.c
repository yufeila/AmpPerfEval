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
#include "tim.h"
#include <string.h>  // 用于strlen函数

/* USER CODE BEGIN 0 */
// ADC DMA缓冲区 - 使用volatile确保调试器可见性
volatile uint16_t adc_buffer[BUF_SIZE] __attribute__((section(".bss")));
volatile uint8_t ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE; 
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
  // 启动ADC1的传输完成中断
  __HAL_DMA_ENABLE_IT(&hdma_adc1, DMA_IT_TC);
  // 注意：不在初始化时启动ADC+DMA，改为手动控制
  // 启动ADC1转换和DMA搬运将在主程序中手动调用
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
    // Normal模式下，DMA传输完成后自动停止
    // 主程序需要处理完数据后，重新启动ADC+DMA
  }
}

/* 添加ADC数据采集测试函数 */
void Test_ADC_Data_Collection(void)
{
    char debug_msg[200];
    
    // 串口输出开始信息
    sprintf(debug_msg, "\r\n=== ADC Data Collection Test Start ===\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
    
    // 清空缓冲区
    for(int i = 0; i < BUF_SIZE; i++) {
        adc_buffer[i] = 0;
    }
    
    sprintf(debug_msg, "Buffer cleared. BUF_SIZE = %d\r\n", BUF_SIZE);
    HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
    
    // 启动一次ADC采集
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    HAL_TIM_Base_Start(&htim2);
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE) == HAL_OK)
    {
        sprintf(debug_msg, "ADC+DMA started successfully\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
        
        // 等待采集完成
        uint32_t timeout = 0;
        while (ADC_BufferReadyFlag != BUFFER_READY_FLAG_FULL && timeout < 2000)
        {
            HAL_Delay(1);
            timeout++;
        }
        
        // 停止采集
        HAL_ADC_Stop_DMA(&hadc1);
        HAL_TIM_Base_Stop(&htim2);
        
        if(timeout >= 2000) {
            sprintf(debug_msg, "ERROR: ADC collection timeout! ADC_BufferReadyFlag=%d\r\n", ADC_BufferReadyFlag);
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            return;
        }
        
        sprintf(debug_msg, "ADC collection completed in %lu ms\r\n", timeout);
        HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
        
        // 分析数据
        if(ADC_BufferReadyFlag == BUFFER_READY_FLAG_FULL) {
            uint32_t sum_ch2 = 0, sum_ch4 = 0, sum_ch6 = 0;
            uint32_t min_ch2 = 4095, min_ch4 = 4095, min_ch6 = 4095;
            uint32_t max_ch2 = 0, max_ch4 = 0, max_ch6 = 0;
            
            // 先显示前10个原始数据
            sprintf(debug_msg, "First 10 raw ADC values:\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            for(int i = 0; i < 10 && (3*i+2) < BUF_SIZE; i++) {
                sprintf(debug_msg, "[%d] CH2=%d, CH4=%d, CH6=%d\r\n", 
                        i, adc_buffer[3*i], adc_buffer[3*i+1], adc_buffer[3*i+2]);
                HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            }
            
            // 分析前300个采样点 (100组x3通道)
            for(int i = 0; i < 100 && (3*i+2) < BUF_SIZE; i++) {
                uint16_t val_ch2 = adc_buffer[3*i];
                uint16_t val_ch4 = adc_buffer[3*i+1]; 
                uint16_t val_ch6 = adc_buffer[3*i+2];
                
                sum_ch2 += val_ch2; sum_ch4 += val_ch4; sum_ch6 += val_ch6;
                if(val_ch2 < min_ch2) min_ch2 = val_ch2; if(val_ch2 > max_ch2) max_ch2 = val_ch2;
                if(val_ch4 < min_ch4) min_ch4 = val_ch4; if(val_ch4 > max_ch4) max_ch4 = val_ch4;
                if(val_ch6 < min_ch6) min_ch6 = val_ch6; if(val_ch6 > max_ch6) max_ch6 = val_ch6;
            }
            
            // 计算平均值和范围
            uint32_t avg_ch2 = sum_ch2/100;
            uint32_t avg_ch4 = sum_ch4/100;
            uint32_t avg_ch6 = sum_ch6/100;
            uint32_t range_ch2 = max_ch2 - min_ch2;
            uint32_t range_ch4 = max_ch4 - min_ch4;
            uint32_t range_ch6 = max_ch6 - min_ch6;
            
            // 串口输出统计结果
            sprintf(debug_msg, "\r\n=== ADC Statistics (100 samples) ===\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            sprintf(debug_msg, "CH2 (Input):  Min=%lu, Max=%lu, Avg=%lu, Range=%lu\r\n", 
                    min_ch2, max_ch2, avg_ch2, range_ch2);
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            sprintf(debug_msg, "CH4 (AC Out): Min=%lu, Max=%lu, Avg=%lu, Range=%lu\r\n", 
                    min_ch4, max_ch4, avg_ch4, range_ch4);
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            sprintf(debug_msg, "CH6 (DC Out): Min=%lu, Max=%lu, Avg=%lu, Range=%lu\r\n", 
                    min_ch6, max_ch6, avg_ch6, range_ch6);
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            // 电压转换显示
            float vol_ch2 = avg_ch2 * 0.0008058f;
            float vol_ch4 = avg_ch4 * 0.0008058f;
            float vol_ch6 = avg_ch6 * 0.0008058f;
            
            sprintf(debug_msg, "\r\n=== Voltage Conversion ===\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            sprintf(debug_msg, "CH2 (Input):  %.3fV\r\n", vol_ch2);
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            sprintf(debug_msg, "CH4 (AC Out): %.3fV\r\n", vol_ch4);
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            sprintf(debug_msg, "CH6 (DC Out): %.3fV\r\n", vol_ch6);
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
            
            sprintf(debug_msg, "=== Test Completed ===\r\n\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
        }
    }
    else 
    {
        sprintf(debug_msg, "ERROR: Failed to start ADC+DMA\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
    }
}
