#include "./sweep_freq_response/sweep_freq_response.h"

//外部变量声明
extern DMA_HandleTypeDef hdma_adc1;
extern __IO  uint16_t adc_buffer[BUF_SIZE];
extern volatile uint8_t ADC_BufferReadyFlag;

// 全局变量声明
/* 基本参数显示 */
static SignalAnalysisResult_t data_at_1k;
static SignalAnalysisResult_t last_data_at_1k;
static char             dispBuff[48];           // 屏幕打印缓冲区
static uint8_t          first_refresh = 1;      // 控制初始屏幕刷新

/* 扫频参数 */
static FreqResponse_t freq_response[FREQ_POINTS];
static uint8_t measurement_complete = 0;
static uint8_t current_point = 0;


// 内部函数声明
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

/* ============ 基本测量部分 ============ */



// 计算长度为N的float数组均值的工具函数
static float average_float_array(const float *arr, int N)
{
    float sum = 0.0f;
    for(int i = 0; i < N; ++i) {
        sum += arr[i];
    }
    if(N > 0)
        return sum / N;
    else
        return 0.0f; // 防止除以0
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
 * @brief 基本数据处理还能输
 * 
 * 此函数用于数据采集，数据封装，数据处理和数据比较
 * 
 * @return 数据更新标志， 0：数据更新 1 数据未更新
 */
uint16_t Basic_Data_Process(void)
{
    uint8_t newDataProcessed = Process_ADC_Data_F32(&adc_in_Result, &adc_ac_out_Result, &adc_dc_out_Result);
}

/**
 * @brief 显示频谱结果
 * 
 * 此函数用于显示 FFT 结果，并将其刷新到 LCD 上。
 * 
 * @note 根据 Mode_Select 的值，专注于特定模式的显示。
 */
void Display_Spectrum_Results(void)
{
    static char dispBuff[50];
    static SpectrumResult_t temp_adc_in[WINDOW];
    static SpectrumResult_t temp_adc_ac_out[WINDOW];
    static float temp_adc_dc_out[WINDOW];
    static int cnt = 0;
    
    // ... 采集到的新数据 ...
    uint8_t newDataProcessed = Process_ADC_Data_F32(&adc_in_Result, &adc_ac_out_Result, &adc_dc_out_Result);

    if (newDataProcessed || first_refresh) {
        first_refresh = 0;

        // 环形写入
        temp_adc_in[cnt % WINDOW]     = adc_in_Result;
        temp_adc_ac_out[cnt % WINDOW] = adc_ac_out_Result;
        temp_adc_dc_out[cnt % WINDOW] = adc_dc_out_Result;
        cnt++;

        int valid_count = (cnt < WINDOW) ? cnt : WINDOW;

        SpectrumResult_t avg_in, avg_ac_out;
        average_spectrumresult_array(temp_adc_in, &avg_in, valid_count);
        average_spectrumresult_array(temp_adc_ac_out, &avg_ac_out, valid_count);

        // Vin AC 显示
        LCD_ShowString(0, LINE(2), 240, 16, 16, (uint8_t*)"Vin AC Measuring Results");
        LCD_Fill(0, LINE(3), LCD_X_LENGTH, LINE(3)+16, BLACK);  // 清除指定区域
        if(avg_in.frequency < 1000)
            sprintf(dispBuff, "Freq: %8.4f Hz", avg_in.frequency);
        else if(avg_in.frequency < 1000000)
            sprintf(dispBuff, "Freq: %8.4f kHz", avg_in.frequency / 1000.0f);
        else
            sprintf(dispBuff, "Freq: %8.4f MHz", avg_in.frequency / 1000000.0f);
        LCD_ShowString(0, LINE(3), 240, 16, 16, (uint8_t*)dispBuff);

        float Vpp = avg_in.amplitude * 2.0f;
        LCD_Fill(0, LINE(4), LCD_X_LENGTH, LINE(4)+16, BLACK);  // 清除指定区域
        sprintf(dispBuff, "Vpp : %.3f V", Vpp);
        LCD_ShowString(0, LINE(4), 240, 16, 16, (uint8_t*)dispBuff);

        // Vout AC 显示
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

        // Vout DC 显示
        float avg_dc = average_float_array(temp_adc_dc_out, valid_count);
        LCD_ShowString(0, LINE(10), 240, 16, 16, (uint8_t*)"Vout DC Measuring Results");
        LCD_Fill(0, LINE(11), LCD_X_LENGTH, LINE(11)+16, BLACK);  // 清除指定区域
        sprintf(dispBuff, "V bias : %.3f V", avg_dc);
        LCD_ShowString(0, LINE(11), 240, 16, 16, (uint8_t*)dispBuff);
    }
}


/**
 * @brief  从ADC缓冲区处理数据，将定点数（1024点格式）转换为浮点数并进行处理。
 * @param  无
 * @retval 1 表示已生成新结果，0 表示无新结果
 */
uint8_t Process_ADC_Data_F32(void)
{  
  uint8_t new_data = 0;
  static float adc_in_buffer[FFT_SIZE];      // 通道1
  static float adc_ac_out_buffer[FFT_SIZE];  // 通道2
  static float adc_dc_out_buffer[FFT_SIZE];  // 通道3

    // 数据采集和分装：
    // 1. 从DMA缓存区搬运数据
    // 2. 将数据分装成到3个子数组
    // 3. 分别处理3个子数组中的数据
    if (ADC_BufferReadyFlag == BUFFER_READY_FLAG_HALF)
    {
        //  将数据分装成到3个子数组
        const uint16_t *src = (const uint16_t *)&adc_buffer[0];
        DemuxADCData(src, adc_in_buffer, adc_ac_out_buffer, adc_dc_out_buffer, FFT_SIZE);                      // 数据分拣
        ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;

        // 分别处理3个子数组中的数据
        ProcessSampleData_F32(adc_in_buffer, data_at_1k.adc_in_Result);
        ProcessSampleData_F32(adc_ac_out_buffer, data_at_1k.adc_ac_out_Result);
        CalcArrayMean(adc_dc_out_buffer, data_at_1k.adc_dc_out_Result);

        new_data = 1;
    }
    else if (ADC_BufferReadyFlag == BUFFER_READY_FLAG_FULL)
    {
        //  将数据分装成到3个子数组
        const uint16_t *src = (const uint16_t *)&adc_buffer[0 + 3 * FFT_SIZE];
        DemuxADCData(src, adc_in_buffer, adc_ac_out_buffer, adc_dc_out_buffer, FFT_SIZE);                      // 数据分拣
        ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;

        // 分别处理3个子数组中的数据
        ProcessSampleData_F32(adc_in_buffer, data_at_1k.adc_in_Result);
        ProcessSampleData_F32(adc_ac_out_buffer, data_at_1k.adc_ac_out_Result);
        CalcArrayMean(adc_dc_out_buffer, data_at_1k.adc_dc_out_Result);

        new_data = 1;
    }

    return new_data;
}

/**
 * @brief 分离ADC数据到三个缓冲区中，并做电压转换。
 * @param src 指向源数据数组的指针，包含所有通道的ADC数据。
 * @param buf1 指向通道1数据缓冲区的指针。
 * @param buf2 指向通道2数据缓冲区的指针。
 * @param buf3 指向通道3数据缓冲区的指针。
 * @param len 每个缓冲区的长度（即每个通道的数据点数量）。
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
        buf2[i] = ((float)src[3*i+1] - ADC_ZERO_CODE)* ADC_LSB_VOLT;   // 通道2
        buf3[i] = ((float)src[3*i+2])* ADC_LSB_VOLT;   // 通道3
    }
}

// 计算 float 数组均值（适合直流分量平均）
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
 *  函数名称: ProcessSampleData_F32
 *  功能: 调用 spectrum_analysis() 减轻 MCU 负担
 *  描述: 使用 process_interval 控制间隔执行 FFT
 *----------------------------------------------------*/
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes)
{
  

  const uint8_t process_interval = PROCESS_INTERVAL;   // 每隔 3 while循环执行一次 FFT
  static uint8_t process_counter  = 0;

  if (++process_counter >= process_interval)
  {
    process_counter = 0;
    spectrum_analysis(sampleData, FFT_SIZE, F_s, pRes);
  }
}

