#ifndef __KEY_H
#define __KEY_H

#include "stm32f4xx_hal.h"

#define KEY0_PIN GPIO_PIN_8
#define KEY0_PORT GPIOB

#define KEY1_PIN GPIO_PIN_9
#define KEY1_PORT GPIOB

#define RELAY_PIN GPIO_PIN_12
#define RELAY_PORT GPIOB

#define RELAY_ON HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_SET)

#define BASIC_MEASUREMENT_FLAG      1
#define SWEEP_FREQ_RESPONSE         2

#endif /* key.h */
