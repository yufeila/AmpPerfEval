/**
 * @file basic_measurement_logic_review.md
 * @brief Basic_Measurement���������߼���鱨��
 * @date 2025-01-06
 * @author ��������Ŷ�
 */

# Basic_Measurement���������߼���鱨��

## 1. ����ܹ�����

### ? **����ְ��**
`Basic_Measurement`��������
- ��ʼ��1kHz�źŵĲ�������
- ʵʱ�ɼ��ʹ���ADC����
- ��ʾ������·�����������ѹ��Ƶ�ʡ��迹�ȣ�
- ��������迹���������败����

### ? **��������**
```
DDS(1kHz) �� �����· �� ADC���� �� DMA���� �� ���ݴ��� �� LCD��ʾ
                                           ��
                                    �̵������� �� ����迹����
```

## 2. �߼�������ϸ����

### 2.1 ��ʼ���׶�
```c
if(first_refresh)
{
    // ����DDSƵ��Ϊ1000Hz
    AD9851_Set_Frequency(1000);
    
    // ����ADC+DMA+TIMϵͳ�����õ�1000Hz��Ӧ�Ĳ�����
    current_fs = Start_ADC_DMA_TIM_System(1000.0f);
    
    // ��ʼ����Ļ��ʾ
    Basic_Measurement_Page_Init();
    first_refresh = 0;
}
```

**? �ŵ�**��
- ʹ�þ�̬����`first_refresh`�����ظ���ʼ��
- DDSƵ��������ADCϵͳ����֮ǰ��ȷ���ź��ȶ�
- ����ʵ�ʲ��������ں������ݴ���

**?? Ǳ������**��
- ȱ�ٳ�ʼ��ʧ�ܼ��
- û����֤DDSƵ�������Ƿ�ɹ�

### 2.2 ���ݲɼ��ʹ���
```c
// ���ݲɼ��ʹ���ʹ��ʵ�ʵĲ�����
Process_ADC_Data_F32(&data_at_1k.adc_in_Result, 
                    &data_at_1k.adc_ac_out_Result, 
                    &data_at_1k.adc_dc_out_Result, current_fs);
```

**? �ŵ�**��
- �����������ʣ���ߴ��������
- ���������ݽṹ���루���롢���������ֱ�������

## 3. ���ݴ���������

### 3.1 Process_ADC_Data_F32����
```c
uint8_t Process_ADC_Data_F32(SpectrumResult_t* pRes1, SpectrumResult_t* pRes2, float* pRes3, float fs)
```

**��������**��
1. **��黺����״̬**��`ADC_BufferReadyFlag == BUFFER_READY_FLAG_FULL`
2. **���ݷ���**��`DemuxADCData()` - �ӽ���洢����3��ͨ��
3. **�źŴ���**��
   - ͨ��1&2��`ProcessSampleData_F32()` - FFTƵ�׷���
   - ͨ��3��`CalcArrayMean()` - ֱ��ƽ��ֵ

**? �ŵ�**��
- ����ֵָʾ�Ƿ��������ݿ���
- ʹ�þ�̬�����������ظ������ڴ�
- ��ͬͨ��ʹ���ʵ��Ĵ�����

**?? Ǳ������**��
- ȱ��������Ч�Լ��
- FFT���������ƿ���Ӱ��ʵʱ��

### 3.2 DemuxADCData����
```c
static void DemuxADCData(const uint16_t *src, float *buf1, float *buf2, float *buf3, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
    {
        buf1[i] = ((float)src[3*i] - ADC_ZERO_CODE)* ADC_LSB_VOLT;
        buf2[i] = ((float)src[3*i+1] - ADC_ZERO_CODE)* ADC_LSB_VOLT;
        buf3[i] = ((float)src[3*i+2])* ADC_LSB_VOLT;  // ע�⣺ͨ��3�������
    }
}
```

**? �ŵ�**��
- ��ȷ����ADC���ݵĽ���洢��ʽ
- һ�������ADC��ֵ����ѹ��ת��
- ͨ��3��ֱ����������㣬����ֱ����������

**? ��Ҫȷ��**��
- `ADC_ZERO_CODE = 2048.0f` �Ƿ��ʺ�����ͨ����
- ͨ��3������������Ƿ���ȷ��

### 3.3 ProcessSampleData_F32����
```c
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes, float fs)
{
    const uint8_t process_interval = PROCESS_INTERVAL;
    static uint8_t process_counter = 0;

    if (++process_counter >= process_interval)
    {
        process_counter = 0;
        spectrum_analysis(sampleData, FFT_SIZE, fs, pRes);
    }
}
```

**? �ŵ�**��
- ʹ�ü�����Ƽ���CPU����
- �����������ʴ��ݸ�FFT����

**?? Ǳ������**��
- `PROCESS_INTERVAL = 1` ��ʾÿ�ζ������������δ��Ч
- ȱ��FFT����ʧ�ܵĴ�����

## 4. ��ʾϵͳ����

### 4.1 Basic_Measurement_Page_Update����

**��ʾ����**��
- �����źţ���ѹ���ȡ�Ƶ�ʡ������迹
- ����ź�(AC)����·��ѹ��Ƶ��
- ����ź�(DC)��ֱ����ѹ
- ����迹��������ʾ

**? �ŵ�**��
- ��������ʾ����
- �Զ���λת����Hz/kHz/MHz��
- ʵʱ����ˢ��

**?? ��������**��
1. **�������**��`LCD_Fill(38, 70, 230, 82, WHITE)` �� `LCD_Display_Frequency(..., 60, 85, ...)` λ�ò�ƥ��
2. **���Ƶ����ʾ����**�����AC������ʾƵ�ʣ���Ӧ��������Ƶ����ͬ
3. **ȱ�ٴ���ֵ����**��û�д����쳣����Ч�Ĳ������

## 5. ����迹��������

### 5.1 ��������
```c
if(measure_r_out_flag)
{
    // 1. ���濪·��ѹ
    SpectrumResult_t v_open_drain_out = data_at_1k.adc_ac_out_Result;
    
    // 2. �����̵���
    RELAY_ON;
    
    // 3. �ȴ��ȶ�
    HAL_Delay(1000);
    
    // 4. ��β��������ص�ѹ
    for(uint8_t i = 0; i<PROCESS_ARRAY_SIZE_WITH_R_L; i++) { ... }
    
    // 5. ƽ���ͼ���
    average_spectrumresult_array(&v_out_with_load, &v_out, PROCESS_ARRAY_SIZE_WITH_R_L);
    float R_out = CalculateRout(&v_out, &v_open_drain_out);
    
    // 6. ��ʾ���
    DisplayRout(R_out);
}
```

**? �ŵ�**��
- ��β�����߾���
- ������ȶ��ȴ�ʱ��
- �Զ������־λ

**?? Ǳ������**��
1. **�̵����ر�ȱʧ**��������ɺ�û�� `RELAY_OFF`
2. **��������**��`HAL_Delay(1000)` ����������ϵͳ
3. **���ݾ���**�������ڼ� `data_at_1k` ���ܱ�����

### 5.2 CalculateRout����
```c
static float CalculateRout(SpectrumResult_t* v_out_with_load, SpectrumResult_t* v_out_open_drain)
{
    if(v_out_with_load->amplitude > 0.001f)
    {
        return (v_out_open_drain->amplitude - v_out_with_load->amplitude) / v_out_with_load->amplitude * R_L;
    }
    return 0.0f;
}
```

**��ʽ��֤**��
���ڵ�ѹ��ѹԭ��`Rout = (Vopen - Vload) / Vload �� RL`

**? ��ʽ��ȷ**������Ҫע�⣺
- ���㱣����ֵ `0.001f` �Ƿ���ʣ�
- ����0�������û������鷵������ֵ��ʾ����ʧ��

## 6. ���ֵľ�������

### 6.1 ��������
1. **��ʾ���겻ƥ��**�������������ʾλ�ò�һ��
2. **�̵���δ�ر�**�����ܵ��¹��ĺͰ�ȫ����
3. **������ʱ**��Ӱ��ϵͳ��Ӧ��
4. **��������**��ȱ���쳣�������

### 6.2 �߼�����
1. **����迹����ʱ��**����������ʾ�����ڼ���У�����Ӱ��������ʾ
2. **����һ����**�������ڼ�ȱ��������������

## 7. �Ż�����

### 7.1 �����޸�
1. ������ʾ����ƥ������
2. ��Ӽ̵����رղ���
3. �Ľ����������

### 7.2 �����Ż�
1. ʹ�÷�������ʱ����
2. ʵ��������������
3. �Ż�FFT������

### 7.3 ������ǿ
1. ��Ӳ��������Ч�Լ��
2. ʵ���쳣ֵ����
3. �ṩ�û��ѺõĴ�����ʾ

## 8. ��������

**? �ŵ�**��
- ����ܹ�������ģ�黯����
- ���ݴ������̺���
- ��ʾ�����Ѻ�

**? ��Ҫ�Ľ�**��
- ������ʾ������Ҫ����
- ����迹����������Ҫ�Ż�
- �����������Ҫ��ǿ

**? �����޸�**��
- �̵���δ�رյİ�ȫ����
- ������ʱӰ��ϵͳ��Ӧ
- ��ʾ���겻ƥ�䵼�½����쳣

�ܵ���˵��`Basic_Measurement`�����ĺ����߼�����ȷ�ģ�����ҪһЩ�������Ż�������ȶ��Ժ��û����顣
