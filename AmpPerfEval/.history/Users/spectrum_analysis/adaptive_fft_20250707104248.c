/*-------------------------------------------------------------*/
/* adaptive_fft.c                                              */
/*-------------------------------------------------------------*/
#include "./spectrum_analysis/adaptive_fft.h"
#include "./spectrum_analysis/fir.h"
/* ===== ��ͨϵ����ʾ���� ===== */
static const float32_t fir4_48[]  = { /* M=4 48 taps �� */ };
static const float32_t fir8_64[]  = { /* M=8 64 taps �� */ };
static const float32_t fir16_80[] = { /* M=16 80 taps �� */ };
static const float32_t fir32_96[] = { /* M=32 96 taps �� */ };

Decimator_t g_decim[] = {
    {4 , 48,  fir4_48 , {0}},
    {8 , 64,  fir8_64 , {0}},
    {16, 80,  fir16_80, {0}},
    {32, 96,  fir32_96, {0}},
};

/* ---------- ���֣��� M �� decimator ---------- */
static Decimator_t* get_dec(uint16_t M)
{
    for (uint32_t i = 0; i < sizeof(g_decim)/sizeof(g_decim[0]); ++i)
        if (g_decim[i].M == M) return &g_decim[i];
    return NULL;    // M==1 ʱ���� NULL
}

/* ---------- �µġ�����Ӧ�����׷��� ---------- */
static void ProcessSampleData_Adaptive_F32(float *in, SpectrumResult_t *out)
{
    /* 1. ���� ���������������������������������������������������������� */
    static SpectrumResult_t coarse;      // ������һ�δ���
    spectrum_analysis(in, FFT_SIZE, F_s, &coarse);

    /* 2. ѡ��ȡ���� ���������������������������������������������� */
    uint16_t M = choose_M(coarse.frequency);
    if (M == 1) {         // ��Ƶ��ֱ�ӷ��ش��м���
        *out = coarse;
        return;
    }

    /* 3. ��ȡ����ͨ + down-sample���������������������� */
    Decimator_t *dec = get_dec(M);
    if (!dec->inst.numTaps) {            // �״� init
        arm_fir_decimate_init_f32(&dec->inst,
                                  dec->taps,   // numTaps
                                  dec->M,
                                  dec->coeff,
                                  dec->state,
                                  FFT_SIZE);   // block size
    }

    /* 3.1 �� 1024 ��ԭʼ�� �� ��ȡ�� out_len = 1024/M */
    float32_t tmp_out[FFT_SIZE / 2];     // M��2���㹻��
    arm_fir_decimate_f32(&dec->inst, in, tmp_out, FFT_SIZE);

    /* 3.2 �ۻ���һ�� 1024-�㴰 *********************************************************
     *   ���� in 1024 �������� in 1024 ����...   ÿֻ֡����� 1024/M ��
     *   ���� out 512 �������� out 512 ����...   ���� M=2 -> 512 ��/֡����Ҫ 2 ֡����ƴ 1024
     * **************************************************************************/
    static float32_t dec_buf[FFT_SIZE];
    static uint16_t  wr_ptr = 0;

    uint16_t out_len = FFT_SIZE / M;
    memcpy(&dec_buf[wr_ptr], tmp_out, out_len * sizeof(float32_t));
    wr_ptr += out_len;

    if (wr_ptr < FFT_SIZE) {
        /* ��û�ܹ����� �� �ȷ��ش��У�ʡ CPU */
        *out = coarse;
        return;
    }
    wr_ptr = 0;  // �����������һ��

    /* 4. ϸ�� FFT ���������������������������������������������������� */
    spectrum_analysis(dec_buf, FFT_SIZE, F_s / M, out);
}


