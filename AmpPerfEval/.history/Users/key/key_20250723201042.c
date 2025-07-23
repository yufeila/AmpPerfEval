#include "key.h"

// 标志位变量定义
uint8_t basic_measurement_flag = 1;      // 基本测量标志位，初始为1
uint8_t sweep_freq_response_flag = 0;    // 扫频响应测量标志位
uint8_t measure_r_out_flag = 0;          // 输出阻抗测量标志位
uint8_t current_system_state = BASIC_MEASUREMENT_STATE;  // 当前系统状态

#define KEY_DEBOUNCE_DELAY 10 // ms

static uint8_t key0_last_state = 1; // 默认按键未按下
static uint8_t key1_last_state = 1;

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = KEY_UP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY_UP_PORT, &GPIO_InitStruct);
}


void KEY_Scan(void)
{
    uint8_t key0_state = HAL_GPIO_ReadPin(KEY0_PORT, KEY0_PIN);
    uint8_t key1_state = HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN);

    static uint32_t last_tick0 = 0;
    static uint32_t last_tick1 = 0;

    // ---- KEY0 检测与消抖 ----
    if (key0_state == GPIO_PIN_RESET && key0_last_state == GPIO_PIN_SET) // 按下
    {
        if ((HAL_GetTick() - last_tick0) > KEY_DEBOUNCE_DELAY)
        {
            last_tick0 = HAL_GetTick();

            if (current_system_state == BASIC_MEASUREMENT_STATE)
            {
                // 在基本测量模式下，KEY0用于触发输出阻抗测量
                measure_r_out_flag = 1;
            }
            else
            {
                // 在其他模式下，KEY0切换到基本测量模式
                basic_measurement_flag = 1;
                sweep_freq_response_flag = 0;
                measure_r_out_flag = 0;
                current_system_state = BASIC_MEASUREMENT_STATE;
            }
        }
    }
    key0_last_state = key0_state;

    // ---- KEY1 检测与消抗 ----
    if (key1_state == GPIO_PIN_RESET && key1_last_state == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - last_tick1) > KEY_DEBOUNCE_DELAY)
        {
            last_tick1 = HAL_GetTick();

            // KEY1: 切换到扫频响应测量模式
            sweep_freq_response_flag = 1;
            basic_measurement_flag = 0;
            measure_r_out_flag = 0;
            current_system_state = SWEEP_FREQ_RESPONSE_STATE;
        }
    }
    key1_last_state = key1_state;

    // --
}
