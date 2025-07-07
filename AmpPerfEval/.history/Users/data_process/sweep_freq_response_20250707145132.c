#include "sweep_freq_response.h"
#include <stdio.h>
#include <string.h>

//外部变量声明
extern __IO  uint16_t adc_bufeerZE];
extern volatile uint8_t ADC_BufferReadyFlag;
extern uint8_t basic_measurement; 
extern uint8_t sweep_freq_response;
extern uint8_t measure_R_out;

// 全局变量声明
/* 基本参数显示 */
static SignalAnalysisResult_t data_at_1k;
static SignalAnalysisResult_t last_data_at_1k;
static char             dispBuff[48];           // 屏幕打印缓冲区
static uint8_t          first_refresh1 = 1;      // 控制初始屏幕刷新
static uint8_t          first_refresh2 = 0;      // 控制初始屏幕刷新
static uint8_t          measure_R_out = 0;
uint8_t key_state = 0;
/* 扫频参数 */
static FreqResponse_t freq_response[FREQ_POINTS];
static uint8_t measurement_complete = 0;
static uint8_t current_point = 0;


// 内部函数声明
/* ============ 基本参数测量部分 ============ */
/* ===== 数据处理函数 ===== */
static float average_float_array(const float *arr, int N);
static void average_spectrumresult_array(SpectrumResult_t *res, SpectrumResult_t *avg_res, int N);
uint8_t Process_ADC_Data_F32(SpectrumResult_t* pRes1, SpectrumResult_t* pRes2, float* pRes3);
static void DemuxADCData(const uint16_t *src,
                  float *buf1,
                  float *buf2,
                  float *buf3,
                  uint16_t len);
static void CalcArrayMean(const float* buf, float *pRes);
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes);
static float CalculateRout(SpectrumResult_t* v_out_with_load, SpectrumResult_t* v_out_open_drain);
/* ===== 显示函数 ===== */
void LCD_Display_Title_Center(const char* title, uint16_t y_pos);
void Basic_Measurement_Page_Init(void);
void LCD_Display_Frequency(float frequency, uint16_t x, uint16_t y, uint16_t color);
void Basic_Measurement_Page_Update(void);
static void DisplayRout(float Rout);
static void Display_3dB_Frequency(float freq_3db);
static void Display_Measurement_Statistics(FreqResponse_t* freq_response_data, int len);


/* ============ 扫频测量部分 ============ */
static float Calculate_Log_Frequency(uint8_t point_index);
static void LCD_Display_Title(const char* title);
static void Display_Frequency_Response_Curve(void);
static void Measure_Single_Point(float frequency, FreqResponse_t* result);
static void Draw_Frequency_Response_Points(void);
static void Draw_Coordinate_System(void);
static void Display_Measurement_Info(void);
static void Update_Measurement_Progress(uint8_t current_point, uint8_t total_points);
static void Display_3dB_Frequency(float freq_3db);  // 显示-3dB截止频率
static void Display_Measurement_Statistics(FreqResponse_t* freq_response_data, int len);  // 显示测量统计信息
void plot_Frequency_Response_Point(FreqResponse_t point);  // 绘制单个频率响应数据点

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
 * @brief 基本测量和结果显示
 * 
 * 此函数用于显示 FF LCD 上。
 * 
 * @note 根据 Mode_Select 的值，专注于特定模式的显示。
 */
