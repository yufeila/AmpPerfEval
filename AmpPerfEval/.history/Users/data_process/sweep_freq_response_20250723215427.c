#include "sweep_freq_response.h"
#include "usart.h"     // 用于串口调试
#include <string.h>    // 用于strlen
#include <stdio.h>     // 用于sprintf
#include <stdbool.h>   // 用于bool类型

#include "./data_process/sweep_freq_response.h"
#include <math.h>

//外部变量声明
extern __IO  uint16_t adc_buffer[BUF_SIZE];
extern volatile uint8_t ADC_BufferReadyFlag;
extern uint8_t basic_measurement_flag; 
extern uint8_t sweep_freq_response_flag;
extern uint8_t measure_r_out_flag;
extern uint8_t fault_detection_flag;
extern uint8_t current_system_state;
extern DMA_HandleTypeDef hdma_adc1;

// 全局变量声明
/* 基本参数显示 */
static SignalAnalysisResult_t data_at_1k;
uint8_t key_state = 0;
/* 扫频参数 */
static FreqResponse_t freq_response[FREQ_POINTS];
static uint8_t measurement_complete = 0;    // 扫频测量完成标志：0=进行中，1=已完成，避免在while循环中进行大循环扫频
static uint8_t current_point = 0;           // 当前测量频率点索引：0到FREQ_POINTS-1

/* 输出阻抗测量相关 */
static uint8_t r_out_measured = 0;          // R_out测量完成标志：0=未测量，1=已测量
static float r_out_value = 0.0f;            // 存储测量的R_out值
static SpectrumResult_t last_v_open_drain_out;  // 上一次开漏输出测量结果
static SpectrumResult_t v_out ;  // 当前带负载输出测量结果    


// 内部函数声明
/* ============ 基本参数测量部分 ============ */
/* ===== 数据处理函数 ===== */
static float average_float_array(const float *arr, int N);
static void average_spectrumresult_array(SpectrumResult_t *res, SpectrumResult_t *avg_res, int N);
uint8_t Process_ADC_Data_F32(SpectrumResult_t* pRes1, SpectrumResult_t* pRes2, float* pRes3, float fs);
static void DemuxADCData(const uint16_t *src,
                  float *buf1,
                  float *buf2,
                  float *buf3,
                  uint16_t len);
