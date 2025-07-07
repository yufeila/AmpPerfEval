/*-------------------------------------------------------------*/
/* adaptive_fft.h                                              */
/*-------------------------------------------------------------*/
#pragma once
#include "arm_math.h"          // CMSIS-DSP
#include "./spectrum_analysis/spectrum_analysis.h" // 您原来的结果结构体

/* —— 1. 根据粗判频率选抽取因子 —— */
static inline uint16_t choose_M(float f)
{
    if (f < 8000.0f)      return DECIMATE_0_8K;     // 32
    else if (f < 16000.0f) return DECIMATE_8_16K;   // 16
    else if (f < 40000.0f) return DECIMATE_16_40K;  // 8
    else if (f < 80000.0f) return DECIMATE_40_80K;  // 4
    else                   return DECIMATE_80K_200K;// 2 (或改 1)
}

/* —— 2. 每个因子配一套 FIR 低通 —— */
/* 这里只给出 M=4/8/16/32 4 组系数示范，可用 Matlab/Octave fir1 生成 */
#define MAX_TAPS  96
typedef struct {
    uint16_t M;                 // 抽取因子
    uint16_t taps;              // FIR 阶数
    const float32_t *coeff;     // 系数表
    float32_t state[MAX_TAPS+FFT_SIZE]; // state 缓冲
    arm_fir_decimate_instance_f32 inst;
} Decimator_t;
extern Decimator_t g_decim[];   // adaptive_fft.c 里定义