void Basic_Measurement(void)
{
    static float fs = 0;

    if(first_refresh1)
    {
        // 设置DDS频率为1000Hz
        AD9851_Set_Frequency(1000);
        
        // 启动ADC+DMA+TIM系统，配置到1000Hz对应的采样率
        Start_ADC_DMA_TIM_System(1000.0f);
        
        // 初始化屏幕显示
        Basic_Measurement_Page_Init();
        first_refresh1 = 0;
        first_refresh2 = 1;
    }

    // 数据采集和处理
    Process_ADC_Data_F32(&data_at_1k.adc_in_Result, &data_at_1k.adc_ac_out_Result, &data_at_1k.adc_dc_out_Result);

    // 数据更新和显示
    Basic_Measurement_Page_Update();

    if(measure_R_out)
    {
        SpectrumResult_t v_open_drain_out = data_at_1k.adc_ac_out_Result;
        SpectrumResult_t v_out ;
        // 开启继电器
        RELAY_ON;

        static uint8_t cnt;

        static SpectrumResult_t v_out_with_load[PROCESS_ARRAY_SIZE_WITH_R_L];
        // 延时等待稳定
        HAL_Delay(1000);

        for(uint8_t i = 0; i<PROCESS_ARRAY_SIZE_WITH_R_L; i++)
        {
            Process_ADC_Data_F32(&v_out_with_load[i].adc_in_Result, &v_out_with_load[i].adc_ac_out_Result, &v_out_with_load[i].adc_dc_out_Result);
        }

        average_spectrumresult_array(&v_out_with_load, &v_out, PROCESS_ARRAY_SIZE_WITH_R_L);
        

        float R_out = CalculateRout(&v_out, &v_open_drain_out);

        DisplayRout(R_out);

        measure_R_out = 0;
    }

}

/* ===== 数据处理函数 ===== */
/**
 * @brief  从ADC缓冲区处理数据，将定点数（1024点格式）转换为浮点数并进行处理。
 * @param  SpectrumResult_t* pRes1: adc_in
 * @param  SpectrumResult_t* pRes2: adc_ac_out
 * @param  float* pRes3      pRes3: adc_dc_out
 * @retval 1 表示已生成新结果，0 表示无新结果
 */
