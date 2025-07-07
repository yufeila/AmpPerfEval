/*-------------------------------------------------------------*/
/* adaptive_fft.h                                              */
/*-------------------------------------------------------------*/
#pragma once
#include "arm_math.h"          // CMSIS-DSP
#include "./spectrum_analysis/spectrum_analysis.h" // ��ԭ���Ľ���ṹ��

/* ���� 1. ���ݴ���Ƶ��ѡ��ȡ���� ���� */
static inline uint16_t choose_M(float f)
{
    if (f < 8000.0f)      return DECIMATE_0_8K;     // 32
    else if (f < 16000.0f) return DECIMATE_8_16K;   // 16
    else if (f < 40000.0f) return DECIMATE_16_40K;  // 8
    else if (f < 80000.0f) return DECIMATE_40_80K;  // 4
    else                   return DECIMATE_80K_200K;// 2 (��� 1)
}

/* ���� 2. ÿ��������һ�� FIR ��ͨ ���� */
/* ����ֻ���� M=4/8/16/32 4 ��ϵ��ʾ�������� Matlab/Octave fir1 ���� */
#define MAX_TAPS  96
typedef struct {
    uint16_t M;                 // ��ȡ����
    uint16_t taps;              // FIR ����
    const float32_t *coeff;     // ϵ����
    float32_t state[MAX_TAPS+FFT_SIZE]; // state ����
    arm_fir_decimate_instance_f32 inst;
} Decimator_t;
extern Decimator_t g_decim[];   // adaptive_fft.c �ﶨ��
