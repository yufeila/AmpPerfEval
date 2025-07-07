#include "./sweep_freq_response/sweep_freq_response.h"

//�ⲿ��������
extern DMA_HandleTypeDef hdma_adc1;
extern __IO  uint16_t adc_buffer[BUF_SIZE];
extern volatile uint8_t ADC_BufferReadyFlag;

// ȫ�ֱ�������
/* ����������ʾ */
static SignalAnalysisResult_t data_at_1k;
static SignalAnalysisResult_t last_data_at_1k;
static char             dispBuff[48];           // ��Ļ��ӡ������
static uint8_t          first_refresh = 1;      // ���Ƴ�ʼ��Ļˢ��

/* ɨƵ���� */
static FreqResponse_t freq_response[FREQ_POINTS];
static uint8_t measurement_complete = 0;
static uint8_t current_point = 0;


// �ڲ���������
static void     ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes);
static void DemuxADCData(const uint16_t *src, float *buf1, float *buf2,float *buf3, uint16_t len);
static void CalcArrayMean(const float* buf, float *pRes);

static float Calculate_Log_Frequency(uint8_t point_index);
static void LCD_Display_Title(const char* title);
static void Display_Frequency_Response_Curve(void);
static void Measure_Single_Point(float frequency, FreqResponse_t* result);
static void Draw_Frequency_Response_Points(void);
static void Draw_Coordinate_System(void);
static void Display_Measurement_Info(void);
static void Update_Measurement_Progress(uint8_t current_point, uint8_t total_points);

/* ============ ������������ ============ */



// ���㳤��ΪN��float�����ֵ�Ĺ��ߺ���
static float average_float_array(const float *arr, int N)
{
    float sum = 0.0f;
    for(int i = 0; i < N; ++i) {
        sum += arr[i];
    }
    if(N > 0)
        return sum / N;
    else
        return 0.0f; // ��ֹ����0
}


static void average_spectrumresult_array(SpectrumResult_t *res, SpectrumResult_t *avg_res, int N)
{
    float Freq = 0.0f;
    float Vpk = 0.0f;
    for(int i = 0; i < N; ++i) {
        Freq += res[i].frequency;
        Vpk += res[i].amplitude;
    }
    if(N > 0) {
        avg_res->frequency = Freq / N;
        avg_res->amplitude = Vpk / N;
        avg_res->bin_index = 0;
        avg_res->delta = 0.0f;
    } else {
        avg_res->frequency = 0.0f;
        avg_res->amplitude = 0.0f;
        avg_res->bin_index = 0;
        avg_res->delta = 0.0f;
    }
}

/**
 * @brief �������ݴ�������
 * 
 * �˺����������ݲɼ������ݷ�װ�����ݴ�������ݱȽ�
 * 
 * @return ���ݸ��±�־�� 0�����ݸ��� 1 ����δ����
 */
uint16_t Basic_Data_Process(void)
{
    uint8_t newDataProcessed = Process_ADC_Data_F32(&adc_in_Result, &adc_ac_out_Result, &adc_dc_out_Result);
}

/**
 * @brief ��ʾƵ�׽��
 * 
 * �˺���������ʾ FFT �����������ˢ�µ� LCD �ϡ�
 * 
 * @note ���� Mode_Select ��ֵ��רע���ض�ģʽ����ʾ��
 */