uint8_t Process_ADC_Data_F32(SpectrumResult_t* pRes1, SpectrumResult_t* pRes2, float* pRes3)
{  
    uint8_t new_data = 0;
    static float adc_in_buffer[FFT_SIZE];      // 通道1
    static float adc_ac_out_buffer[FFT_SIZE];  // 通道2
    static float adc_dc_out_buffer[FFT_SIZE];  // 通道3

    // 数据采集和分装：
    // 1. 从DMA缓存区搬运数据
    // 2. 将数据分装成到3个子数组
    // 3. 分别处理3个子数组中的数据
    if (ADC_BufferReadyFlag == BUFFER_READY_FLAG_FULL)
    {
        //  将数据分装成到3个子数组
        const uint16_t *src = (const uint16_t *)&adc_buffer[0];
        DemuxADCData(src, adc_in_buffer, adc_ac_out_buffer, adc_dc_out_buffer, FFT_SIZE);
        ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;

        // 获取当前采样率
        extern float g_current_Fs;

        // 分别处理3个子数组中的数据
        ProcessSampleData_F32(adc_in_buffer, pRes1, g_current_Fs);
        ProcessSampleData_F32(adc_ac_out_buffer, pRes2, g_current_Fs);
        CalcArrayMean(adc_dc_out_buffer, pRes3);

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
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes, float fs)
{
    const uint8_t process_interval = PROCESS_INTERVAL;   // 每隔N个循环执行一次FFT
    static uint8_t process_counter  = 0;

    if (++process_counter >= process_interval)
    {
        process_counter = 0;
        spectrum_analysis(sampleData, FFT_SIZE, fs, pRes);
    }
}

/**
 * @brief 计算输出阻抗
 * @param v_out_with_load 带负载时的输出电压
 * @param v_out_open_drain 开路时的输出电压
 * @retval 输出阻抗值（kΩ）
 */
static float CalculateRout(SpectrumResult_t* v_out_with_load, SpectrumResult_t* v_out_open_drain)
{
    if(v_out_with_load->amplitude > 0.001f)  // 避免除零
    {
        return (v_out_open_drain->amplitude - v_out_with_load->amplitude) / v_out_with_load->amplitude * R_L;
    }
    return 0.0f;  // 如果分母太小，返回0
}

/**
 * @brief 输入阻抗计算函数
 */
static float CalculateRin(SignalAnalysisResult_t* res)
{
    float Rin = (V_s - res->adc_in_Result.amplitude/V_Rs_Gain)/(res->adc_in_Result.amplitude/V_Rs_Gain) * R_s;
    return Rin;  // kΩ
}

/* ===== 显示函数 ===== */
/**
 * @brief 在LCD屏幕顶部中心显示标题
 * @param title 要显示的标题字符串
 * @param y_pos 标题在Y轴上的位置（可选，默认建议10-20）
 * @retval None
 */
void LCD_Display_Title_Center(const char* title, uint16_t y_pos)
{
    // 屏幕尺寸定义
    #define SCREEN_WIDTH  240
    #define SCREEN_CENTER_X (SCREEN_WIDTH / 2)  // 120
    
    // 字体参数
    #define FONT_WIDTH  8   // 16x8字体的字符宽度
    #define FONT_HEIGHT 16  // 16x8字体的字符高度
    
    // 计算字符串长度
    uint16_t str_len = strlen(title);
    
    // 计算字符串总像素宽度
    uint16_t total_width = str_len * FONT_WIDTH;
    
    // 计算起始X坐标(确保字符串中心对齐到屏幕中心)
    uint16_t start_x = SCREEN_CENTER_X - (total_width / 2);
    
    // 边界检查，确保字符串不会超出屏幕边界
    if(start_x > SCREEN_WIDTH) start_x = 0;  // 防止下溢
    if(start_x + total_width > SCREEN_WIDTH) start_x = SCREEN_WIDTH - total_width;
    
    // 设置显示颜色
    POINT_COLOR = WHITE;    // 白色字体
    BACK_COLOR = BLACK;     // 黑色背景
    
    // 显示标题字符串
    LCD_ShowString(start_x, y_pos, total_width, FONT_HEIGHT, 16, (uint8_t*)title);
}

/**
 * @brief 初始化屏幕显示
 */
void Basic_Measurement_Page_Init(void)
{
    LCD_Clear(WHITE);

    // 显示标题
    POINT_COLOR = TITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_Display_Title_Center("Basic Measurement", 10);

    // ========== 输入信号部分 ==========
    // 显示副标题
    POINT_COLOR = SUBTITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(10, 35, 200, 16, 16, (uint8_t*)"Input Signal");

    // 显示数据标签
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(15, 55, 80, 12, 12, (uint8_t*)"Vin: 1V");
    LCD_ShowString(15, 70, 80, 12, 12, (uint8_t*)"Vs:");
    LCD_ShowString(15, 85, 80, 12, 12, (uint8_t*)"fs:");
    LCD_ShowString(15, 100, 80, 12, 12, (uint8_t*)"Rin:");

    // ========== 输出信号(AC)部分 ==========
    // 显示副标题
    POINT_COLOR = SUBTITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(10, 125, 200, 16, 16, (uint8_t*)"Output Signal(AC)");

    // 显示数据标签
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(15, 145, 100, 12, 12, (uint8_t*)"Vout(Open):");
    LCD_ShowString(15, 160, 100, 12, 12, (uint8_t*)"Vout(RL):");
    LCD_ShowString(15, 175, 100, 12, 12, (uint8_t*)"fs:");
    LCD_ShowString(15, 190, 100, 12, 12, (uint8_t*)"Rout:");

    // ========== 输出信号(DC)部分 ==========
    // 显示副标题
    POINT_COLOR = SUBTITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(10, 215, 200, 16, 16, (uint8_t*)"Output Signal(DC)");

    // 显示数据标签
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(15, 235, 100, 12, 12, (uint8_t*)"Vout(DC):");

    // ========== 绘制分割线 ==========
    POINT_COLOR = GRAY;
    LCD_DrawLine(10, 120, 230, 120);    // 输入和AC输出之间的分割线
    LCD_DrawLine(10, 210, 230, 210);    // AC输出和DC输出之间的分割线
}

/**
 * @brief 在LCD上显示频率值（自动单位转换，至少4位有效数字）
 * @param frequency 频率值（单位：Hz）
 * @param x X坐标
 * @param y Y坐标
 * @param color 显示颜色
 * @retval None
 */
void LCD_Display_Frequency(float frequency, uint16_t x, uint16_t y, uint16_t color)
{
    char freq_buffer[20];
    
    // 设置显示颜色
    POINT_COLOR = color;
    BACK_COLOR = WHITE;
    
    // 根据频率大小自动选择合适的单位和格式
    if(frequency >= 1000000.0f)  // MHz级别
    {
        float freq_mhz = frequency / 1000000.0f;
        if(freq_mhz >= 10.0f)
            sprintf(freq_buffer, "%.2f MHz", freq_mhz);  // 10.00 MHz
        else
            sprintf(freq_buffer, "%.3f MHz", freq_mhz);  // 1.000 MHz
    }
    else if(frequency >= 10000.0f)  // 大于10000Hz转换为kHz
    {
        float freq_khz = frequency / 1000.0f;
        if(freq_khz >= 100.0f)
            sprintf(freq_buffer, "%.2f kHz", freq_khz);  // 100.00 kHz
        else if(freq_khz >= 10.0f)
            sprintf(freq_buffer, "%.3f kHz", freq_khz);  // 10.000 kHz
    }
    else  // 小于10000Hz，都使用Hz单位，保留2位小数
    {
        sprintf(freq_buffer, "%.2f Hz", frequency);      // 1234.50 Hz
    }
    
    // 显示频率字符串
    LCD_ShowString(x, y, 120, 12, 12, (uint8_t*)freq_buffer);
}

/**
 * @brief 更新基本测量页面显示
 */
void Basic_Measurement_Page_Update(void)  
{
    // 设置颜色
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    
    /* 输入部分 */
    // 电压显示 (Vs:)
    LCD_Fill(38, 70, 230, 82, WHITE);
    sprintf(dispBuff, "%.3f V", data_at_1k.adc_in_Result.amplitude); 
    LCD_ShowString(60, 70, 120, 12, 12, (uint8_t*)dispBuff);

    // 频率显示 (fs:)
    LCD_Fill(38, 85, 230, 97, WHITE);
    LCD_Display_Frequency(data_at_1k.adc_in_Result.frequency, 60, 85, DATA_COLOR);

    // 输入阻抗显示 (Rin:)
    LCD_Fill(60, 100, 230, 112, WHITE);
    float R_in = CalculateRin(&data_at_1k);
    sprintf(dispBuff, "%.3f kΩ", R_in); 
    LCD_ShowString(60, 100, 120, 12, 12, (uint8_t*)dispBuff);

    /* 输出交流部分 */
    // 开路电压显示 (Vout(Open):)
    LCD_Fill(130, 145, 230, 157, WHITE);
    sprintf(dispBuff, "%.3f V", data_at_1k.adc_ac_out_Result.amplitude);
    LCD_ShowString(130, 145, 120, 12, 12, (uint8_t*)dispBuff);

    // 频率显示 (fs:)
    LCD_Fill(38, 175, 230, 187, WHITE);
    LCD_Display_Frequency(data_at_1k.adc_ac_out_Result.frequency, 60, 175, DATA_COLOR);

    /* 输出直流部分 */
    // DC电压显示 (Vout(DC):)
    LCD_Fill(90, 235, 230, 247, WHITE);
    sprintf(dispBuff, "%.3f V", data_at_1k.adc_dc_out_Result);  // 修正：去掉.amplitude
    LCD_ShowString(90, 235, 120, 12, 12, (uint8_t*)dispBuff);
}

/**
 * @brief 输出阻抗显示函数
 */
static void DisplayRout(float Rout)
{
    // 设置显示颜色
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    
    // 清除输出阻抗显示区域
    LCD_Fill(60, 190, 230, 202, WHITE);
    
    // 格式化并显示输出阻抗
    sprintf(dispBuff, "%.3f kΩ", Rout);
    LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)dispBuff);
}

