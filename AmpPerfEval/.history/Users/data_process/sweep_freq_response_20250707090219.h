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
#include './spectrum_analysis/spectrum_analysis.h'
#include "./lcd/lcd.h"
#include "./key/key.h"
#include "adc.h"

#define FREQ_START      100.0f   // 起始扫描频率 100Hz
#define FREQ_STOP       200000.0f   // 终止扫描频率 200 kHz
#define FREQ_POINTS     50         
#define SETTLE_TIME_MS  100       
#define ARROW_SIZE      8          

// 频率相关设定
#define ADC_TOTAL_FS       1354838.7f   // ADC 触发频率 (全部通道)
#define ADC_NUM_CHANNELS   3
#define F_s   (ADC_TOTAL_FS / ADC_NUM_CHANNELS)

/* 采样频率调整因子 */
#define DECIMATE_0_8K    32  // 采样频率调整因子，8KHz
#define DECIMATE_8_16K   16
#define DECIMATE_16_40K   8
#define DECIMATE_40_80K   4
#define DECIMATE_80K_200K 2

// LCD显示相关宏定义
#define LCD_X_LENGTH    240
#define LCD_Y_LENGTH    320
#define LINE(x)         ((x) * 16)  // 每行16像素高度

// 数据处理(更新快慢)相关宏定义
#define PROCESS_INTERVAL 3
#define PROCESS_ARRAY_SIZE_WITH_R_L 8

// 颜色相关宏定义
#define TITLE_COLOR             WHITE
#define SUB_TITLE_COLOR         BLUE
#define DATA_COLOR              WHITE

// 电路相关参数定义
#define V_s                     20.0f //单位：mV
#define V_Rs_Gain               106.0f
#define R_s                     0.022f //单位：kΩ
#define R_L                     570.0f   //单位：kΩ     

typedef struct {
    SpectrumResult_t adc_in_Result;
    SpectrumResult_t adc_ac_out_Result;
    float adc_dc_out_Result;     
} SignalAnalysisResult_t;

// 幅频特性曲线点结构体
typedef struct {
    float frequency;   
    float input_amp;    
    float output_amp;   
    float gain_db;       
    float phase_deg;     
} FreqResponse_t;


void Basic_Measurement(void);
void Auto_Frequency_Response_Measurement(void);


#endif /*  sweep_freq_response.h  */