void Display_Spectrum_Results(void)
{
    static char dispBuff[50];
    static SpectrumResult_t temp_adc_in[WINDOW];
    static SpectrumResult_t temp_adc_ac_out[WINDOW];
    static float temp_adc_dc_out[WINDOW];
    static int cnt = 0;
    
    // ... �ɼ����������� ...
    uint8_t newDataProcessed = Process_ADC_Data_F32(&adc_in_Result, &adc_ac_out_Result, &adc_dc_out_Result);

    if (newDataProcessed || first_refresh) {
        first_refresh = 0;

        // ����д��
        temp_adc_in[cnt % WINDOW]     = adc_in_Result;
        temp_adc_ac_out[cnt % WINDOW] = adc_ac_out_Result;
        temp_adc_dc_out[cnt % WINDOW] = adc_dc_out_Result;
        cnt++;

        int valid_count = (cnt < WINDOW) ? cnt : WINDOW;

        SpectrumResult_t avg_in, avg_ac_out;
        average_spectrumresult_array(temp_adc_in, &avg_in, valid_count);
        average_spectrumresult_array(temp_adc_ac_out, &avg_ac_out, valid_count);

        // Vin AC ��ʾ
        LCD_ShowString(0, LINE(2), 240, 16, 16, (uint8_t*)"Vin AC Measuring Results");
        LCD_Fill(0, LINE(3), LCD_X_LENGTH, LINE(3)+16, BLACK);  // ���ָ������
        if(avg_in.frequency < 1000)
            sprintf(dispBuff, "Freq: %8.4f Hz", avg_in.frequency);
        else if(avg_in.frequency < 1000000)
            sprintf(dispBuff, "Freq: %8.4f kHz", avg_in.frequency / 1000.0f);
        else
            sprintf(dispBuff, "Freq: %8.4f MHz", avg_in.frequency / 1000000.0f);
        LCD_ShowString(0, LINE(3), 240, 16, 16, (uint8_t*)dispBuff);

        float Vpp = avg_in.amplitude * 2.0f;
        LCD_Fill(0, LINE(4), LCD_X_LENGTH, LINE(4)+16, BLACK);  // ���ָ������
        sprintf(dispBuff, "Vpp : %.3f V", Vpp);
        LCD_ShowString(0, LINE(4), 240, 16, 16, (uint8_t*)dispBuff);

        // Vout AC ��ʾ
        LCD_ShowString(0, LINE(6), 240, 16, 16, (uint8_t*)"Vout AC Measuring Results");
        ILI9341_Clear(0, LINE(7), LCD_X_LENGTH, 16);
        if(avg_ac_out.frequency < 1000)
            sprintf(dispBuff, "Freq: %8.4f Hz", avg_ac_out.frequency);
        else if(avg_ac_out.frequency < 1000000)
            sprintf(dispBuff, "Freq: %8.4f kHz", avg_ac_out.frequency / 1000.0f);
        else
            sprintf(dispBuff, "Freq: %8.4f MHz", avg_ac_out.frequency / 1000000.0f);
        LCD_ShowString(0, LINE(7), 240, 16, 16, (uint8_t*)dispBuff);
      
        Vpp = avg_ac_out.amplitude * 2.0f;
        ILI9341_Clear(0, LINE(8), LCD_X_LENGTH, 16);
        sprintf(dispBuff, "Vpp : %.3f V", Vpp);
        LCD_ShowString(0, LINE(8), 240, 16, 16, (uint8_t*)dispBuff);

        // Vout DC ��ʾ
        float avg_dc = average_float_array(temp_adc_dc_out, valid_count);
        LCD_ShowString(0, LINE(10), 240, 16, 16, (uint8_t*)"Vout DC Measuring Results");
        LCD_Fill(0, LINE(11), LCD_X_LENGTH, LINE(11)+16, BLACK);  // ���ָ������
        sprintf(dispBuff, "V bias : %.3f V", avg_dc);
        LCD_ShowString(0, LINE(11), 240, 16, 16, (uint8_t*)dispBuff);
    }
}


/**
 * @brief  ��ADC�������������ݣ�����������1024���ʽ��ת��Ϊ�����������д���
 * @param  ��
 * @retval 1 ��ʾ�������½����0 ��ʾ���½��
 */