/* ============ 扫频测量部分 ============ */

/**
 * @brief 自动频幅特性测量主函数
 */
void Auto_Frequency_Response_Measurement(void)
{
    printf("Starting Automatic Frequency Response Measurement...\r\n");
    printf("Frequency Range: %.1f Hz - %.1f kHz\r\n", FREQ_START, FREQ_STOP/1000.0f);
    printf("Number of Points: %d\r\n", FREQ_POINTS);
    
    /* 统一设置字体与颜色 */
    POINT_COLOR = RED;
    BACK_COLOR = BLACK;

    // 清除LCD显示测量状态
    LCD_Clear(BLACK);

    // 显示标题
    LCD_Display_Title("Freq Response Measurement");
    
    // 执行扫频测量
    for(current_point = 0; current_point < FREQ_POINTS; current_point++)
    {
        // 计算当前测量频率 (对数扫频)
        float freq = Calculate_Log_Frequency(current_point);
        
        // 设置DDS输出频率
        AD9851_Set_Frequency(freq);
        
        // 等待信号稳定
        HAL_Delay(SETTLE_TIME_MS);
        
        // 测量该频点的幅频响应
        Measure_Single_Point(freq, &freq_response[current_point]);
        
        // 更新LCD显示进度
        Update_Measurement_Progress(current_point, FREQ_POINTS);
        
        // 串口输出测量结果
        printf("Point %d: %.1f Hz, Gain: %.2f dB\r\n", 
               current_point, freq, freq_response[current_point].gain_db);
    }
    
    measurement_complete = 1;
    printf("Measurement Complete!\r\n");
	
	LCD_Clear(BLACK);
    
    // 显示完整的频幅特性曲线
    Display_Frequency_Response_Curve();
}

