# ADC/DMA�����������ݼ�����

## ���������ܽ�

### 1. ��ʾ����
- **ֻ��DCͨ����ֵ**��`Vout (DC): 0.866 V` �������仯
- **AC/Ƶ���źŲ���Ϊ0**��
  - Input Signal: `Vs: 0.000 V`, `fs: 0.00 Hz`
  - Output Signal (AC): `Vout (Open): 0.000 V`, `fs: 0.00 Hz`
- **�쳣��ֵ**��`Rin: 34028236692093848000`�����Ƹ��������

### 2. ��������
- **Keil����**��`data_at_1k` ȷʵֻ�� `adc_dc_out_Result` ��ֵ
- **adc_buffer�޷��鿴**����Keil Watch�����п����� `adc_buffer[BUF_SIZE]` ��ֵ

## �������÷���

### ADC����
```c
// ADCͨ�����ã���ȷ��
ADC_CHANNEL_2 (PA2) -> Rank 1 -> �����ź�ͨ��
ADC_CHANNEL_4 (PA4) -> Rank 2 -> AC����ź�ͨ��  
ADC_CHANNEL_6 (PA6) -> Rank 3 -> DC����ź�ͨ��

// DMA����
BUF_SIZE = FFT_SIZE * ADC_CHANNELS = 2048 * 3 = 6144
���ݴ洢��ʽ��[CH2, CH4, CH6, CH2, CH4, CH6, ...]
```

### ���ݷ����߼�����
```c
// DemuxADCData����
for(uint16_t i = 0; i < len; i++)
{
    buf1[i] = ((float)src[3*i] - ADC_ZERO_CODE)* ADC_LSB_VOLT;     // CH2 (����)
    buf2[i] = ((float)src[3*i+1] - ADC_ZERO_CODE)* ADC_LSB_VOLT;   // CH4 (AC���)
    buf3[i] = ((float)src[3*i+2])* ADC_LSB_VOLT;                   // CH6 (DC������������)
}
```

**ע��**��ͨ��3 (DC���) �������ƫ�ƣ����Ǻ���ģ���ΪDC������Ҫ����ֵ��

### ����������
```c
#define ADC_ZERO_CODE    2048.0f    // 12λADC�е㣺2^12/2 = 2048
#define ADC_LSB_VOLT     0.0008058f // LSB��ѹ��3.3V / 4096 = 0.0008058V
#define FFT_SIZE         2048U      // FFT����
#define ADC_CHANNELS     3          // ADCͨ����
```

## ����ԭ�����

### 1. Keil�����п�����adc_buffer��ԭ��

#### ԭ��A���������Ż�
- `adc_buffer` ����Ϊ `__IO uint16_t`�������ܱ��Ż�������
- **�������**����Watch������ʹ�� `&adc_buffer[0],x` �� `(uint16_t*)adc_buffer,x` ��ʽ

#### ԭ��B��DMAδ��ɻ򻺳���δ��ʼ��
- DMA���������δ��ʼ�����
- **��鷽��**���鿴 `ADC_BufferReadyFlag` ״̬

#### ԭ��C���ڴ��ַ����
- ���������ܱ����䵽�����ڴ�����
- **��鷽��**���鿴MAP�ļ���adc_buffer���ڴ��ַ

### 2. AC/Ƶ���źŲ���Ϊ0��ԭ��

#### ԭ��A��ADC/DMA��������
- **ʱ������**��ADC��DMA��TIMͬ������������
- **����Դ**��`ADC_EXTERNALTRIGCONV_T2_TRGO` �����Ƿ���ȷ
- **����ʱ��**��3��ͨ���Ĳ���ʱ����ܲ�ͬ��

#### ԭ��B���ź�·������
- **Ӳ������**��PA2��PA4�����Ƿ��������ź�����
- **�źŷ���**�������źŷ��ȿ��ܳ���ADC������Χ
- **�ź�ƫ��**��AC�źſ�����Ҫƫ�õ�ADC���뷶Χ��

#### ԭ��C�����ݴ�������
- **DemuxADCData**��ͨ��ӳ����ܴ���
- **FFT����**��`spectrum_analysis` ��������������
- **������**��`PROCESS_INTERVAL` ���ܵ������ݶ�ʧ

#### ԭ��D������������
- `Rin: 34028236692093848000` �����Ǽ������
- ���ܳ������Լ�С������

## ���Խ���

### 1. ���������Ŀ

#### A. ��֤adc_buffer����
```c
// ��Keil�������г��ԣ�
&adc_buffer[0],100     // �鿴ǰ100������
(uint16_t*)0x20000XXX,100  // ʹ�þ����ڴ��ַ
```

#### B. ���ADC_BufferReadyFlag״̬
```c
// ȷ��DMA�Ƿ��������
if(ADC_BufferReadyFlag == BUFFER_READY_FLAG_FULL) {
    // �鿴adc_buffer��ʱ������
}
```

#### C. ���ԭʼ������־
```c
// ��DemuxADCData��������ӵ������
for(uint16_t i = 0; i < 10; i++) {  // ֻ��ǰ10�����ݵ�
    printf("ADC[%d]: CH2=%d, CH4=%d, CH6=%d\n", 
           i, src[3*i], src[3*i+1], src[3*i+2]);
}
```

### 2. ϵͳ���Ų�

#### ��һ������֤DMA���ݲɼ�
1. ȷ�� `adc_buffer` ��ȷʵ�з�������
2. ��֤3��ͨ���������Ƿ�Ԥ�ڽ���洢
3. ������ݷ�Χ�Ƿ���0-4095֮��

#### �ڶ�������֤���ݷ���
1. �� `DemuxADCData` ����3��float����������
2. ȷ�ϵ�ѹת�������Ƿ���ȷ
3. ��֤ACͨ��(buf1, buf2)�Ƿ�����Ч�Ľ�������

#### ����������֤FFT����
1. ��鴫�ݸ� `spectrum_analysis` �������Ƿ���Ч
2. ��֤FFT�������ص�Ƶ�ʺͷ��Ƚ��
3. ȷ�� `PROCESS_INTERVAL` �����Ƿ�������

#### ���Ĳ�����֤Ӳ���ź�
1. ��ʾ����ȷ��PA2��PA4��PA6���ŵ��ź�
2. ��֤�źŷ����Ƿ���ADC������Χ��(0-3.3V)
3. ����ź��Ƿ���Ҫ�ʵ���ƫ��

## �����޸�����

### ����1�����ӵ������
```c
// ��Process_ADC_Data_F32���������ӵ��Դ���
uint8_t Process_ADC_Data_F32(SpectrumResult_t* pRes1, SpectrumResult_t* pRes2, float* pRes3, float fs)
{
    // ... ���д��� ...
    
    // ���ԣ����ԭʼADC����
    uint32_t sum_ch2 = 0, sum_ch4 = 0, sum_ch6 = 0;
    for(int i = 0; i < 100; i++) {  // ���ǰ100��������
        sum_ch2 += adc_buffer[3*i];
        sum_ch4 += adc_buffer[3*i+1]; 
        sum_ch6 += adc_buffer[3*i+2];
    }
    
    // ͨ��UART��LCD��ʾͳ����Ϣ
    printf("ADC Debug: CH2_avg=%lu, CH4_avg=%lu, CH6_avg=%lu\n", 
           sum_ch2/100, sum_ch4/100, sum_ch6/100);
    
    // ... ����ԭ�д��� ...
}
```

### ����2��ǿ�����ݿɼ���
```c
// ��adc.c���޸�adc_buffer����
volatile uint16_t adc_buffer[BUF_SIZE] __attribute__((section(".bss")));
```

### ����3���򻯲���
```c
// ��ʱ����FFT��ֱ�Ӳ������ݲɼ�
static void Simple_ADC_Test(void) {
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE);
    HAL_TIM_Base_Start(&htim2);
    
    while(ADC_BufferReadyFlag != BUFFER_READY_FLAG_FULL);
    
    // ֱ����ʾԭʼADCֵ
    LCD_Show_ADC_Raw_Data(adc_buffer);
    
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
}
```

## Ԥ�ڽ��

�޸���Ӧ���ܿ�����
1. **adc_buffer��Ч����**��ÿ��ͨ����Ӧ����0-4095��Χ�ڵ�����
2. **ACͨ���н�������**������1kHz��2V�ź�Ӧ����FFT�м�⵽1kHz��ֵ
3. **Ƶ�ʲ�����ȷ**����ʾӦ�ýӽ������1kHz
4. **���Ȳ�����ȷ**����ʾӦ�ýӽ������2V����
5. **Rin��������**��������ʾ����ļ�����ֵ