uint8_t Process_ADC_Data_F32(void)
{  
  uint8_t new_data = 0;
  static float adc_in_buffer[FFT_SIZE];      // ͨ��1
  static float adc_ac_out_buffer[FFT_SIZE];  // ͨ��2
  static float adc_dc_out_buffer[FFT_SIZE];  // ͨ��3

    // ���ݲɼ��ͷ�װ��
    // 1. ��DMA��������������
    // 2. �����ݷ�װ�ɵ�3��������
    // 3. �ֱ���3���������е�����
    if (ADC_BufferReadyFlag == BUFFER_READY_FLAG_HALF)
    {
        //  �����ݷ�װ�ɵ�3��������
        const uint16_t *src = (const uint16_t *)&adc_buffer[0];
        DemuxADCData(src, adc_in_buffer, adc_ac_out_buffer, adc_dc_out_buffer, FFT_SIZE);                      // ���ݷּ�
        ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;

        // �ֱ���3���������е�����
        ProcessSampleData_F32(adc_in_buffer, data_at_1k.adc_in_Result);
        ProcessSampleData_F32(adc_ac_out_buffer, data_at_1k.adc_ac_out_Result);
        CalcArrayMean(adc_dc_out_buffer, data_at_1k.adc_dc_out_Result);

        new_data = 1;
    }
    else if (ADC_BufferReadyFlag == BUFFER_READY_FLAG_FULL)
    {
        //  �����ݷ�װ�ɵ�3��������
        const uint16_t *src = (const uint16_t *)&adc_buffer[0 + 3 * FFT_SIZE];
        DemuxADCData(src, adc_in_buffer, adc_ac_out_buffer, adc_dc_out_buffer, FFT_SIZE);                      // ���ݷּ�
        ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;

        // �ֱ���3���������е�����
        ProcessSampleData_F32(adc_in_buffer, data_at_1k.adc_in_Result);
        ProcessSampleData_F32(adc_ac_out_buffer, data_at_1k.adc_ac_out_Result);
        CalcArrayMean(adc_dc_out_buffer, data_at_1k.adc_dc_out_Result);

        new_data = 1;
    }

    return new_data;
}

/**
 * @brief ����ADC���ݵ������������У�������ѹת����
 * @param src ָ��Դ���������ָ�룬��������ͨ����ADC���ݡ�
 * @param buf1 ָ��ͨ��1���ݻ�������ָ�롣
 * @param buf2 ָ��ͨ��2���ݻ�������ָ�롣
 * @param buf3 ָ��ͨ��3���ݻ�������ָ�롣
 * @param len ÿ���������ĳ��ȣ���ÿ��ͨ�������ݵ���������
 */
static void DemuxADCData(const uint16_t *src,
                  float *buf1,
                  float *buf2,
                  float *buf3,
                  uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
    {
        buf1[i] = ((float)src[3*i] - ADC_ZERO_CODE)* ADC_LSB_VOLT;
        buf2[i] = ((float)src[3*i+1] - ADC_ZERO_CODE)* ADC_LSB_VOLT;   // ͨ��2
        buf3[i] = ((float)src[3*i+2])* ADC_LSB_VOLT;   // ͨ��3
    }
}

// ���� float �����ֵ���ʺ�ֱ������ƽ����
static void CalcArrayMean(const float* buf, float *pRes)
{
    if (buf == NULL)
        *pRes = 0.0f;

    float sum = 0.0f;
    for (uint16_t i = 0; i < FFT_SIZE; ++i)
        sum += buf[i];

    *pRes = sum / (float)FFT_SIZE;
}


/*------------------------------------------------------
 *  ��������: ProcessSampleData_F32
 *  ����: ���� spectrum_analysis() ���� MCU ����
 *  ����: ʹ�� process_interval ���Ƽ��ִ�� FFT
 *----------------------------------------------------*/
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes)
{
  

  const uint8_t process_interval = PROCESS_INTERVAL;   // ÿ�� 3 whileѭ��ִ��һ�� FFT
  static uint8_t process_counter  = 0;

  if (++process_counter >= process_interval)
  {
    process_counter = 0;
    spectrum_analysis(sampleData, FFT_SIZE, F_s, pRes);
  }
}

/* ============ ɨƵ�������� ============ */

/**
 * @brief �Զ�Ƶ�����Բ���������
 */