/**
 * @brief 计算对数扫频的频率点
 */
static float Calculate_Log_Frequency(uint8_t point_index)
{
    // 对数扫频：频率按对数分布
    float log_start = log10f(FREQ_START);   
    float log_stop = log10f(FREQ_STOP);
    float log_step = (log_stop - log_start) / (FREQ_POINTS - 1);
    
    float log_freq = log_start + point_index * log_step;
    return powf(10.0f, log_freq);   
}

/**
 * @brief 设置DDS输出频率
 */
void Set_DDS_Frequency(float frequency)
{
    printf("Setting DDS to %.2f Hz...\r\n", frequency);
    
    // 调用AD9851设置频率
    AD9851_Set_Frequency((uint32_t)frequency);
    
}

/**
 * @brief 测量单个频点的响应
 */
static void Measure_Single_Point(float frequency, FreqResponse_t* result)
{
    static SpectrumResult_t input_signal;
    static SpectrumResult_t output_signal;
    static float dc_offset;
    
    // 多次采样取平均值提高精度
    float input_sum = 0.0f;
    float output_sum = 0.0f;
    uint8_t sample_count = 5;
    
    for(uint8_t i = 0; i < sample_count; i++)
    {
        // 等待新的ADC数据
        while(!Process_ADC_Data_F32(&input_signal, &output_signal, &dc_offset))
        {
            HAL_Delay(10);
        }
        
        input_sum += input_signal.amplitude;
        output_sum += output_signal.amplitude;
        
        HAL_Delay(20);  // 采样间隔
    }
    
    // 计算平均值
    float input_avg = input_sum / sample_count;
    float output_avg = output_sum / sample_count;
    
    // 填充测量结果
    result->frequency = frequency;
    result->input_amp = ((float)input_avg - ADC_ZERO_CODE) * ADC_LSB_VOLT;   // 转换为电压值
    result->output_amp = ((float)output_avg - ADC_ZERO_CODE) * ADC_LSB_VOLT;    //转换为电压值
    
    // 计算增益 (dB)
    if(result->input_amp > 0.001f)  // 避免除零
    {
        float linear_gain = result->output_amp / result->input_amp;
        result->gain_db = 20.0f * log10f(linear_gain);          // 转化为dB单位
    }
    else
    {
        result->gain_db = -100.0f;  // 输入太小，标记为无效
    }
    
    // 相位测量 (需要更复杂的算法，这里简化)
    result->phase_deg = 0.0f;  // 暂时不实现相位测量
}

/**
 * @brief 在LCD上显示频幅特性曲线
 */
static void Display_Frequency_Response_Curve(void)
{
    POINT_COLOR = WHITE;
    BACK_COLOR = BLACK;
    
    // 显示标题
    LCD_ShowString(0, 0, 240, 16, 16, (uint8_t*)"Freq Response Measurement");

    // 绘制坐标轴
    Draw_Coordinate_System();
    
    // 绘制频幅特性曲线
    Draw_Frequency_Response_Points();
    
    // 显示测量参数
    Display_Measurement_Info();
}

/**
 * @brief 绘制坐标系
 */