/**
 * @brief 显示-3dB截止频率
 * @param freq_3db -3dB截止频率值（Hz）
 * @retval None
 */
static void Display_3dB_Frequency(float freq_3db)
{
    char freq_display[30];
    
    // 设置显示位置（在图表下方）
    uint16_t display_x = 10;
    uint16_t display_y = 200;
    
    // 设置显示颜色
    POINT_COLOR = YELLOW;  // 黄色高亮显示
    BACK_COLOR = WHITE;    // 白色背景

    // 检查频率是否有效
    if(freq_3db > 0)
    {
        // 格式化频率显示
        if(freq_3db >= 1000000.0f)  // MHz级别
        {
            sprintf(freq_display, "-3dB Freq: %.2f MHz", freq_3db / 1000000.0f);
        }
        else if(freq_3db >= 1000.0f)  // kHz级别
        {
            sprintf(freq_display, "-3dB Freq: %.2f kHz", freq_3db / 1000.0f);
        }
        else  // Hz级别
        {
            sprintf(freq_display, "-3dB Freq: %.1f Hz", freq_3db);
        }
    }
    else
    {
        // 未找到-3dB频点
        sprintf(freq_display, "-3dB Freq: Not Found");
    }
    
    // 清除显示区域
    LCD_Fill(display_x, display_y, 230, display_y + 16, WHITE);
    
    // 显示-3dB频率
    LCD_ShowString(display_x, display_y, 220, 16, 16, (uint8_t*)freq_display);
    
    // 如果找到了有效的-3dB频点，在图上标记
    if(freq_3db > 0)
    {
        // 定义绘图区域（与坐标系保持一致）
        uint16_t plot_x_start = 30;
        uint16_t plot_x_end = 210;
        uint16_t plot_y_start = 40;
        uint16_t plot_y_end = 180;
        
        // 计算对数范围
        float log_min = log10f(FREQ_START);     // log10(100) = 2.0
        float log_max = log10f(FREQ_STOP);      // log10(200000) = 5.301
        
        // 计算-3dB频点在图上的X坐标
        if(freq_3db >= FREQ_START && freq_3db <= FREQ_STOP)
        {
            float log_freq_3db = log10f(freq_3db);
            uint16_t x_3db = plot_x_start + (log_freq_3db - log_min) * (plot_x_end - plot_x_start) / (log_max - log_min);
            
            // 在图上绘制-3dB垂直标记线
            POINT_COLOR = YELLOW;
            for(uint16_t y = plot_y_start; y <= plot_y_end; y += 5)
            {
                // 绘制虚线
                LCD_Fast_DrawPoint(x_3db, y, YELLOW);
                LCD_Fast_DrawPoint(x_3db, y+1, YELLOW);
            }
            
            // 在-3dB线上添加标记
            uint16_t y_3db = plot_y_end - (3.0f + 30.0f) * (plot_y_end - plot_y_start) / 60.0f;  // -3dB对应的Y坐标
            
            // 绘制特殊标记（小圆圈）
            POINT_COLOR = YELLOW;
            LCD_DrawLine(x_3db - 3, y_3db, x_3db + 3, y_3db);     // 水平线
            LCD_DrawLine(x_3db, y_3db - 3, x_3db, y_3db + 3);     // 垂直线
            
            // 绘制圆形标记
            for(int i = -2; i <= 2; i++)
            {
                for(int j = -2; j <= 2; j++)
                {
                    if(i*i + j*j <= 4)  // 圆形范围
                    {
                        LCD_Fast_DrawPoint(x_3db + i, y_3db + j, YELLOW);
                    }
                }
            }
        }
    }
}