void Auto_Frequency_Response_Measurement(void)
{
    printf("Starting Automatic Frequency Response Measurement...\r\n");
    printf("Frequency Range: %.1f Hz - %.1f kHz\r\n", FREQ_START, FREQ_STOP/1000.0f);
    printf("Number of Points: %d\r\n", FREQ_POINTS);
    
    /* ͳһ������������ɫ */
    POINT_COLOR = RED;
    BACK_COLOR = BLACK;

    // ���LCD��ʾ����״̬
    LCD_Clear(BLACK);

    // ��ʾ����
    LCD_Display_Title("Freq Response Measurement");
    
    // ִ��ɨƵ����
    for(current_point = 0; current_point < FREQ_POINTS; current_point++)
    {
        // ���㵱ǰ����Ƶ�� (����ɨƵ)
        float freq = Calculate_Log_Frequency(current_point);
        
        // ����DDS���Ƶ��
        AD9851_Set_Frequency(freq);
        
        // �ȴ��ź��ȶ�
        HAL_Delay(SETTLE_TIME_MS);
        
        // ������Ƶ��ķ�Ƶ��Ӧ
        Measure_Single_Point(freq, &freq_response[current_point]);
        
        // ����LCD��ʾ����
        Update_Measurement_Progress(current_point, FREQ_POINTS);
        
        // ��������������
        printf("Point %d: %.1f Hz, Gain: %.2f dB\r\n", 
               current_point, freq, freq_response[current_point].gain_db);
    }
    
    measurement_complete = 1;
    printf("Measurement Complete!\r\n");
	
	LCD_Clear(BLACK);
    
    // ��ʾ������Ƶ����������
    Display_Frequency_Response_Curve();
}

/**
 * @brief �������ɨƵ��Ƶ�ʵ�
 */
static float Calculate_Log_Frequency(uint8_t point_index)
{
    // ����ɨƵ��Ƶ�ʰ������ֲ�
    float log_start = log10f(FREQ_START);   
    float log_stop = log10f(FREQ_STOP);
    float log_step = (log_stop - log_start) / (FREQ_POINTS - 1);
    
    float log_freq = log_start + point_index * log_step;
    return powf(10.0f, log_freq);   
}

/**
 * @brief ����DDS���Ƶ��
 */
void Set_DDS_Frequency(float frequency)
{
    printf("Setting DDS to %.2f Hz...\r\n", frequency);
    
    // ����AD9851����Ƶ��
    AD9851_Set_Frequency((uint32_t)frequency);
    
}

/**
 * @brief ��������Ƶ�����Ӧ
 */
static void Measure_Single_Point(float frequency, FreqResponse_t* result)
{
    static SpectrumResult_t input_signal;
    static SpectrumResult_t output_signal;
    static float dc_offset;
    
    // ��β���ȡƽ��ֵ��߾���
    float input_sum = 0.0f;
    float output_sum = 0.0f;
    uint8_t sample_count = 5;
    
    for(uint8_t i = 0; i < sample_count; i++)
    {
        // �ȴ��µ�ADC����
        while(!Process_ADC_Data_F32(&input_signal, &output_signal, &dc_offset))
        {
            HAL_Delay(10);
        }
        
        input_sum += input_signal.amplitude;
        output_sum += output_signal.amplitude;
        
        HAL_Delay(20);  // �������
    }
    
    // ����ƽ��ֵ
    float input_avg = input_sum / sample_count;
    float output_avg = output_sum / sample_count;
    
    // ���������
    result->frequency = frequency;
    result->input_amp = ((float)input_avg - ADC_ZERO_CODE) * ADC_LSB_VOLT;   // ת��Ϊ��ѹֵ
    result->output_amp = ((float)output_avg - ADC_ZERO_CODE) * ADC_LSB_VOLT;    //ת��Ϊ��ѹֵ
    
    // �������� (dB)
    if(result->input_amp > 0.001f)  // �������
    {
        float linear_gain = result->output_amp / result->input_amp;
        result->gain_db = 20.0f * log10f(linear_gain);          // ת��ΪdB��λ
    }
    else
    {
        result->gain_db = -100.0f;  // ����̫С�����Ϊ��Ч
    }
    
    // ��λ���� (��Ҫ�����ӵ��㷨�������)
    result->phase_deg = 0.0f;  // ��ʱ��ʵ����λ����
}

/**
 * @brief ��LCD����ʾƵ����������
 */
static void Display_Frequency_Response_Curve(void)
{
    POINT_COLOR = WHITE;
    BACK_COLOR = BLACK;
    
    // ��ʾ����
    LCD_ShowString(0, 0, 240, 16, 16, (uint8_t*)"Freq Response Measurement");

    // ����������
    Draw_Coordinate_System();
    
    // ����Ƶ����������
    Draw_Frequency_Response_Points();
    
    // ��ʾ��������
    Display_Measurement_Info();
}