static void Draw_Coordinate_System(void)
{
    // 定义绘图区域
    uint16_t plot_x_start = 30;
    uint16_t plot_x_end = 240;
    uint16_t plot_y_start = 40;
    uint16_t plot_y_end = 180;
    
    // 绘制坐标轴
    POINT_COLOR = WHITE;
    
    // X轴 (频率轴) - 在底部
    LCD_DrawLine(plot_x_start, plot_y_end, plot_x_end, plot_y_end);
    // X轴箭头 (右箭头)
    LCD_DrawLine(plot_x_end, plot_y_end, plot_x_end-ARROW_SIZE, plot_y_end-ARROW_SIZE/2);  // 上斜线
    LCD_DrawLine(plot_x_end, plot_y_end, plot_x_end-ARROW_SIZE, plot_y_end+ARROW_SIZE/2);  // 下斜线

    // Y轴 (增益轴) - 从底部到顶部
    LCD_DrawLine(plot_x_start, plot_y_start, plot_x_start, plot_y_end);
    // Y轴箭头 (上箭头)
    LCD_DrawLine(plot_x_start, plot_y_start, plot_x_start-ARROW_SIZE/2, plot_y_start+ARROW_SIZE);  // 左斜线
    LCD_DrawLine(plot_x_start, plot_y_start, plot_x_start+ARROW_SIZE/2, plot_y_start+ARROW_SIZE);  // 右斜线

    // 绘制刻度线
    char label[20];
    
    // 频率刻度 (对数刻度) - 在X轴上
    for(uint8_t i = 0; i <= 5; i++)
    {
        uint16_t x = plot_x_start + i * (plot_x_end - plot_x_start) / 5;
        float freq = powf(10.0f, 2.0f + i);  // 100Hz, 1kHz, 10kHz, 100kHz, 1M
        
        // 刻度线在X轴上
        ILI9341_DrawLine(x, plot_y_end-3, x, plot_y_end+3);
        
        if(freq < 1000)
            sprintf(label, "%.0fHz", freq);
        else if (freq < 1000000 && freq >= 1000)
            sprintf(label, "%.0fk", freq/1000.0f);
        else
			sprintf(label, "%.0fM",freq/1000000) ;
        // 标签在X轴下方
        ILI9341_DispString_EN(x-15, plot_y_end+8, label);
    }
    
    // 增益刻度 - 在Y轴上
    for(int8_t i = -3; i <= 3; i++)
    {
        // 注意：这里的Y坐标计算需要翻转
        uint16_t y = plot_y_end - (i + 3) * (plot_y_end - plot_y_start) / 6;
        int16_t gain_db = (i) * 10;  // +30dB to -30dB (从上到下)
        
        // 刻度线在Y轴上
        ILI9341_DrawLine(plot_x_start-3, y, plot_x_start+3, y);
        
        sprintf(label, "%+d", gain_db);
        // 标签在Y轴左侧
        ILI9341_DispString_EN(0, y, label);
    }
    
    // 添加坐标轴标签
    POINT_COLOR = CYAN;
    LCD_ShowString(plot_x_end-40, plot_y_end+25, 80, 16, 16, (uint8_t*)"Freq(Hz)");  // X轴标签
    LCD_ShowString(5, plot_y_start-25, 80, 16, 16, (uint8_t*)"Gain(dB)");            // Y轴标签
}


/**
 * @brief 绘制频幅特性数据点
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
        // 计算屏幕坐标
        float log_freq = log10f(freq_response[i].frequency);
        float log_start = log10f(FREQ_START);
        float log_stop = log10f(FREQ_STOP);
        
        // X坐标 (对数刻度)
        uint16_t x = plot_x_start + (log_freq - log_start) * 
                     (plot_x_end - plot_x_start) / (log_stop - log_start);  // x需要取整数精度
        
        // Y坐标 (增益刻度: -30dB to +30dB)
        float gain_clamped = freq_response[i].gain_db;
        if(gain_clamped > 30.0f) gain_clamped = 30.0f;
        if(gain_clamped < -30.0f) gain_clamped = -30.0f;
        
        uint16_t y = plot_y_end - (gain_clamped + 30.0f) * 
                     (plot_y_end - plot_y_start) / 60.0f;
        
        // 绘制数据点
        ILI9341_DrawLine(x - 2, y, x + 2, y);
        ILI9341_DrawLine(x, y + 2, x, y - 2);
        
        // 连线 (如果不是第一个点)
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
 * @brief 显示测量信息
 */
static void Display_Measurement_Info(void)
{
    char info[50];
    
    POINT_COLOR = YELLOW;
    
    // 找到最大增益点
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
    
    // 显示峰值增益
    sprintf(info, "Peak: %.1fdB @ %.1fHz", max_gain, max_gain_freq);
    LCD_ShowString(0, LINE(14), 240, 16, 16, (uint8_t*)info);
    
    // 计算-3dB带宽 (简化实现)
    float target_gain = max_gain - 3.0f;
    float bandwidth_low = 0.0f, bandwidth_high = 0.0f;
    
    // 查找-3dB点
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
 * @brief LCD显示标题
 */
static void LCD_Display_Title(const char* title)
{
    LCD_SetFont(&Font8x16);
    LCD_SetColors(CYAN, BLACK);
    ILI9341_DispString_EN(10, 10,(char *) title);
}

/**
 * @brief 更新测量进度显示
 */
static void Update_Measurement_Progress(uint8_t current_point, uint8_t total_points)
{
    char progress_str[50];
    sprintf(progress_str, "Progress: %d/%d (%.1f%%)", 
            current_point + 1, total_points, 
            ((float)(current_point + 1) / total_points) * 100.0f);
    
    // 清除之前的进度显示
    LCD_SetColors(BLACK, BLACK);
    ILI9341_Clear(10, 50, 300, 20);
    
    // 显示新的进度
    LCD_SetColors(YELLOW, BLACK);
    ILI9341_DispString_EN(10, 50, progress_str);
}