static void CalcArrayMean(const float* buf, float *pRes);
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes, float fs);
static float CalculateRout(SpectrumResult_t* v_out_with_load, SpectrumResult_t* v_out_open_drain);
static float CalculateRin(SignalAnalysisResult_t* res);
static void Reset_Rout_Measurement(void);
static void Force_Stop_All_Operations(void);
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
static float Find_3dB_Frequency(FreqResponse_t* freq_response, int len);
static void Measure_Single_Point(float frequency, FreqResponse_t* result, float fs);
static void Draw_Coordinate_System(void);
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
    static uint8_t first_refresh = 1;  // 控制基本测量页面初始刷新
    static float current_fs = 0.0f;   // 当前采样率

    // 检测是否从其他模式切换回来，如果是则重置R_out测量状态
    if (basic_measurement_flag)
    {
        // 强制停止所有运行中的操作
        Force_Stop_All_Operations();
        
        first_refresh = 1;
        basic_measurement_flag = 0;
        // 从其他模式切换回基本测量模式时，重置R_out测量状态
        Reset_Rout_Measurement();
    }

    if(first_refresh)
    {
        // 完全清除屏幕内容，确保没有残留显示
        LCD_Clear(WHITE);
        HAL_Delay(50);  // 短暂延时，确保LCD清除完成
        
        // 确保AD9851退出休眠模式并正常工作
        AD9851_Exit_Power_Down();
        
        // 设置DDS频率为1000Hz
        AD9851_Set_Frequency(1000);
        
        // 只配置定时器，不启动ADC+DMA+TIM系统,配置采样率为200kHz
        current_fs = Tim2_Config_AutoFs(200000.0f);
        
        // 初始化屏幕显示
        Basic_Measurement_Page_Init();
        first_refresh = 0;
    }

    // 数据采集和处理，使用实际的采样率
    Process_ADC_Data_F32(&data_at_1k.adc_in_Result, &data_at_1k.adc_ac_out_Result, &data_at_1k.adc_dc_out_Result, current_fs);

    // 数据更新和显示
    Basic_Measurement_Page_Update();

    if(measure_r_out_flag)
    {
        // 锁存上一次测量结果
        last_v_open_drain_out = data_at_1k.adc_ac_out_Result;
        
        // 开启继电器
        RELAY_ON;

        static SignalAnalysisResult_t v_out_with_load[PROCESS_ARRAY_SIZE_WITH_R_L];
        // 延时等待稳定
        HAL_Delay(500);

        for(uint8_t i = 0; i<PROCESS_ARRAY_SIZE_WITH_R_L; i++)
        {
            Process_ADC_Data_F32(&v_out_with_load[i].adc_in_Result, &v_out_with_load[i].adc_ac_out_Result, &v_out_with_load[i].adc_dc_out_Result, current_fs);
        }
		
		SpectrumResult_t tmp[PROCESS_ARRAY_SIZE_WITH_R_L];
		
		for (uint8_t i = 0; i < PROCESS_ARRAY_SIZE_WITH_R_L; ++i) {
			tmp[i] = v_out_with_load[i].adc_ac_out_Result;
		}
		average_spectrumresult_array(tmp, &v_out, PROCESS_ARRAY_SIZE_WITH_R_L);

        // 计算并保存R_out结果
        r_out_value = CalculateRout(&v_out, &last_v_open_drain_out);
        r_out_measured = 1;  // 标记已测量完成

        measure_r_out_flag = 0;  // 清除测量触发标志位
        
        // 关闭继电器
       RELAY_OFF;
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
uint8_t Process_ADC_Data_F32(SpectrumResult_t* pRes1, SpectrumResult_t* pRes2, float* pRes3, float fs)
{  
    static float adc_in_buffer[FFT_SIZE];      // 通道1
    static float adc_ac_out_buffer[FFT_SIZE];  // 通道2
    static float adc_dc_out_buffer[FFT_SIZE];  // 通道3

    // 1. 启动本轮ADC+DMA+TIM采集（彻底修复启动时序）
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    
    // 先启动ADC+DMA，确保准备就绪
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
    
    // 清除可能的DMA标志位
    __HAL_DMA_CLEAR_FLAG(&hdma_adc1, DMA_FLAG_TCIF0_4);
    
    // 采用方案A：设置URS位，确保只有真正的溢出才产生TRGO
    htim2.Instance->CR1 |= TIM_CR1_URS;  // 只在overflow触发Update，不在CEN或UG时触发
    
    // 重置计数器到0，确保从完整周期开始
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    
    // 启动定时器，第一次TRGO将在完整周期后发生
    HAL_TIM_Base_Start(&htim2);
    
    // 2. 等待本轮数据采集完成
    uint32_t timeout = 0;
    while (ADC_BufferReadyFlag != BUFFER_READY_FLAG_FULL && timeout < 1000)
    {
        HAL_Delay(1);  // 短暂延时，避免CPU占用过高
        timeout++;
    }
    
    // 3. 检查是否超时
    if (timeout >= 1000)
    {
        // 超时处理：停止ADC+TIM，返回无新数据
        HAL_ADC_Stop_DMA(&hadc1);
        HAL_TIM_Base_Stop(&htim2);
        return 0;
    }
    
    // 4. 停止当前轮次的ADC+TIM（为下次启动做准备）
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    
    // 清除URS位，恢复默认行为
    htim2.Instance->CR1 &= ~TIM_CR1_URS;
    
    // 5. 处理本轮采集的数据
    const uint16_t *src = (const uint16_t *)&adc_buffer[0];
    DemuxADCData(src, adc_in_buffer, adc_ac_out_buffer, adc_dc_out_buffer, FFT_SIZE);
    
//    // *** 调试输出：验证通道对齐 ***
//    // 采样前10个原始ADC值和分离后的电压值进行验证
//    if (current_system_state == SWEEP_FREQ_RESPONSE_STATE) {
//        printf("Raw ADC[0-8]: %d,%d,%d | %d,%d,%d | %d,%d,%d\r\n",
//               adc_buffer[0], adc_buffer[1], adc_buffer[2],
//               adc_buffer[3], adc_buffer[4], adc_buffer[5], 
//               adc_buffer[6], adc_buffer[7], adc_buffer[8]);
//        
//        printf("After Demux[0-2]: Vin=%.3f, Vout=%.3f, Vdc=%.3f\r\n",
//               adc_in_buffer[0] + (adc_in_buffer[0] + adc_in_buffer[1] + adc_in_buffer[2])/3.0f, // 恢复DC后的值
//               adc_ac_out_buffer[0] + (adc_ac_out_buffer[0] + adc_ac_out_buffer[1] + adc_ac_out_buffer[2])/3.0f,
//               adc_dc_out_buffer[0]);
//    }
    
    // 6. 分别处理3个子数组中的数据
    ProcessSampleData_F32(adc_in_buffer, pRes1, fs);
    ProcessSampleData_F32(adc_ac_out_buffer, pRes2, fs);
    CalcArrayMean(adc_dc_out_buffer, pRes3);

    return 1;  // 返回有新数据
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
    // 第一步：转换为电压并计算平均值（DC分量）
    float sum1 = 0.0f, sum2 = 0.0f;
    
    for(uint16_t i = 0; i < len; i++)
    {
        float volt1 = (float)src[3*i] * ADC_LSB_VOLT;
        float volt2 = (float)src[3*i+1] * ADC_LSB_VOLT;
        
        buf1[i] = volt1;
        buf2[i] = volt2;
        buf3[i] = (float)src[3*i+2] * ADC_LSB_VOLT;  // DC通道保持原样
        
        sum1 += volt1;
        sum2 += volt2;
    }
    
    // 第二步：为AC通道去除DC分量（FFT需要以0为中心的信号）
    float dc1 = sum1 / (float)len;
    float dc2 = sum2 / (float)len;
    
    for(uint16_t i = 0; i < len; i++)
    {
        buf1[i] -= dc1;  // 去除DC分量，得到纯AC信号
        buf2[i] -= dc2;  // 去除DC分量，得到纯AC信号
        // buf3保持不变，因为它就是DC信号
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
 *  功能: 调用 spectrum_analysis() 
 *  描述: 处理采样数据并进行FFT分析，仅保留结果有效性检查
 *----------------------------------------------------*/
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes, float fs)
{
    
    // 1. 调用FFT分析
    spectrum_analysis(sampleData, FFT_SIZE, fs, pRes);

    // 2. 检查当前系统状态，决定是否启用有效性检查
    extern uint8_t current_system_state; // 声明外部系统状态变量
    
    if(current_system_state == BASIC_MEASUREMENT_STATE) {
        // 基本测量模式：启用结果有效性检查和滤波
        bool result_valid = true;
        
        // 检查1：幅度应该在合理范围内 (0.01V ~ 5V)
        if(pRes->amplitude < 0.01f || pRes->amplitude > 5.0f) {
            result_valid = false;
        }

        // 检查2：频率应该在合理范围内 (50Hz ~ 220kHz)
        if(pRes->frequency < 50.0f || pRes->frequency > 220000.0f) {
            result_valid = false;
        }
        

 
    } 
    else if(current_system_state == SWEEP_FREQ_RESPONSE_STATE) {
        // 扫频模式：完全禁用有效性检查，直接使用FFT原始结果
        // 每个频点只采样一次，做一次FFT，保持原始测量结果
        // 不做任何过滤或替换
    }
    // 其他状态：保持FFT原始结果
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
    float Rin = (V_s * 1e-3 * 0.5 - res->adc_in_Result.amplitude/V_Rs_Gain)/(res->adc_in_Result.amplitude/V_Rs_Gain) * R_s;
    return Rin;  // kΩ
}
/* ===== 硬件控制函数 ===== */
/**
 * @brief 重置输出阻抗测量状态
 * @retval None
 */
static void Reset_Rout_Measurement(void)
{
    r_out_measured = 0;     // 重置测量完成标志
    r_out_value = 0.0f;     // 重置测量值
    measure_r_out_flag = 0; // 重置测量触发标志
}

/**
 * @brief 强制停止所有运行中的操作，用于模式切换
 * @retval None
 */
static void Force_Stop_All_Operations(void)
{
    // 停止ADC+DMA+TIM系统
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    
    // 重置ADC缓冲区标志
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    
    // 关闭继电器（如果开启的话）
    RELAY_OFF;
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
    POINT_COLOR = BLACK;    // 黑色字体
    BACK_COLOR = WHITE;     // 白色背景

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

    // 增益测量部分
    POINT_COLOR = SUBTITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(10, 255, 200, 16, 16, (uint8_t*)"Gain Measurement");

    // 显示数据标签
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE; 
    LCD_ShowString(15, 275, 100, 12, 12, (uint8_t*)"Gain:");

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
    char dispBuff[48];  // 局部字符串缓冲区
    
    // 设置颜色
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    
    /* 输入部分 */
    // 电压显示 (Vs:)
    LCD_Fill(60, 70, 230, 82, WHITE);
    sprintf(dispBuff, "%.3f V", data_at_1k.adc_in_Result.amplitude); 
    LCD_ShowString(60, 70, 120, 12, 12, (uint8_t*)dispBuff);

    // 频率显示 (fs:)
    LCD_Fill(60, 85, 230, 97, WHITE);
    LCD_Display_Frequency(data_at_1k.adc_in_Result.frequency, 60, 85, DATA_COLOR);

    // 输入阻抗显示 (Rin:)
    LCD_Fill(60, 100, 230, 112, WHITE);
    float R_in = CalculateRin(&data_at_1k);
    sprintf(dispBuff, "%.3f kΩ", R_in); 
    LCD_ShowString(60, 100, 120, 12, 12, (uint8_t*)dispBuff);

    /* 输出交流部分 */
    // 开路电压显示 (Vout(Open):)
    if(!r_out_measured) // 如果未测量输出阻抗，显示开路电压
    {
        LCD_Fill(130, 145, 230, 157, WHITE);
        sprintf(dispBuff, "%.3f V", data_at_1k.adc_ac_out_Result.amplitude);
        LCD_ShowString(130, 145, 120, 12, 12, (uint8_t*)dispBuff);
    }
    else // 如果已测量输出阻抗，显示带负载电压
    {
        LCD_Fill(130, 145, 230, 157, WHITE);
        sprintf(dispBuff, "%.3f V", last_v_open_drain_out.amplitude);
        LCD_ShowString(130, 145, 120, 12, 12, (uint8_t*)dispBuff);
    }

    // 输出频率显示 (fs:) - 应该与输入频率相同
    LCD_Fill(60, 175, 230, 187, WHITE);
    LCD_Display_Frequency(data_at_1k.adc_ac_out_Result.frequency, 60, 175, DATA_COLOR);

    /* 输出直流部分 */
    // DC电压显示 (Vout(DC):)
    LCD_Fill(90, 235, 230, 247, WHITE);
    sprintf(dispBuff, "%.3f V", data_at_1k.adc_dc_out_Result);  // 修正：去掉.amplitude
    LCD_ShowString(90, 235, 120, 12, 12, (uint8_t*)dispBuff);
    
    /* 开路增益显示 */
    LCD_Fill(100, 160, 230, 172, WHITE);
    /* 计算开路增益 */
    float gain = (data_at_1k.adc_ac_out_Result.amplitude * 11 * 47/247) / (V_s * 1e-3 * 0.5 - data_at_1k.adc_in_Result.amplitude/V_Rs_Gain) ;
    sprintf(dispBuff, "%.3f V", gain);
    LCD_ShowString(90, 275, 120, 12, 12, (uint8_t*)dispBuff);

    /* 输出阻抗部分 - 根据测量状态显示 */
    LCD_Fill(60, 190, 230, 202, WHITE);

    if(r_out_measured)
    {
        // 显示测量结果
        sprintf(dispBuff, "%.3f kΩ", r_out_value);
        LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)dispBuff);
    }
    else
    {
        // 显示未测量状态
        LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)"--- kΩ");
    }
    
    /* 显示Vout(RL) - 修复缺失的显示 */
    if(r_out_measured)
    {
        // 如果已测量输出阻抗，显示负载电压（这里需要保存测量时的值）
        LCD_Fill(100, 160, 230, 172, WHITE);
        sprintf(dispBuff, "%.3f V", v_out.amplitude, "(measured)");
        LCD_ShowString(100, 160, 120, 12, 12, (uint8_t*)dispBuff);
    }
    else
    {
        // 未测量时显示占位符
        LCD_Fill(100, 160, 230, 172, WHITE);
        LCD_ShowString(100, 160, 120, 12, 12, (uint8_t*)"--- V");
    }
}