/**
 * @brief ��������ϵ
 */
static void Draw_Coordinate_System(void)
{
    // �����ͼ����
    uint16_t plot_x_start = 30;
    uint16_t plot_x_end = 240;
    uint16_t plot_y_start = 40;
    uint16_t plot_y_end = 180;
    
    // ����������
    POINT_COLOR = WHITE;
    
    // X�� (Ƶ����) - �ڵײ�
    LCD_DrawLine(plot_x_start, plot_y_end, plot_x_end, plot_y_end);
    // X���ͷ (�Ҽ�ͷ)
    LCD_DrawLine(plot_x_end, plot_y_end, plot_x_end-ARROW_SIZE, plot_y_end-ARROW_SIZE/2);  // ��б��
    LCD_DrawLine(plot_x_end, plot_y_end, plot_x_end-ARROW_SIZE, plot_y_end+ARROW_SIZE/2);  // ��б��

    // Y�� (������) - �ӵײ�������
    LCD_DrawLine(plot_x_start, plot_y_start, plot_x_start, plot_y_end);
    // Y���ͷ (�ϼ�ͷ)
    LCD_DrawLine(plot_x_start, plot_y_start, plot_x_start-ARROW_SIZE/2, plot_y_start+ARROW_SIZE);  // ��б��
    LCD_DrawLine(plot_x_start, plot_y_start, plot_x_start+ARROW_SIZE/2, plot_y_start+ARROW_SIZE);  // ��б��

    // ���ƿ̶���
    char label[20];
    
    // Ƶ�ʿ̶� (�����̶�) - ��X����
    for(uint8_t i = 0; i <= 5; i++)
    {
        uint16_t x = plot_x_start + i * (plot_x_end - plot_x_start) / 5;
        float freq = powf(10.0f, 2.0f + i);  // 100Hz, 1kHz, 10kHz, 100kHz, 1M
        
        // �̶�����X����
        ILI9341_DrawLine(x, plot_y_end-3, x, plot_y_end+3);
        
        if(freq < 1000)
            sprintf(label, "%.0fHz", freq);
        else if (freq < 1000000 && freq >= 1000)
            sprintf(label, "%.0fk", freq/1000.0f);
        else
			sprintf(label, "%.0fM",freq/1000000) ;
        // ��ǩ��X���·�
        ILI9341_DispString_EN(x-15, plot_y_end+8, label);
    }
    
    // ����̶� - ��Y����
    for(int8_t i = -3; i <= 3; i++)
    {
        // ע�⣺�����Y���������Ҫ��ת
        uint16_t y = plot_y_end - (i + 3) * (plot_y_end - plot_y_start) / 6;
        int16_t gain_db = (i) * 10;  // +30dB to -30dB (���ϵ���)
        
        // �̶�����Y����
        ILI9341_DrawLine(plot_x_start-3, y, plot_x_start+3, y);
        
        sprintf(label, "%+d", gain_db);
        // ��ǩ��Y�����
        ILI9341_DispString_EN(0, y, label);
    }
    
    // ����������ǩ
    POINT_COLOR = CYAN;
    LCD_ShowString(plot_x_end-40, plot_y_end+25, 80, 16, 16, (uint8_t*)"Freq(Hz)");  // X���ǩ
    LCD_ShowString(5, plot_y_start-25, 80, 16, 16, (uint8_t*)"Gain(dB)");            // Y���ǩ
}


/**
 * @brief ����Ƶ���������ݵ�
 */
