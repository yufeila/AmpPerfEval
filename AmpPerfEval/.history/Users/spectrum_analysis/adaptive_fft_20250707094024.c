/*-------------------------------------------------------------*/
/* adaptive_fft.c                                              */
/*-------------------------------------------------------------*/
#include "adaptive_fft.h"

/* ===== 低通系数（示例） ===== */
static const float32_t fir4_48[]  = { /* M=4 48 taps … */ };
static const float32_t fir8_64[]  = { /* M=8 64 taps … */ };
static const float32_t fir16_80[] = { /* M=16 80 taps … */ };
static const float32_t fir32_96[] = { /* M=32 96 taps … */ };

Decimator_t g_decim[] = {
    {4 , 48,  fir4_48 , {0}},
    {8 , 64,  fir8_64 , {0}},
    {16, 80,  fir16_80, {0}},
    {32, 96,  fir32_96, {0}},
};

/* ---------- 帮手：按 M 找 decimator ---------- */
static Decimator_t* get_dec(uint16_t M)
{
    for (uint32_t i = 0; i < sizeof(g_decim)/sizeof(g_decim[0]); ++i)
        if (g_decim[i].M == M) return &g_decim[i];
    return NULL;    // M==1 时返回 NULL
}

/* ---------- 新的“自适应”光谱分析 ---------- */
static void ProcessSampleData_Adaptive_F32(float *in, SpectrumResult_t *out)
{
    /* 1. 粗判 ───────────────────────────── */
    static SpectrumResult_t coarse;      // 保存上一次粗判
    spectrum_analysis(in, FFT_SIZE, F_s, &coarse);

    /* 2. 选抽取因子 ─────────────────────── */
    uint16_t M = choose_M(coarse.frequency);
    if (M == 1) {         // 高频段直接返回粗判即可
        *out = coarse;
        return;
    }

    /* 3. 抽取（低通 + down-sample）────────── */
    Decimator_t *dec = get_dec(M);
    if (!dec->inst.numTaps) {            // 首次 init
        arm_fir_decimate_init_f32(&dec->inst,
                                  dec->taps,   // numTaps
                                  dec->M,
                                  dec->coeff,
                                  dec->state,
                                  FFT_SIZE);   // block size
    }

    /* 3.1 把 1024 点原始流 → 抽取后 out_len = 1024/M */
    float32_t tmp_out[FFT_SIZE / 2];     // M≥2，足够了
    arm_fir_decimate_f32(&dec->inst, in, tmp_out, FFT_SIZE);

    /* 3.2 累积到一个 1024-点窗 *********************************************************
     *   ┌─ in 1024 ─┐┌─ in 1024 ─┐...   每帧只能输出 1024/M 点
     *   └─ out 512 ─┘└─ out 512 ─┘...   例如 M=2 -> 512 点/帧，需要 2 帧才能拼 1024
     * **************************************************************************/
    static float32_t dec_buf[FFT_SIZE];
    static uint16_t  wr_ptr = 0;

    uint16_t out_len = FFT_SIZE / M;
    memcpy(&dec_buf[wr_ptr], tmp_out, out_len * sizeof(float32_t));
    wr_ptr += out_len;

    if (wr_ptr < FFT_SIZE) {
        /* 还没攒够整窗 → 先返回粗判，省 CPU */
        *out = coarse;
        return;
    }
    wr_ptr = 0;  // 清零继续积下一窗

    /* 4. 细判 FFT ────────────────────────── */
    spectrum_analysis(dec_buf, FFT_SIZE, F_s / M, out);
}


/* ---------------------------------------------------------- */
/* ProcessSampleData_ByHint_F32                               */
/*  - 已知目标频率，不做粗判                                   */
/* ---------------------------------------------------------- */
static void ProcessSampleData_ByHint_F32(float *in, SpectrumResult_t *out)
{
    uint16_t M = g_target_M;          // 直接用外部给出的 M
    if (M == 1) {                     // 高频段：直接做一次 1024-pt FFT
        spectrum_analysis(in, FFT_SIZE, F_s, out);
        return;
    }

    /* --- 低通 + 抽取 ------------------------------------ */
    Decimator_t *dec = get_dec(M);
    //初始化一个浮点 FIR 抽取器实例（写好 numTaps、M、pCoeffs、pState 等元数据）
    if (!dec->inst.numTaps) {         // 首次 init
        arm_fir_decimate_init_f32(&dec->inst,
                                  dec->taps,
                                  dec->M,
                                  dec->coeff,
                                  dec->state,
                                  FFT_SIZE);
    }

    float32_t tmp_out[FFT_SIZE / 2];  // M≥2 → 512 max
    arm_fir_decimate_f32(&dec->inst, in, tmp_out, FFT_SIZE);

    /* --- 累积 M 帧拼到 1024 点 -------------------------- */
    static float32_t dec_buf[FFT_SIZE];
    static uint16_t  wr_ptr = 0;

    uint16_t out_len = FFT_SIZE / M;
    memcpy(&dec_buf[wr_ptr], tmp_out, out_len * sizeof(float32_t));
    wr_ptr += out_len;

    if (wr_ptr < FFT_SIZE)            // 还没攒满
        return;

    wr_ptr = 0;                       // 清指针

    /* --- 细判：补偿 DDS / 滤波器的微小误差 --------------- */
    spectrum_analysis(dec_buf, FFT_SIZE, F_s / M, out);
}
