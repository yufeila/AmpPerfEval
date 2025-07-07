#ifndef SPECTRUM_ANALYSIS_H
#define SPECTRUM_ANALYSIS_H

#include <stdint.h>

#define M_PI 3.14159265358979323846f

#define USE_TIME_DOMAIN_VPP   0    /* 1: 时域幅值, 0: FFT 幅值 */

#define AMP_CAL_SCALE     1.0f      
#define AMP_CAL_OFFSET    0.0f   

typedef struct {
    float amplitude;    // 估计幅值
    float frequency;    // 估计频率(Hz)
    uint16_t bin_index; // 主峰bin
    float delta;        // 插值偏移量
} SpectrumResult_t;

// 主分析接口
void spectrum_analysis(const float *x, uint16_t N, float fs, SpectrumResult_t *result);

#endif // SPECTRUM_ANALYSIS_H