static void Draw_Frequency_Response_Points(void)
{
    uint16_t plot_x_start = 30;
    uint16_t plot_x_end = 240;
    uint16_t plot_y_start = 40;
    uint16_t plot_y_end = 180;
    
    POINT_COLOR = RED;
    
    for(uint8_t i = 0; i < FREQ_POINTS; i++)
    {
        // ������Ļ����
        float log_freq = log10f(freq_response[i].frequency);
        float log_start = log10f(FREQ_START);
        float log_stop = log10f(FREQ_STOP);
        
        // X���� (�����̶�)
        uint16_t x = plot_x_start + (log_freq - log_start) * 
                     (plot_x_end - plot_x_start) / (log_stop - log_start);  // x��Ҫȡ��������
        
        // Y���� (����̶�: -30dB to +30dB)
        float gain_clamped = freq_response[i].gain_db;
        if(gain_clamped > 30.0f) gain_clamped = 30.0f;
        if(gain_clamped < -30.0f) gain_clamped = -30.0f;
        
        uint16_t y = plot_y_end - (gain_clamped + 30.0f) * 
                     (plot_y_end - plot_y_start) / 60.0f;
        
        // �������ݵ�
        ILI9341_DrawLine(x - 2, y, x + 2, y);
        ILI9341_DrawLine(x, y + 2, x, y - 2);
        
        // ���� (������ǵ�һ����)
        if(i > 0)
        {
            float prev_log_freq = log10f(freq_response[i-1].frequency);
            uint16_t prev_x = plot_x_start + (prev_log_freq - log_start) * 
                             (plot_x_end - plot_x_start) / (log_stop - log_start);
            
            float prev_gain_clamped = freq_response[i-1].gain_db;
            if(prev_gain_clamped > 30.0f) prev_gain_clamped = 30.0f;
            if(prev_gain_clamped < -30.0f) prev_gain_clamped = -30.0f;
            
            uint16_t prev_y = plot_y_end - (prev_gain_clamped + 30.0f) * 
                             (plot_y_end - plot_y_start) / 60.0f;
            
            ILI9341_DrawLine(prev_x, prev_y, x, y);
        }
    }
}

/**
 * @brief ��ʾ������Ϣ
 */
static void Display_Measurement_Info(void)
{
    char info[50];
    
    POINT_COLOR = YELLOW;
    
    // �ҵ���������
    float max_gain = -100.0f;
    float max_gain_freq = 0.0f;
    
    for(uint8_t i = 0; i < FREQ_POINTS; i++)
    {
        if(freq_response[i].gain_db > max_gain)
        {
            max_gain = freq_response[i].gain_db;
            max_gain_freq = freq_response[i].frequency;
        }
    }
    
    // ��ʾ��ֵ����
    sprintf(info, "Peak: %.1fdB @ %.1fHz", max_gain, max_gain_freq);
    LCD_ShowString(0, LINE(14), 240, 16, 16, (uint8_t*)info);
    
    // ����-3dB���� (��ʵ��)
    float target_gain = max_gain - 3.0f;
    float bandwidth_low = 0.0f, bandwidth_high = 0.0f;
    
    // ����-3dB��
    for(uint8_t i = 0; i < FREQ_POINTS; i++)
    {
        if(freq_response[i].gain_db >= target_gain)
        {
            if(bandwidth_low == 0.0f) bandwidth_low = freq_response[i].frequency;
            bandwidth_high = freq_response[i].frequency;
        }
    }
    
    sprintf(info, "BW(-3dB): %.1f-%.1fHz", bandwidth_low, bandwidth_high);
    ILI9341_DispStringLine_EN(LINE(15), info);
}

/**
 * @brief LCD��ʾ����
 */
static void LCD_Display_Title(const char* title)
{
    LCD_SetFont(&Font8x16);
    LCD_SetColors(CYAN, BLACK);
    ILI9341_DispString_EN(10, 10,(char *) title);
}

/**
 * @brief ���²���������ʾ
 */
static void Update_Measurement_Progress(uint8_t current_point, uint8_t total_points)
{
    char progress_str[50];
    sprintf(progress_str, "Progress: %d/%d (%.1f%%)", 
            current_point + 1, total_points, 
            ((float)(current_point + 1) / total_points) * 100.0f);
    
    // ���֮ǰ�Ľ�����ʾ
    LCD_SetColors(BLACK, BLACK);
    ILI9341_Clear(10, 50, 300, 20);
    
    // ��ʾ�µĽ���
    LCD_SetColors(YELLOW, BLACK);
    ILI9341_DispString_EN(10, 50, progress_str);
}