/**
 * @brief 输出阻抗显示函数
 */
static void DisplayRout(float Rout)
{
    char dispBuff[48];  // 局部字符串缓冲区
    
    // 设置显示颜色
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    
    // 清除输出阻抗显示区域
    LCD_Fill(60, 190, 230, 202, WHITE);
    
    // 格式化并显示输出阻抗
    sprintf(dispBuff, "%.3f kΩ", Rout);
    LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)dispBuff);
}


/* ============ 扫频测量部分 ============ */

/**
 * @brief 自动频幅特性测量主函数
 */
void Auto_Frequency_Response_Measurement(void)
{
    static uint8_t first_refresh = 1;  // 控制扫频测量页面初始刷新
    
    // 检测是否从其他模式切换到扫频模式
    if (sweep_freq_response_flag)
    {
        // 强制停止所有运行中的操作
        Force_Stop_All_Operations();
        
        first_refresh = 1;  // 触发重新初始化
        sweep_freq_response_flag = 0;  // 清除标志位
    }
    
    if(first_refresh)
    {
        // 完全清除屏幕内容，确保没有残留显示
        LCD_Clear(WHITE);
        HAL_Delay(50);  // 短暂延时，确保LCD清除完成
        
        // 确保AD9851退出休眠模式并正常工作
        AD9851_Exit_Power_Down();
        
        // 初始化页面显示
        LCD_Display_Title_Center("Frequency Response", 10);
        Draw_Coordinate_System();
        
        // *** 新增：扫频开始调试信息 ***
        printf("\r\n=== FREQUENCY SWEEP START ===\r\n");
        printf("Frequency Range: %.1f Hz ~ %.1f Hz\r\n", FREQ_START, FREQ_STOP);
        printf("Total Points: %d\r\n", FREQ_POINTS);
        printf("Format: [Point] Set_Freq, Measured_In/Out_Freq, Vin, Vout, Gain, Sampling_Freq\r\n");
        printf("===============================\r\n");
        
        first_refresh = 0;
        measurement_complete = 0;  // 重置测量完成标志
        current_point = 0;         // 重置测量点索引
    }

    // 如果测量还未完成，继续扫频测量
    if (!measurement_complete && current_point < FREQ_POINTS)
    {
        // 计算当前频率点
        float input_freq = Calculate_Log_Frequency(current_point);
        
        // 配置DDS到新频率
        AD9851_Set_Frequency((uint32_t)input_freq);
        
        // 只配置定时器，不启动硬件（让Process_ADC_Data_F32负责启动）
        float actual_fs = Tim2_Config_AutoFs(input_freq);
        
        // 测量当前频率点，传入实际采样率
        Measure_Single_Point(input_freq, &freq_response[current_point], actual_fs);
        
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
            
            // *** 新增：扫频完成调试总结 ***
            printf("\r\n=== FREQUENCY SWEEP COMPLETED ===\r\n");
            printf("-3dB Frequency: %.1f Hz\r\n", freq_3db);
            
            // 计算并显示最大/最小增益
            float max_gain = -100.0f, min_gain = 100.0f;
            float max_gain_freq = 0.0f, min_gain_freq = 0.0f;
            for(int i = 0; i < FREQ_POINTS; i++) {
                if(freq_response[i].gain_db > max_gain) {
                    max_gain = freq_response[i].gain_db;
                    max_gain_freq = freq_response[i].frequency;
                }
                if(freq_response[i].gain_db < min_gain) {
                    min_gain = freq_response[i].gain_db;
                    min_gain_freq = freq_response[i].frequency;
                }
            }
            printf("Max Gain: %.2f dB @ %.1f Hz\r\n", max_gain, max_gain_freq);
            printf("Min Gain: %.2f dB @ %.1f Hz\r\n", min_gain, min_gain_freq);
            printf("Gain Range: %.2f dB\r\n", max_gain - min_gain);
            printf("==================================\r\n\r\n");
        }
    }
}

