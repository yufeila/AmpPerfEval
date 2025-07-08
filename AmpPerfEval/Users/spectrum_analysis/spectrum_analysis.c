#include "./spectrum_analysis/spectrum_analysis.h"
#include "arm_math.h"
//#include "arm_const_structs.h"
#include <math.h>

/* ==============================================================
 *  用户可调常量
 * ============================================================*/
#define MAX_FFT_N   2048u        /* 允许的最大 FFT 点数 (必须 ≤ N) */

/* ==============================================================
 *  静态工作缓冲区 —— 放在 .bss，绝不占用栈
 * ============================================================*/
static float g_fft_workbuf[MAX_FFT_N];          /* RFFT 输出/临时区   N  个 float */
static float g_mag_buf [MAX_FFT_N/2 + 1];       /* 单边幅度谱         N/2+1 个 */


// ========== 工具函数 ==========

/* 归一化 sinc(x) = sin(pi x)/(pi x) */
static __inline float sinc_norm(float x)
{
    if (fabsf(x) < 1e-6f) return 1.0f;
    return sinf(M_PI * x) / (M_PI * x);
}

/* 由最大幅值与邻点幅值求 |δ| = r/(1+r)，0≤|δ|≤1 */
static __inline float interp_delta(float mag_max, float mag_neigh)
{
    if (mag_neigh <= 0.0f || mag_max <= 0.0f) return 0.0f;
    float r = mag_neigh / mag_max;
    return r / (1.0f + r);
}

/**
 * @brief  计算单边幅度谱 |X[k]|，结果存入 mag[]
 * @param  x        实数输入序列，长度 N
 * @param  N        FFT 点数 (2 的幂)
 * @param  workbuf  长度 N 的工作区 (RFFT 输出)
 * @param  mag      输出幅度谱，长度 N/2+1
 */
static void calc_magnitude(const float *x, uint16_t N,
                           float *workbuf,
                           float *mag)
{
    /* 1) 实数 FFT */
    arm_rfft_fast_instance_f32 S;
    arm_rfft_fast_init_f32(&S, N);
    arm_rfft_fast_f32(&S, (float *)x, workbuf, 0);      /* 正向变换 */

    /* 2) 单边幅度谱 |X[k]| */
    mag[0] = fabsf(workbuf[0]);                         /* DC */
    for (uint16_t k = 1; k < N/2; ++k)
    {
        float re = workbuf[2u * k];
        float im = workbuf[2u * k + 1u];
        mag[k] = sqrtf(re * re + im * im);
    }
    mag[N/2] = fabsf(workbuf[1]);                       /* Nyquist */
}

/* ==============================================================
 *  主接口：高精度幅频测量（3 点 sinc 插值）
 * ============================================================*/
/**
 * @brief 对给定信号进行频谱分析，并计算主峰的频率和幅值。
 * 
 * @param x       指向输入信号数组的指针（float类型）。
 * @param N       输入信号的样本数量。必须为2的幂，且不超过MAX_FFT_N。
 * @param fs      输入信号的采样频率（单位：Hz）。
 * @param result  指向SpectrumResult_t结构的指针，用于存储分析结果。
 * 
 * @details
 * 此函数执行以下步骤：
 * 1. 计算输入信号的幅度谱。
 * 2. 识别主频率峰值（排除直流分量）。
 * 3. 选择邻近峰值中幅度较大的一个进行插值。
 * 4. 使用基于sinc的插值方法优化峰值位置。
 * 5. 计算主峰的校正频率和幅值。
 * 
 * 幅值根据CMSIS RFFT定义进行校正，结果经过缩放和偏移以匹配所需单位（峰值电压）。
 * 
 * @note
 * - 输入信号长度（N）必须为2的幂。
 * - 函数使用全局缓冲区`g_fft_workbuf`和`g_mag_buf`进行中间计算。
 * - 如果输入参数无效，函数将直接返回，不执行任何分析。
 * 
 * @return 无返回值。分析结果存储在`result`结构中。
 */
void spectrum_analysis(const float *x, uint16_t N, float fs, SpectrumResult_t *result)
{
    /* -------- 参数检查 -------- */
    if (!x || !result || N == 0 || (N & (N - 1))) {
        // 参数错误，返回0值
        result->amplitude = 0.0f;
        result->frequency = 0.0f;
        result->bin_index = 0;
        result->delta = 0.0f;
        return;
    }
    if (N > MAX_FFT_N) {
        // 超出FFT最大长度，返回0值
        result->amplitude = 0.0f;
        result->frequency = 0.0f;
        result->bin_index = 0;
        result->delta = 0.0f;
        return;
    }

    /* ---------- (a) 统计时域极值 ---------- */
    float vmax = x[0], vmin = x[0];
    for(uint16_t i = 1; i < N; ++i) {
        if(x[i] > vmax) vmax = x[i];
        if(x[i] < vmin) vmin = x[i];
    }
    float vpk_time = 0.5f * (vmax - vmin);   /* 直接得到 Vpk */

    /* ---------- (b) FFT 求主频 ---------- */
    float *work = g_fft_workbuf;
    float *mag  = g_mag_buf;

    /* 1) 幅度谱 */
    calc_magnitude(x, N, work, mag);

    /* 2) 找主峰(跳过 DC) */
    uint16_t k_max   = 1;
    float    max_val = mag[1];
    for (uint16_t k = 2; k < N / 2; ++k)
    {
        if (mag[k] > max_val) { max_val = mag[k]; k_max = k; }
    }

    /* 3) 选左 / 右邻幅值大的做插值 */
    uint16_t k_left  = (k_max == 1)       ? k_max     : k_max - 1;
    uint16_t k_right = (k_max >= N/2 - 1) ? k_max     : k_max + 1;
    uint16_t k_neigh = (mag[k_right] > mag[k_left]) ? k_right : k_left;

    /* 4) 三点 sinc-插值求 δ (-1..1) */
    float delta = interp_delta(mag[k_max], mag[k_neigh]);
    delta *= (k_neigh > k_max) ? 1.0f : -1.0f;

    /* 5) 频率 & 幅值校正 */
    float k_true = (float)k_max + delta;                /* 真正 bin 位置 */
    float freq   = k_true * fs / (float)N;              /* 频率估计 (Hz)  */

    /* ---------- (c) 幅度选路 ---------- */
#if USE_TIME_DOMAIN_VPP
    float amp_pk = vpk_time;                  /* 时域半幅 */
#else
    float amp_pk = (mag[k_max] / fabsf(sinc_norm(delta))) * (2.0f / (float)N);
#endif

    /* ---------- (d) 标定 & 写回 ---------- */
    result->amplitude = AMP_CAL_SCALE * amp_pk + AMP_CAL_OFFSET;     /* 单位 = Volt_peak (Vpk) */
    result->frequency = freq;
    result->bin_index = k_max;
    result->delta     = delta;
   
}

