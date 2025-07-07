/*-------------------------------------------------------------*/
/* adaptive_fft.c                                              */
/*-------------------------------------------------------------*/
#include "./spectrum_analysis/adaptive_fft.h"
#include "./spectrum_analysis/fir.h"
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
/**
 * @brief   自适应抽取 + 细分辨率 FFT 处理（已知激励频率）
 *
 * 在采样端已经拿到 #FFT_SIZE (=1024) 个浮点样本的前提下，
 * 按当前 sweep 点的名义频率（由全局 g_target_M 给出）决定是否抽取：
 *   ? M == 1 : 高频段——直接 1024-pt FFT；
 *   ? M > 1  : 先做 FIR 低通 + 抽取 M 倍 → 积满 1024 点再做细判 FFT。
 *
 * 流程：
 *   1. 通过 choose_M() 预先选好的抽取因子 g_target_M；
 *   2. 首次调用时用 arm_fir_decimate_init_f32() 懒加载 FIR 实例；
 *   3. arm_fir_decimate_f32()   → 低通 + 降采样（blockSize=1024）；
 *   4. 将输出数据累计到 dec_buf[1024]，攒够整窗后
 *      spectrum_analysis(dec_buf, 1024, F_s/M, out) 得到精细频幅结果。
 *
 * @param[in]  in   指向 1024 个已换算成伏特的单通道样本（float32_t）。
 * @param[out] out  指向调用方提供的 #SpectrumResult_t 结构体，用于返回
 *                  精细频率、幅度、相位等结果。
 *
 * @note  调用者需要在 DMA 半/全缓冲回调中：
 *        1. DemuxADCData() 将 interleave 的三通道数据拆成各自 buffer；
 *        2. 调用 Set_DDS_Frequency() 更新 g_target_M；
 *        3. 把对应通道的 buffer 指针传进本函数。
 *
 * @warning  本函数内部使用两个 static 缓冲：
 *           - tmp_out[512]  ≈ 2 kB
 *           - dec_buf[1024] ≈ 4 kB
 *           若多个并发通道同时调用，请分别拷贝此函数或加临界区保护。
 */
static void ProcessSampleData_ByHint_F32(float *in, SpectrumResult_t *out, uint16_t g_target_M)
{
    uint16_t M = g_target_M;          //  抽取因子M
    if (M == 1) {                     // 高频段：直接做一次 1024-pt FFT
        spectrum_analysis(in, FFT_SIZE, F_s, out);
        return;
    }

    /* --- 低通 + 抽取 ------------------------------------ */
    Decimator_t *dec = get_dec(M);
    // 初始化一个浮点 FIR 抽取器实例（写好 numTaps、M、pCoeffs、pState 等元数据
    if (!dec->inst.numTaps) {         // 首次 init
        arm_fir_decimate_init_f32(&dec->inst,
                                  dec->taps,
                                  dec->M,
                                  dec->coeff,
                                  dec->state,
                                  FFT_SIZE);
    }

    // 运行一次 (FIR 低通 + 丢样) 的多相抽取，处理 blockSize 个输入样本，输出 blockSize / M 个样本
    float32_t tmp_out[FFT_SIZE / 2];  // M≥2 → 512 max , 最大覆盖点
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