/* ====== 数据处理部分 ====== */

/**
 * @brief 测量单个频率点的响应
 * @param frequency 测量频率
 * @param result 测量结果存储
 * @param fs 实际采样频率
 */
static void Measure_Single_Point(float frequency, FreqResponse_t* result, float fs)
{
    static SignalAnalysisResult_t single_result;
    
    // 等待新的ADC数据
    uint8_t data_ready = 0;
    uint32_t timeout = 0;
    while (!data_ready && timeout < 1000)
    {
        data_ready = Process_ADC_Data_F32(&single_result.adc_in_Result, 
                                         &single_result.adc_ac_out_Result, 
                                         &single_result.adc_dc_out_Result, fs);
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
        result->dc_out = single_result.adc_dc_out_Result;
        
        // 计算增益(dB)
        if (result->input_amp > 0.001f)
        {
            result->gain_db = 20.0f * log10f(result->output_amp / ( V_s *1e-3 - result->input_amp/V_Rs_Gain ));
        }
        else
        {
            result->gain_db = -100.0f;  // 极小值
        }
        
        // 相位计算(简化处理)
        result->phase_deg = 0.0f;
        
        // *** 新增：扫频测量调试输出 ***
        printf("SWEEP[%02d]: Set=%.1fHz, Measured=%.1fHz/%.1fHz, Vin=%.3fV, Vout=%.3fV, Vout_DC=%.3fV, Gain=%.2fdB, Fs=%.0fHz\r\n",
               current_point + 1,                                    // 测量点序号
               frequency,                                             // 设定频率
               single_result.adc_in_Result.frequency,                 // 输入测量频率
               single_result.adc_ac_out_Result.frequency,             // 输出测量频率
               result->input_amp,                                     // 输入幅度
               result->output_amp,                                    // 输出幅度
               result->dc_out,                                        // 输出直流电压
               result->gain_db,                                       // 增益(dB)
               fs,                                                    // 采样频率
               );                                      
    }
    else
    {
        // 超时情况下的默认值
        result->frequency = frequency;
        result->input_amp = 0.0f;
        result->output_amp = 0.0f;
        result->gain_db = -100.0f;
        result->phase_deg = 0.0f;
        
        // *** 新增：超时情况调试输出 ***
        printf("SWEEP[%02d]: TIMEOUT! Set=%.1fHz, Fs=%.0fHz (No valid data after %.1fs)\r\n",
               current_point + 1,
               frequency,
               fs,
               timeout / 1000.0f);
    }
}