/**
 * @brief 显示测量统计信息
 * @param freq_response_data 频率响应数据数组
 * @param len 数组长度
 * @retval None
 */
static void Display_Measurement_Statistics(FreqResponse_t* freq_response_data, int len)
{
    char info_display[40];
    
    // 设置显示位置
    uint16_t display_x = 10;
    uint16_t display_y = 220;
    
    // 设置显示颜色
    POINT_COLOR = CYAN;
    BACK_COLOR = BLACK;
    
    // 找到最大增益和对应频率
    float max_gain = -100.0f;
    float max_gain_freq = 0.0f;
    float min_gain = 100.0f;
    
    for(int i = 0; i < len; i++)
    {
        if(freq_response_data[i].frequency > 0)  // 有效数据
        {
            if(freq_response_data[i].gain_db > max_gain)
            {
                max_gain = freq_response_data[i].gain_db;
                max_gain_freq = freq_response_data[i].frequency;
            }
            if(freq_response_data[i].gain_db < min_gain)
            {
                min_gain = freq_response_data[i].gain_db;
            }
        }
    }
    
    // 清除显示区域
    LCD_Fill(display_x, display_y, 230, display_y + 32, WHITE);
    
    // 显示峰值增益信息
    if(max_gain_freq >= 1000.0f)
    {
        sprintf(info_display, "Peak: %.1fdB @ %.1fkHz", max_gain, max_gain_freq/1000.0f);
    }
    else
    {
        sprintf(info_display, "Peak: %.1fdB @ %.1fHz", max_gain, max_gain_freq);
    }
    LCD_ShowString(display_x, display_y, 220, 16, 12, (uint8_t*)info_display);
    
    // 显示动态范围
    sprintf(info_display, "Range: %.1f ~ %.1f dB", min_gain, max_gain);
    LCD_ShowString(display_x, display_y + 16, 220, 16, 12, (uint8_t*)info_display);
}


