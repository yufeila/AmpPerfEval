/*
 * @Author: yyf 17786321727@163.com
 * @Date: 2025-07-07 13:03:52
 * @LastEditors: yyf 17786321727@163.com
 * @LastEditTime: 2025-07-07 15:05:58
 * @FilePath: /AmpPerfEval/Users/key/key.h
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
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

// ϵͳ״̬����
#define BASIC_MEASUREMENT_STATE           1
#define SWEEP_FREQ_RESPONSE_STATE         2

// �ⲿ��������
extern uint8_t basic_measurement_flag;      // ����������־λ
extern uint8_t sweep_freq_response_flag;    // ɨƵ��Ӧ������־λ
extern uint8_t measure_r_out_flag;          // ����迹������־λ
extern uint8_t current_system_state;        // ��ǰϵͳ״̬

// ��������
void KEY_Scan(void);

#endif /* key.h */