/**
 * @brief 计算对数分布的频率点
 * @param point_index 频率点索引 (0 到 FREQ_POINTS-1)
 * @retval 对应的频率值 (Hz)
 */
static float Calculate_Log_Frequency(uint8_t point_index)
{
    if (point_index >= FREQ_POINTS) {
        return FREQ_STOP;
    }
    
    // 对数分布：freq = FREQ_START * (FREQ_STOP/FREQ_START)^(index/(FREQ_POINTS-1))
    float log_ratio = (float)point_index / (float)(FREQ_POINTS - 1);
    float frequency = FREQ_START * powf(FREQ_STOP / FREQ_START, log_ratio);
    
    return frequency;
}

/**
 * @brief 查找-3dB频点
 * @param freq_response 频率响应数据数组
 * @param len 数组长度
 * @retval -3dB频点，如果未找到返回0
 */
static float Find_3dB_Frequency(FreqResponse_t* freq_response, int len)
{
    if (len < 2) return 0.0f;
    
    // 找到最大增益
    float max_gain = -100.0f;
	uint8_t max_index = 0;
    for (int i = 0; i < len; i++) {
        if (freq_response[i].gain_db > max_gain) {
            max_gain = freq_response[i].gain_db;
			max_index = i;
        }
    }
    
    // 计算-3dB目标值
    float target_gain = max_gain - 3.0f;
    
    // 查找第一个低于-3dB点的频率
    for (int i = max_index; i < len; i++) {
        if (freq_response[i].gain_db < target_gain) {
            return freq_response[i].frequency;
        }
    }
    
    return 0.0f;  // 未找到
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
    int16_t gain_range[] = {-20, 0, 20, 40};
    char gain_labels[][5] = {"-20", "0", "20", "+40"};
    
    for(int i = 0; i < 4; i++)
    {
        // 假设增益范围为-30dB到+50dBdB
        uint16_t y_pos = plot_y_end - (gain_range[i] + 30) * (plot_y_end - plot_y_start) / 80;
        
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
    LCD_ShowString(30, plot_y_start, 20, 12, 12, (uint8_t*)"dB");
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
    
    // 计算Y坐标(增益范围-30dB到+50dB)
    float gain_min = -30.0f;
    float gain_max = 50.0f;
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


/* ===== 系统控制函数 ===== */
/**
 * @brief 启动ADC+DMA+TIM系统
 * @param frequency 目标测量频率（Hz）
 * @retval 实际采样频率
 */
float Start_ADC_DMA_TIM_System(float frequency)
{
    // 1. 停止可能正在运行的ADC和定时器
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    
    // 2. 配置定时器到目标频率对应的采样率，并获取实际采样率
    float actual_fs = Tim2_Config_AutoFs(frequency);
    
    // 3. 双重保护：确保采样率不会过低
    if (actual_fs < 10000.0f) {
        // 强制使用安全的采样率
        actual_fs = Tim2_Config_AutoFs(20000.0f);  // 重新配置为20kHz
    }
    
    // 4. 启动ADC+DMA（先启动ADC+DMA）
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
    
    // 清除可能的DMA标志位
    __HAL_DMA_CLEAR_FLAG(&hdma_adc1, DMA_FLAG_TCIF0_4);
    
    // 5. 启动定时器（后启动定时器触发）
    HAL_TIM_Base_Start(&htim2);
    
    // 6. 返回实际采样率
    return actual_fs;
}

/**
 * @brief 停止ADC+DMA+TIM系统
 * @retval None
 */
void Stop_ADC_DMA_TIM_System(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
}

// 故障检测函数
void Fault_Detection(void)
{
    static uint8_t first_refresh = 1;
    static uint8_t fault_detected = 0;
    static uint8_t fault_type = 0;

    if(fault_detection_flag)
    {
        first_refresh = 1;
        fault_detection_flag = 0;

        Force_Stop_All_Operations();
    }

    if(first_refresh)
    {
        first_refresh = 0;
        fault_detected = 0;
        measurement_complete = 0;  // 重置测量完成标志
        current_point = 0;         // 重置测量点索引
        LCD_Clear(WHITE);

        HAL_Delay(50);  // 短暂延时，确保LCD清除完成

        // 初始化页面显示
        LCD_Display_Title_Center("Fault Detection", 10);
    }

    // ---- 常规情况下：1kHz的数据扫描 ----
    current_fs = Tim2_Config_AutoFs(200000.0f);  // 配置常规情况下的采样率为200kHz
    
    Process_ADC_Data_F32(&data_at_1k.adc_in_Result, &data_at_1k.adc_ac_out_Result, &data_at_1k.adc_dc_out_Result, current_fs);

    // ---- 故障检测 ----
    // 1. 计算输入信号的幅值
    float input_amp = V_s * 1e-3 * 0.5 - data_at_1k.adc_in_Result.amplitude/V_Rs_Gain;

    // 2. 计算输出信号的幅值
    float output_amp = data_at_1k.adc_ac_out_Result.max_amp;

    // 3. 计算增益
    float gain = output_amp / input_amp;

    // 4. 计算输入阻抗
    float R_in = CalculateRin(&data_at_1k);

    // 5. 判断正常范围
    if ((fabs(gain - NORMAL_GAIN_AT_1K) <= NORMAL_GAIN_RANGE)
    &&  (fabs(R_in - NORMAL_R_IN_AT_1K) <= NORMAL_R_IN_RANGE)
    &&  (fabs(output_amp - NORMAL_VOUT_AC_AT_1K) <= NORMAL_VOUT_AC_RANGE))
    {
        // 正常
        LCD_ShowString(10, 40, 220, 16, 12, (uint8_t*)"Normal");
        fault_detected = 0;
    }
    else
    {
        fault_detected = 1;
    }

    if(fault_detected)
    {
        // ---- 开始扫频 ----
        // 如果测量还未完成，继续扫频测量
        if (!measurement_complete && current_point < FREQ_POINTS)
        {
            // 计算当前频率点
            float input_freq = Calculate_Log_Frequency(current_point);
            
            // 配置DDS到新频率
            AD9851_Set_Frequency((uint32_t)input_freq);
            
            // 只配置定时器，不启动硬件（让Process_ADC_Data_F32负责启动）
            float actual_fs = Tim2_Config_AutoFs(input_freq);
            
            // 测量当前频率点，传入实际采样率
            Measure_Single_Point(input_freq, &freq_response[current_point], actual_fs);
            
            // 绘制当前测量点
            plot_Frequency_Response_Point(freq_response[current_point]);
            
            // 更新进度显示
            Update_Measurement_Progress(current_point + 1, FREQ_POINTS);
            
            // 移动到下一个频率点
            current_point++;
        }

        if (current_point >= FREQ_POINTS)
        {
            measurement_complete = 1;

            // 判断是电阻异常，还是电容异常
            float gain_variance = Calculate_Gain_Variance(freq_response, FREQ_POINTS);
            if(gain_variance < R_Gain_Variance_Threshold)
            {
                // 电阻正常, 电容异常
                fault_type = C_FAULT_TYPE;
            }
            else if(gain_variance < C_Gain_Variance_Threshold)
            {
                // 电阻异常, 电容正常
                fault_type = R_FAULT_TYPE;
            }

            if(fault_type == R_FAULT_TYPE)
            {
                // 细化判断电阻故障类型, 使用正常情况下1khz时测得的直流输出电压判断
                if( fabs()   0.9f)

            }
            else if(fault_type == C_FAULT_TYPE)
            {

            }
        }
    }

}

/*
 * @brief 计算增益的方差
 * @param freq_response 频率响应数据数组
 * @param len 数组长度
 * @retval 增益的方差
 */
float Calculate_Gain_Variance(FreqResponse_t* freq_response, int len)
{
    float average_gain = 0.0f;
    float sum_gain = 0.0f;

    for(uint8_t i = 0; i < len; i++)
    {
        sum_gain += freq_response[i].gain_db;
    }

    average_gain = sum_gain / len;

    for(uint8_t i = 0; i < len; i++)
    {
        sgain += (freq_response[i].gain_db - average_gain) * (freq_response[i].gain_db - average_gain);
    }

    return sum_gain / len;
}








