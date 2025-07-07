#include "key.h"

uint8_t basic_measurement = 1;          // 初始为1
uint8_t sweep_freq_response = 0;
uint8_t measure_r_out = 0;

#define KEY_DEBOUNCE_DELAY 10 // ms

static uint8_t key0_last_state = 1; // 默认按键未按下
static uint8_t key1_last_state = 1;

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

            basic_measurement = 1;      // KEY0按下，basic_measurement 保持1
            sweep_freq_response = 0;    // 按下KEY0，sweep_freq_response 必须为0

            // 只有在basic_measurement=1时按下KEY0，measure_r_out才置1
            if(basic_measurement == 1)
                measure_r_out = 1;
        }
    }
    key0_last_state = key0_state;

    // ---- KEY1 检测与消抖 ----
    if (key1_state == GPIO_PIN_RESET && key1_last_state == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - last_tick1) > KEY_DEBOUNCE_DELAY)
        {
            last_tick1 = HAL_GetTick();

            sweep_freq_response = 1;
            basic_measurement = 0;
            measure_r_out = 0;
        }
    }
    key1_last_state = key1_state;
}
