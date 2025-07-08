/*
 * @Author: yyf 17786321727@163.com
 * @Date: 2025-07-07 13:03:52
 * @LastEditors: yyf 17786321727@163.com
 * @LastEditTime: 2025-07-08 09:24:52
 * @FilePath: /AmpPerfEval/Core/Inc/main.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void Test_ADC_Data_Collection(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RELAY_Pin GPIO_PIN_12
#define RELAY_GPIO_Port GPIOB
#define LCD_BK_Pin GPIO_PIN_15
#define LCD_BK_GPIO_Port GPIOB
#define AD9851_RESET_Pin GPIO_PIN_4
#define AD9851_RESET_GPIO_Port GPIOB
#define AD9851_BIT_DATA_Pin GPIO_PIN_5
#define AD9851_BIT_DATA_GPIO_Port GPIOB
#define AD9851_W_CLK_Pin GPIO_PIN_6
#define AD9851_W_CLK_GPIO_Port GPIOB
#define AD9851_FQ_UP_Pin GPIO_PIN_7
#define AD9851_FQ_UP_GPIO_Port GPIOB
#define KEY0_Pin GPIO_PIN_8
#define KEY0_GPIO_Port GPIOB
#define KEY1_Pin GPIO_PIN_9
#define KEY1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
// LCD FSMC地址定义
#define LCD_BASE        ((uint32_t)(0x6C000000 | 0x0000007E))
#define LCD_CMD         (*((volatile uint16_t *)(LCD_BASE)))
#define LCD_DATA        (*((volatile uint16_t *)(LCD_BASE + 2)))

#define FFT_SIZE        1024U  // 改为1024点FFT

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