/* ============ 扫频测量部分 ============ */

/**
 * @brief 自动频幅特性测量主函数
 */
void Auto_Frequency_Response_Measurement(void)
{
    if(first_refresh2)
    {
        // 初始化页面显示
        Sweep_Freq_Response_Page_Init();
        first_refresh2 = 0;
        measurement_complete = 0;
        current_point = 0;
    }

    // 如果测量还未完成，继续扫频测量
    if (!measurement_complete && current_point < FREQ_POINTS)
    {
        // 计算当前频率点
        float input_freq = Calculate_Log_Frequency(current_point);
        
        // 配置DDS和定时器
        Set_DDS_Frequency(input_freq);
        
        // 启动ADC+DMA+TIM系统到新频率
        Start_ADC_DMA_TIM_System(input_freq);
        
        // 测量当前频率点
        Measure_Single_Point(input_freq, &freq_response[current_point]);
        
        // 绘制当前测量点
        plot_Frequency_Response_Point(freq_response[current_point]);
        
        // 更新进度显示
        Update_Measurement_Progress(current_point + 1, FREQ_POINTS);
        
        // 移动到下一个频率点
        current_point++;
        
        // 检查是否完成所有测量
        if (current_point >= FREQ_POINTS)
        {
            measurement_complete = 1;
            
            // 找出-3dB频点并显示
            float freq_3db = Find_3dB_Frequency(freq_response, FREQ_POINTS);
            Display_3dB_Frequency(freq_3db);
            
            // 显示测量统计信息
            Display_Measurement_Statistics(freq_response, FREQ_POINTS);
        }
    }
}

/**
 * @brief 测量单个频率点的响应
 * @param frequency 测量频率
 * @param result 测量结果存储
 */
