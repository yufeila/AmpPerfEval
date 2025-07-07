#include "key.h"

#define KEY_DEBOUNCE_DELAY 10 // ms

// 静态变量保存当前状态
static uint16_t key0_flag = 0; // 0: BASIC, 1: MEASURE
static uint16_t current_flag = BASIC_MEASUREMENT_FLAG;
static uint8_t key0_last_state = 1; // 默认按键未按下
static uint8_t key1_last_state = 1;

uint16_t KEY_Scan(void)
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

            key0_flag = !key0_flag; // 交替翻转
            if (key0_flag)
                current_flag = MEASURE_R_OUT_FLAG;
            else
                current_flag = BASIC_MEASUREMENT_FLAG;
        }
    }
    key0_last_state = key0_state;

    // ---- KEY1 检测与消抖 ----
    if (key1_state == GPIO_PIN_RESET && key1_last_state == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - last_tick1) > KEY_DEBOUNCE_DELAY)
        {
            last_tick1 = HAL_GetTick();
            current_flag = SWEEP_FREQ_RESPONSE_FLAG;
        }
    }
    key1_last_state = key1_state;

    return current_flag;
}
