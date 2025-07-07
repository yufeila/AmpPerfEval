/*
 * @Author: 杨宇菲 17786321727@163.com
 * @Date: 2025-07-06 12:06:21
 * @LastEditors: 杨宇菲 17786321727@163.com
 * @LastEditTime: 2025-07-06 14:50:47
 * @FilePath: \AmpPerfEval\Core\Inc\main.h
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

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LCD_BK_Pin GPIO_PIN_15
#define LCD_BK_GPIO_Port GPIOB
#define AD9851_W_CLK_Pin GPIO_PIN_6
#define AD9851_W_CLK_GPIO_Port GPIOB
#define KEY0_Pin GPIO_PIN_8
#define KEY0_GPIO_Port GPIOB
#define KEY0_EXTI_IRQn EXTI9_5_IRQn
#define KEY1_Pin GPIO_PIN_9
#define KEY1_GPIO_Port GPIOB
#define KEY1_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */
// LCD FSMC地址定义
#define LCD_BASE        ((uint32_t)(0x6C000000 | 0x0000007E))
#define LCD_CMD         (*((volatile uint16_t *)(LCD_BASE)))
#define LCD_DATA        (*((volatile uint16_t *)(LCD_BASE + 2)))
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