/* ---------------------------------------------------------- */
/* ProcessSampleData_ByHint_F32                               */
/*  - ��֪Ŀ��Ƶ�ʣ���������                                   */
/* ---------------------------------------------------------- */
/**
 * @brief   ����Ӧ��ȡ + ϸ�ֱ��� FFT ������֪����Ƶ�ʣ�
 *
 * �ڲ������Ѿ��õ� #FFT_SIZE (=1024) ������������ǰ���£�
 * ����ǰ sweep �������Ƶ�ʣ���ȫ�� g_target_M �����������Ƿ��ȡ��
 *   ? M == 1 : ��Ƶ�Ρ���ֱ�� 1024-pt FFT��
 *   ? M > 1  : ���� FIR ��ͨ + ��ȡ M �� �� ���� 1024 ������ϸ�� FFT��
 *
 * ���̣�
 *   1. ͨ�� choose_M() Ԥ��ѡ�õĳ�ȡ���� g_target_M��
 *   2. �״ε���ʱ�� arm_fir_decimate_init_f32() ������ FIR ʵ����
 *   3. arm_fir_decimate_f32()   �� ��ͨ + ��������blockSize=1024����
 *   4. ����������ۼƵ� dec_buf[1024]���ܹ�������
 *      spectrum_analysis(dec_buf, 1024, F_s/M, out) �õ���ϸƵ�������
 *
 * @param[in]  in   ָ�� 1024 ���ѻ���ɷ��صĵ�ͨ��������float32_t����
 * @param[out] out  ָ����÷��ṩ�� #SpectrumResult_t �ṹ�壬���ڷ���
 *                  ��ϸƵ�ʡ����ȡ���λ�Ƚ����
 *
 * @note  ��������Ҫ�� DMA ��/ȫ����ص��У�
 *        1. DemuxADCData() �� interleave ����ͨ�����ݲ�ɸ��� buffer��
 *        2. ���� Set_DDS_Frequency() ���� g_target_M��
 *        3. �Ѷ�Ӧͨ���� buffer ָ�봫����������
 *
 * @warning  �������ڲ�ʹ������ static ���壺
 *           - tmp_out[512]  �� 2 kB
 *           - dec_buf[1024] �� 4 kB
 *           ���������ͨ��ͬʱ���ã���ֱ𿽱��˺�������ٽ���������
 */
static void ProcessSampleData_ByHint_F32(float *in, SpectrumResult_t *out, uint16_t g_target_M)
{
    uint16_t M = g_target_M;          //  ��ȡ����M
    if (M == 1) {                     // ��Ƶ�Σ�ֱ����һ�� 1024-pt FFT
        spectrum_analysis(in, FFT_SIZE, F_s, out);
        return;
    }

    /* --- ��ͨ + ��ȡ ------------------------------------ */
    Decimator_t *dec = get_dec(M);
    // ��ʼ��һ������ FIR ��ȡ��ʵ����д�� numTaps��M��pCoeffs��pState ��Ԫ����
    if (!dec->inst.numTaps) {         // �״� init
        arm_fir_decimate_init_f32(&dec->inst,
                                  dec->taps,
                                  dec->M,
                                  dec->coeff,
                                  dec->state,
                                  FFT_SIZE);
    }

    // ����һ�� (FIR ��ͨ + ����) �Ķ����ȡ������ blockSize ��������������� blockSize / M ������
    float32_t tmp_out[FFT_SIZE / 2];  // M��2 �� 512 max , ��󸲸ǵ�
    arm_fir_decimate_f32(&dec->inst, in, tmp_out, FFT_SIZE);

    /* --- �ۻ� M ֡ƴ�� 1024 �� -------------------------- */
    static float32_t dec_buf[FFT_SIZE];
    static uint16_t  wr_ptr = 0;

    uint16_t out_len = FFT_SIZE / M;
    memcpy(&dec_buf[wr_ptr], tmp_out, out_len * sizeof(float32_t));
    wr_ptr += out_len;

    if (wr_ptr < FFT_SIZE)            // ��û����
        return;

    wr_ptr = 0;                       // ��ָ��

    /* --- ϸ�У����� DDS / �˲�����΢С��� --------------- */
    spectrum_analysis(dec_buf, FFT_SIZE, F_s / M, out);
}