static void Measure_Single_Point(float frequency, FreqResponse_t* result)
{
    static SignalAnalysisResult_t single_result;
    
    // 等待新的ADC数据
    uint8_t data_ready = 0;
    uint32_t timeout = 0;
    while (!data_ready && timeout < 1000)
    {
        data_ready = Process_ADC_Data_F32(&single_result.adc_in_Result, 
                                         &single_result.adc_ac_out_Result, 
                                         &single_result.adc_dc_out_Result);
        if (!data_ready)
        {
            HAL_Delay(10);
            timeout += 10;
        }
    }
    
    if (data_ready)
    {
        // 存储测量结果
        result->frequency = frequency;
        result->input_amp = single_result.adc_in_Result.amplitude;
        result->output_amp = single_result.adc_ac_out_Result.amplitude;
        
        // 计算增益(dB)
        if (result->input_amp > 0.001f)
        {
            result->gain_db = 20.0f * log10f(result->output_amp / result->input_amp);
        }
        else
        {
            result->gain_db = -100.0f;  // 极小值
        }
        
        // 相位计算(简化处理)
        result->phase_deg = 0.0f;
    }
    else
    {
        // 超时情况下的默认值
        result->frequency = frequency;
        result->input_amp = 0.0f;
        result->output_amp = 0.0f;
        result->gain_db = -100.0f;
        result->phase_deg = 0.0f;
    }
}

/**
 * @brief 更新测量进度显示
 * @param current_point 当前测量点
 * @param total_points 总测量点数
 */
static void Update_Measurement_Progress(uint8_t current_point, uint8_t total_points)
{
    char progress_text[30];
    sprintf(progress_text, "Progress: %d/%d", current_point, total_points);
    
    // 设置显示位置和颜色
    POINT_COLOR = BLUE;
    BACK_COLOR = WHITE;
    
    // 清除进度显示区域
    LCD_Fill(10, 25, 200, 35, WHITE);
    
    // 显示进度信息
    LCD_ShowString(10, 25, 180, 12, 12, (uint8_t*)progress_text);
}

/**
 * @brief 绘制完整的坐标系统
 */
static void Draw_Coordinate_System(void)
{
    // 定义绘图区域
    uint16_t plot_x_start = 30;
    uint16_t plot_x_end = 210;
    uint16_t plot_y_start = 40;
    uint16_t plot_y_end = 180;
    
    // 设置绘图颜色
    POINT_COLOR = BLACK;
    
    // 绘制坐标轴
    LCD_DrawLine(plot_x_start, plot_y_end, plot_x_end, plot_y_end);    // X轴
    LCD_DrawLine(plot_x_start, plot_y_start, plot_x_start, plot_y_end); // Y轴
    
    // 绘制X轴刻度和标签(对数刻度)
    float log_min = log10f(FREQ_START);     // log10(100) = 2.0
    float log_max = log10f(FREQ_STOP);      // log10(200000) = 5.301
    
    // 主要刻度点: 100Hz, 1kHz, 10kHz, 100kHz
    float major_freqs[] = {100.0f, 1000.0f, 10000.0f, 100000.0f};
    char* major_labels[] = {"100", "1k", "10k", "100k"};
    
    for(int i = 0; i < 4; i++)
    {
        if(major_freqs[i] >= FREQ_START && major_freqs[i] <= FREQ_STOP)
        {
            float log_freq = log10f(major_freqs[i]);
            uint16_t x_pos = plot_x_start + (log_freq - log_min) * (plot_x_end - plot_x_start) / (log_max - log_min);
            
            // 绘制刻度线
            LCD_DrawLine(x_pos, plot_y_end, x_pos, plot_y_end - 5);
            
            // 绘制标签
            POINT_COLOR = BLACK;
            BACK_COLOR = WHITE;
            LCD_ShowString(x_pos - 10, plot_y_end + 5, 30, 12, 12, (uint8_t*)major_labels[i]);
        }
    }
    
    // 绘制Y轴刻度和标签(增益dB)
    int16_t gain_range[] = {-40, -20, 0, 20};
    char gain_labels[][5] = {"-40", "-20", "0", "+20"};
    
    for(int i = 0; i < 4; i++)
    {
        // 假设增益范围为-50dB到+30dB
        uint16_t y_pos = plot_y_end - (gain_range[i] + 50) * (plot_y_end - plot_y_start) / 80;
        
        if(y_pos >= plot_y_start && y_pos <= plot_y_end)
        {
            // 绘制刻度线
            LCD_DrawLine(plot_x_start, y_pos, plot_x_start + 5, y_pos);
            
            // 绘制标签
            POINT_COLOR = BLACK;
            BACK_COLOR = WHITE;
            LCD_ShowString(5, y_pos - 6, 25, 12, 12, (uint8_t*)gain_labels[i]);
        }
    }
    
    // 绘制坐标轴标题
    POINT_COLOR = BLACK;
    BACK_COLOR = WHITE;
    LCD_ShowString(plot_x_start + 60, plot_y_end + 20, 80, 12, 12, (uint8_t*)"Frequency");
    
    // 旋转文字较复杂，先用简单的Y轴标题
    LCD_ShowString(5, plot_y_start + 50, 20, 12, 12, (uint8_t*)"dB");
}

