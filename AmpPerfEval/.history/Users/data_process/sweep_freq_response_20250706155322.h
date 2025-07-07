/*
 * @Author: 杨宇菲 17786321727@163.com
 * @Date: 2025-07-03 18:09:36
 * @LastEditors: 杨宇菲 17786321727@163.com
 * @LastEditTime: 2025-07-05 19:18:28
 * @FilePath: \AmpPerfEval\Users\sweep_freq_response\sweep_freq_response.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __SWEEP_FREQ_RESPONSE_H
#define __SWEEP_FREQ_RESPONSE_H

#include "main.h"
#include <math.h>  
#include "./ad9851/bsp_ad9851.h"
#include "./lcd/lcd.h"
#include "adc.h"

#define FREQ_START      100.0f   // 起始扫描频率 100Hz
#define FREQ_STOP       200000.0f   // 终止扫描频率 200 kHz
#define FREQ_POINTS     50         
#define SETTLE_TIME_MS  100       
#define ARROW_SIZE      8          


typedef struct {
    float frequency;   
    float input_amp;    
    float output_amp;   
    float gain_db;       
    float phase_deg;     
} FreqResponse_t;


void Display_Spectrum_Results(void);
void Auto_Frequency_Response_Measurement(void);


#endif /*  sweep_freq_response.h  */