/**
 * @brief 绘制单个频率响应数据点
 * @param point 频率响应数据点
 */
void plot_Frequency_Response_Point(FreqResponse_t point)
{
    // 定义绘图区域(与坐标系保持一致)
    uint16_t plot_x_start = 30;
    uint16_t plot_x_end = 210;
    uint16_t plot_y_start = 40;
    uint16_t plot_y_end = 180;
    
    // 计算对数频率坐标
    float log_min = log10f(FREQ_START);
    float log_max = log10f(FREQ_STOP);
    float log_freq = log10f(point.frequency);
    
    // 计算X坐标
    uint16_t x_pos = plot_x_start + (log_freq - log_min) * (plot_x_end - plot_x_start) / (log_max - log_min);
    
    // 计算Y坐标(增益范围-50dB到+30dB)
    float gain_min = -50.0f;
    float gain_max = 30.0f;
    uint16_t y_pos = plot_y_end - (point.gain_db - gain_min) * (plot_y_end - plot_y_start) / (gain_max - gain_min);
    
    // 边界检查
    if(x_pos >= plot_x_start && x_pos <= plot_x_end && 
       y_pos >= plot_y_start && y_pos <= plot_y_end)
    {
        // 设置绘图颜色
        POINT_COLOR = RED;
        
        // 绘制数据点(十字标记)
        LCD_DrawLine(x_pos - 2, y_pos, x_pos + 2, y_pos);    // 水平线
        LCD_DrawLine(x_pos, y_pos - 2, x_pos, y_pos + 2);    // 垂直线
        
        // 绘制小圆点
        LCD_DrawPoint(x_pos, y_pos);
        LCD_DrawPoint(x_pos + 1, y_pos);
        LCD_DrawPoint(x_pos - 1, y_pos);
        LCD_DrawPoint(x_pos, y_pos + 1);
        LCD_DrawPoint(x_pos, y_pos - 1);
    }
}

/* ===== 系统控制函数 ===== */
/**
 * @brief 启动ADC+DMA+TIM系统
 * @param frequency 目标测量频率（Hz）
 * @retval None
 */
static void Start_ADC_DMA_TIM_System(float frequency)
{
    // 1. 停止可能正在运行的ADC和定时器
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    
    // 2. 配置定时器到目标频率对应的采样率
    Tim2_Config_AutoFs(frequency);
    
    // 3. 启动定时器
    HAL_TIM_Base_Start(&htim2);
    
    // 4. 启动ADC+DMA
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief 停止ADC+DMA+TIM系统
 * @retval None
 */
static void Stop_ADC_DMA_TIM_System(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
}


