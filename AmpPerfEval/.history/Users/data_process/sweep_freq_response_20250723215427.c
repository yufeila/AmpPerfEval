#include "sweep_freq_response.h"
#include "usart.h"     // ���ڴ��ڵ���
#include <string.h>    // ����strlen
#include <stdio.h>     // ����sprintf
#include <stdbool.h>   // ����bool����

#include "./data_process/sweep_freq_response.h"
#include <math.h>

//�ⲿ��������
extern __IO  uint16_t adc_buffer[BUF_SIZE];
extern volatile uint8_t ADC_BufferReadyFlag;
extern uint8_t basic_measurement_flag; 
extern uint8_t sweep_freq_response_flag;
extern uint8_t measure_r_out_flag;
extern uint8_t fault_detection_flag;
extern uint8_t current_system_state;
extern DMA_HandleTypeDef hdma_adc1;

// ȫ�ֱ�������
/* ����������ʾ */
static SignalAnalysisResult_t data_at_1k;
uint8_t key_state = 0;
/* ɨƵ���� */
static FreqResponse_t freq_response[FREQ_POINTS];
static uint8_t measurement_complete = 0;    // ɨƵ������ɱ�־��0=�����У�1=����ɣ�������whileѭ���н��д�ѭ��ɨƵ
static uint8_t current_point = 0;           // ��ǰ����Ƶ�ʵ�������0��FREQ_POINTS-1

/* ����迹������� */
static uint8_t r_out_measured = 0;          // R_out������ɱ�־��0=δ������1=�Ѳ���
static float r_out_value = 0.0f;            // �洢������R_outֵ
static SpectrumResult_t last_v_open_drain_out;  // ��һ�ο�©����������
static SpectrumResult_t v_out ;  // ��ǰ����������������    


// �ڲ���������
/* ============ ���������������� ============ */
/* ===== ���ݴ����� ===== */
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
/* ===== ��ʾ���� ===== */
void LCD_Display_Title_Center(const char* title, uint16_t y_pos);
void Basic_Measurement_Page_Init(void);
void LCD_Display_Frequency(float frequency, uint16_t x, uint16_t y, uint16_t color);
void Basic_Measurement_Page_Update(void);
static void DisplayRout(float Rout);
static void Display_3dB_Frequency(float freq_3db);
static void Display_Measurement_Statistics(FreqResponse_t* freq_response_data, int len);


/* ============ ɨƵ�������� ============ */
static float Calculate_Log_Frequency(uint8_t point_index);
static float Find_3dB_Frequency(FreqResponse_t* freq_response, int len);
static void Measure_Single_Point(float frequency, FreqResponse_t* result, float fs);
static void Draw_Coordinate_System(void);
static void Update_Measurement_Progress(uint8_t current_point, uint8_t total_points);
static void Display_3dB_Frequency(float freq_3db);  // ��ʾ-3dB��ֹƵ��
static void Display_Measurement_Statistics(FreqResponse_t* freq_response_data, int len);  // ��ʾ����ͳ����Ϣ
void plot_Frequency_Response_Point(FreqResponse_t point);  // ���Ƶ���Ƶ����Ӧ���ݵ�

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
 * @brief ���������ͽ����ʾ
 * 
 * �˺���������ʾ FF LCD �ϡ�
 * 
 * @note ���� Mode_Select ��ֵ��רע���ض�ģʽ����ʾ��
 */
void Basic_Measurement(void)
{
    static uint8_t first_refresh = 1;  // ���ƻ�������ҳ���ʼˢ��
    static float current_fs = 0.0f;   // ��ǰ������

    // ����Ƿ������ģʽ�л������������������R_out����״̬
    if (basic_measurement_flag)
    {
        // ǿ��ֹͣ���������еĲ���
        Force_Stop_All_Operations();
        
        first_refresh = 1;
        basic_measurement_flag = 0;
        // ������ģʽ�л��ػ�������ģʽʱ������R_out����״̬
        Reset_Rout_Measurement();
    }

    if(first_refresh)
    {
        // ��ȫ�����Ļ���ݣ�ȷ��û�в�����ʾ
        LCD_Clear(WHITE);
        HAL_Delay(50);  // ������ʱ��ȷ��LCD������
        
        // ȷ��AD9851�˳�����ģʽ����������
        AD9851_Exit_Power_Down();
        
        // ����DDSƵ��Ϊ1000Hz
        AD9851_Set_Frequency(1000);
        
        // ֻ���ö�ʱ����������ADC+DMA+TIMϵͳ,���ò�����Ϊ200kHz
        current_fs = Tim2_Config_AutoFs(200000.0f);
        
        // ��ʼ����Ļ��ʾ
        Basic_Measurement_Page_Init();
        first_refresh = 0;
    }

    // ���ݲɼ��ʹ���ʹ��ʵ�ʵĲ�����
    Process_ADC_Data_F32(&data_at_1k.adc_in_Result, &data_at_1k.adc_ac_out_Result, &data_at_1k.adc_dc_out_Result, current_fs);

    // ���ݸ��º���ʾ
    Basic_Measurement_Page_Update();

    if(measure_r_out_flag)
    {
        // ������һ�β������
        last_v_open_drain_out = data_at_1k.adc_ac_out_Result;
        
        // �����̵���
        RELAY_ON;

        static SignalAnalysisResult_t v_out_with_load[PROCESS_ARRAY_SIZE_WITH_R_L];
        // ��ʱ�ȴ��ȶ�
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

        // ���㲢����R_out���
        r_out_value = CalculateRout(&v_out, &last_v_open_drain_out);
        r_out_measured = 1;  // ����Ѳ������

        measure_r_out_flag = 0;  // �������������־λ
        
        // �رռ̵���
       RELAY_OFF;
    }

}

/* ===== ���ݴ����� ===== */
/**
 * @brief  ��ADC�������������ݣ�����������1024���ʽ��ת��Ϊ�����������д���
 * @param  SpectrumResult_t* pRes1: adc_in
 * @param  SpectrumResult_t* pRes2: adc_ac_out
 * @param  float* pRes3      pRes3: adc_dc_out
 * @retval 1 ��ʾ�������½����0 ��ʾ���½��
 */
uint8_t Process_ADC_Data_F32(SpectrumResult_t* pRes1, SpectrumResult_t* pRes2, float* pRes3, float fs)
{  
    static float adc_in_buffer[FFT_SIZE];      // ͨ��1
    static float adc_ac_out_buffer[FFT_SIZE];  // ͨ��2
    static float adc_dc_out_buffer[FFT_SIZE];  // ͨ��3

    // 1. ��������ADC+DMA+TIM�ɼ��������޸�����ʱ��
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    
    // ������ADC+DMA��ȷ��׼������
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
    
    // ������ܵ�DMA��־λ
    __HAL_DMA_CLEAR_FLAG(&hdma_adc1, DMA_FLAG_TCIF0_4);
    
    // ���÷���A������URSλ��ȷ��ֻ������������Ų���TRGO
    htim2.Instance->CR1 |= TIM_CR1_URS;  // ֻ��overflow����Update������CEN��UGʱ����
    
    // ���ü�������0��ȷ�����������ڿ�ʼ
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    
    // ������ʱ������һ��TRGO�����������ں���
    HAL_TIM_Base_Start(&htim2);
    
    // 2. �ȴ��������ݲɼ����
    uint32_t timeout = 0;
    while (ADC_BufferReadyFlag != BUFFER_READY_FLAG_FULL && timeout < 1000)
    {
        HAL_Delay(1);  // ������ʱ������CPUռ�ù���
        timeout++;
    }
    
    // 3. ����Ƿ�ʱ
    if (timeout >= 1000)
    {
        // ��ʱ����ֹͣADC+TIM��������������
        HAL_ADC_Stop_DMA(&hadc1);
        HAL_TIM_Base_Stop(&htim2);
        return 0;
    }
    
    // 4. ֹͣ��ǰ�ִε�ADC+TIM��Ϊ�´�������׼����
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    
    // ���URSλ���ָ�Ĭ����Ϊ
    htim2.Instance->CR1 &= ~TIM_CR1_URS;
    
    // 5. �����ֲɼ�������
    const uint16_t *src = (const uint16_t *)&adc_buffer[0];
    DemuxADCData(src, adc_in_buffer, adc_ac_out_buffer, adc_dc_out_buffer, FFT_SIZE);
    
//    // *** �����������֤ͨ������ ***
//    // ����ǰ10��ԭʼADCֵ�ͷ����ĵ�ѹֵ������֤
//    if (current_system_state == SWEEP_FREQ_RESPONSE_STATE) {
//        printf("Raw ADC[0-8]: %d,%d,%d | %d,%d,%d | %d,%d,%d\r\n",
//               adc_buffer[0], adc_buffer[1], adc_buffer[2],
//               adc_buffer[3], adc_buffer[4], adc_buffer[5], 
//               adc_buffer[6], adc_buffer[7], adc_buffer[8]);
//        
//        printf("After Demux[0-2]: Vin=%.3f, Vout=%.3f, Vdc=%.3f\r\n",
//               adc_in_buffer[0] + (adc_in_buffer[0] + adc_in_buffer[1] + adc_in_buffer[2])/3.0f, // �ָ�DC���ֵ
//               adc_ac_out_buffer[0] + (adc_ac_out_buffer[0] + adc_ac_out_buffer[1] + adc_ac_out_buffer[2])/3.0f,
//               adc_dc_out_buffer[0]);
//    }
    
    // 6. �ֱ���3���������е�����
    ProcessSampleData_F32(adc_in_buffer, pRes1, fs);
    ProcessSampleData_F32(adc_ac_out_buffer, pRes2, fs);
    CalcArrayMean(adc_dc_out_buffer, pRes3);

    return 1;  // ������������
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
    // ��һ����ת��Ϊ��ѹ������ƽ��ֵ��DC������
    float sum1 = 0.0f, sum2 = 0.0f;
    
    for(uint16_t i = 0; i < len; i++)
    {
        float volt1 = (float)src[3*i] * ADC_LSB_VOLT;
        float volt2 = (float)src[3*i+1] * ADC_LSB_VOLT;
        
        buf1[i] = volt1;
        buf2[i] = volt2;
        buf3[i] = (float)src[3*i+2] * ADC_LSB_VOLT;  // DCͨ������ԭ��
        
        sum1 += volt1;
        sum2 += volt2;
    }
    
    // �ڶ�����ΪACͨ��ȥ��DC������FFT��Ҫ��0Ϊ���ĵ��źţ�
    float dc1 = sum1 / (float)len;
    float dc2 = sum2 / (float)len;
    
    for(uint16_t i = 0; i < len; i++)
    {
        buf1[i] -= dc1;  // ȥ��DC�������õ���AC�ź�
        buf2[i] -= dc2;  // ȥ��DC�������õ���AC�ź�
        // buf3���ֲ��䣬��Ϊ������DC�ź�
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
 *  ����: ���� spectrum_analysis() 
 *  ����: ����������ݲ�����FFT�����������������Ч�Լ��
 *----------------------------------------------------*/
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes, float fs)
{
    
    // 1. ����FFT����
    spectrum_analysis(sampleData, FFT_SIZE, fs, pRes);

    // 2. ��鵱ǰϵͳ״̬�������Ƿ�������Ч�Լ��
    extern uint8_t current_system_state; // �����ⲿϵͳ״̬����
    
    if(current_system_state == BASIC_MEASUREMENT_STATE) {
        // ��������ģʽ�����ý����Ч�Լ����˲�
        bool result_valid = true;
        
        // ���1������Ӧ���ں���Χ�� (0.01V ~ 5V)
        if(pRes->amplitude < 0.01f || pRes->amplitude > 5.0f) {
            result_valid = false;
        }

        // ���2��Ƶ��Ӧ���ں���Χ�� (50Hz ~ 220kHz)
        if(pRes->frequency < 50.0f || pRes->frequency > 220000.0f) {
            result_valid = false;
        }
        

 
    } 
    else if(current_system_state == SWEEP_FREQ_RESPONSE_STATE) {
        // ɨƵģʽ����ȫ������Ч�Լ�飬ֱ��ʹ��FFTԭʼ���
        // ÿ��Ƶ��ֻ����һ�Σ���һ��FFT������ԭʼ�������
        // �����κι��˻��滻
    }
    // ����״̬������FFTԭʼ���
}

/**
 * @brief ��������迹
 * @param v_out_with_load ������ʱ�������ѹ
 * @param v_out_open_drain ��·ʱ�������ѹ
 * @retval ����迹ֵ��k����
 */
static float CalculateRout(SpectrumResult_t* v_out_with_load, SpectrumResult_t* v_out_open_drain)
{
    if(v_out_with_load->amplitude > 0.001f)  // �������
    {
        return (v_out_open_drain->amplitude - v_out_with_load->amplitude) / v_out_with_load->amplitude * R_L;
    }
    return 0.0f;  // �����ĸ̫С������0
}

/**
 * @brief �����迹���㺯��
 */
static float CalculateRin(SignalAnalysisResult_t* res)
{
    float Rin = (V_s * 1e-3 * 0.5 - res->adc_in_Result.amplitude/V_Rs_Gain)/(res->adc_in_Result.amplitude/V_Rs_Gain) * R_s;
    return Rin;  // k��
}
/* ===== Ӳ�����ƺ��� ===== */
/**
 * @brief ��������迹����״̬
 * @retval None
 */
static void Reset_Rout_Measurement(void)
{
    r_out_measured = 0;     // ���ò�����ɱ�־
    r_out_value = 0.0f;     // ���ò���ֵ
    measure_r_out_flag = 0; // ���ò���������־
}

/**
 * @brief ǿ��ֹͣ���������еĲ���������ģʽ�л�
 * @retval None
 */
static void Force_Stop_All_Operations(void)
{
    // ֹͣADC+DMA+TIMϵͳ
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    
    // ����ADC��������־
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    
    // �رռ̵�������������Ļ���
    RELAY_OFF;
}


/* ===== ��ʾ���� ===== */
/**
 * @brief ��LCD��Ļ����������ʾ����
 * @param title Ҫ��ʾ�ı����ַ���
 * @param y_pos ������Y���ϵ�λ�ã���ѡ��Ĭ�Ͻ���10-20��
 * @retval None
 */
void LCD_Display_Title_Center(const char* title, uint16_t y_pos)
{
    // ��Ļ�ߴ綨��
    #define SCREEN_WIDTH  240
    #define SCREEN_CENTER_X (SCREEN_WIDTH / 2)  // 120
    
    // �������
    #define FONT_WIDTH  8   // 16x8������ַ����
    #define FONT_HEIGHT 16  // 16x8������ַ��߶�
    
    // �����ַ�������
    uint16_t str_len = strlen(title);
    
    // �����ַ��������ؿ��
    uint16_t total_width = str_len * FONT_WIDTH;
    
    // ������ʼX����(ȷ���ַ������Ķ��뵽��Ļ����)
    uint16_t start_x = SCREEN_CENTER_X - (total_width / 2);
    
    // �߽��飬ȷ���ַ������ᳬ����Ļ�߽�
    if(start_x > SCREEN_WIDTH) start_x = 0;  // ��ֹ����
    if(start_x + total_width > SCREEN_WIDTH) start_x = SCREEN_WIDTH - total_width;
    
    // ������ʾ��ɫ
    POINT_COLOR = BLACK;    // ��ɫ����
    BACK_COLOR = WHITE;     // ��ɫ����

    // ��ʾ�����ַ���
    LCD_ShowString(start_x, y_pos, total_width, FONT_HEIGHT, 16, (uint8_t*)title);
}

/**
 * @brief ��ʼ����Ļ��ʾ
 */
void Basic_Measurement_Page_Init(void)
{
    LCD_Clear(WHITE);

    // ��ʾ����
    POINT_COLOR = TITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_Display_Title_Center("Basic Measurement", 10);

    // ========== �����źŲ��� ==========
    // ��ʾ������
    POINT_COLOR = SUBTITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(10, 35, 200, 16, 16, (uint8_t*)"Input Signal");

    // ��ʾ���ݱ�ǩ
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(15, 55, 80, 12, 12, (uint8_t*)"Vin: 1V");
    LCD_ShowString(15, 70, 80, 12, 12, (uint8_t*)"Vs:");
    LCD_ShowString(15, 85, 80, 12, 12, (uint8_t*)"fs:");
    LCD_ShowString(15, 100, 80, 12, 12, (uint8_t*)"Rin:");

    // ========== ����ź�(AC)���� ==========
    // ��ʾ������
    POINT_COLOR = SUBTITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(10, 125, 200, 16, 16, (uint8_t*)"Output Signal(AC)");

    // ��ʾ���ݱ�ǩ
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(15, 145, 100, 12, 12, (uint8_t*)"Vout(Open):");
    LCD_ShowString(15, 160, 100, 12, 12, (uint8_t*)"Vout(RL):");
    LCD_ShowString(15, 175, 100, 12, 12, (uint8_t*)"fs:");
    LCD_ShowString(15, 190, 100, 12, 12, (uint8_t*)"Rout:");

    // ========== ����ź�(DC)���� ==========
    // ��ʾ������
    POINT_COLOR = SUBTITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(10, 215, 200, 16, 16, (uint8_t*)"Output Signal(DC)");

    // ��ʾ���ݱ�ǩ
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(15, 235, 100, 12, 12, (uint8_t*)"Vout(DC):");

    // ========== ���Ʒָ��� ==========
    POINT_COLOR = GRAY;
    LCD_DrawLine(10, 120, 230, 120);    // �����AC���֮��ķָ���
    LCD_DrawLine(10, 210, 230, 210);    // AC�����DC���֮��ķָ���

    // �����������
    POINT_COLOR = SUBTITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(10, 255, 200, 16, 16, (uint8_t*)"Gain Measurement");

    // ��ʾ���ݱ�ǩ
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE; 
    LCD_ShowString(15, 275, 100, 12, 12, (uint8_t*)"Gain:");

}

/**
 * @brief ��LCD����ʾƵ��ֵ���Զ���λת��������4λ��Ч���֣�
 * @param frequency Ƶ��ֵ����λ��Hz��
 * @param x X����
 * @param y Y����
 * @param color ��ʾ��ɫ
 * @retval None
 */
void LCD_Display_Frequency(float frequency, uint16_t x, uint16_t y, uint16_t color)
{
    char freq_buffer[20];
    
    // ������ʾ��ɫ
    POINT_COLOR = color;
    BACK_COLOR = WHITE;
    
    // ����Ƶ�ʴ�С�Զ�ѡ����ʵĵ�λ�͸�ʽ
    if(frequency >= 1000000.0f)  // MHz����
    {
        float freq_mhz = frequency / 1000000.0f;
        if(freq_mhz >= 10.0f)
            sprintf(freq_buffer, "%.2f MHz", freq_mhz);  // 10.00 MHz
        else
            sprintf(freq_buffer, "%.3f MHz", freq_mhz);  // 1.000 MHz
    }
    else if(frequency >= 10000.0f)  // ����10000Hzת��ΪkHz
    {
        float freq_khz = frequency / 1000.0f;
        if(freq_khz >= 100.0f)
            sprintf(freq_buffer, "%.2f kHz", freq_khz);  // 100.00 kHz
        else if(freq_khz >= 10.0f)
            sprintf(freq_buffer, "%.3f kHz", freq_khz);  // 10.000 kHz
    }
    else  // С��10000Hz����ʹ��Hz��λ������2λС��
    {
        sprintf(freq_buffer, "%.2f Hz", frequency);      // 1234.50 Hz
    }
    
    // ��ʾƵ���ַ���
    LCD_ShowString(x, y, 120, 12, 12, (uint8_t*)freq_buffer);
}

/**
 * @brief ���»�������ҳ����ʾ
 */
void Basic_Measurement_Page_Update(void)  
{
    char dispBuff[48];  // �ֲ��ַ���������
    
    // ������ɫ
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    
    /* ���벿�� */
    // ��ѹ��ʾ (Vs:)
    LCD_Fill(60, 70, 230, 82, WHITE);
    sprintf(dispBuff, "%.3f V", data_at_1k.adc_in_Result.amplitude); 
    LCD_ShowString(60, 70, 120, 12, 12, (uint8_t*)dispBuff);

    // Ƶ����ʾ (fs:)
    LCD_Fill(60, 85, 230, 97, WHITE);
    LCD_Display_Frequency(data_at_1k.adc_in_Result.frequency, 60, 85, DATA_COLOR);

    // �����迹��ʾ (Rin:)
    LCD_Fill(60, 100, 230, 112, WHITE);
    float R_in = CalculateRin(&data_at_1k);
    sprintf(dispBuff, "%.3f k��", R_in); 
    LCD_ShowString(60, 100, 120, 12, 12, (uint8_t*)dispBuff);

    /* ����������� */
    // ��·��ѹ��ʾ (Vout(Open):)
    if(!r_out_measured) // ���δ��������迹����ʾ��·��ѹ
    {
        LCD_Fill(130, 145, 230, 157, WHITE);
        sprintf(dispBuff, "%.3f V", data_at_1k.adc_ac_out_Result.amplitude);
        LCD_ShowString(130, 145, 120, 12, 12, (uint8_t*)dispBuff);
    }
    else // ����Ѳ�������迹����ʾ�����ص�ѹ
    {
        LCD_Fill(130, 145, 230, 157, WHITE);
        sprintf(dispBuff, "%.3f V", last_v_open_drain_out.amplitude);
        LCD_ShowString(130, 145, 120, 12, 12, (uint8_t*)dispBuff);
    }

    // ���Ƶ����ʾ (fs:) - Ӧ��������Ƶ����ͬ
    LCD_Fill(60, 175, 230, 187, WHITE);
    LCD_Display_Frequency(data_at_1k.adc_ac_out_Result.frequency, 60, 175, DATA_COLOR);

    /* ���ֱ������ */
    // DC��ѹ��ʾ (Vout(DC):)
    LCD_Fill(90, 235, 230, 247, WHITE);
    sprintf(dispBuff, "%.3f V", data_at_1k.adc_dc_out_Result);  // ������ȥ��.amplitude
    LCD_ShowString(90, 235, 120, 12, 12, (uint8_t*)dispBuff);
    
    /* ��·������ʾ */
    LCD_Fill(100, 160, 230, 172, WHITE);
    /* ���㿪·���� */
    float gain = (data_at_1k.adc_ac_out_Result.amplitude * 11 * 47/247) / (V_s * 1e-3 * 0.5 - data_at_1k.adc_in_Result.amplitude/V_Rs_Gain) ;
    sprintf(dispBuff, "%.3f V", gain);
    LCD_ShowString(90, 275, 120, 12, 12, (uint8_t*)dispBuff);

    /* ����迹���� - ���ݲ���״̬��ʾ */
    LCD_Fill(60, 190, 230, 202, WHITE);

    if(r_out_measured)
    {
        // ��ʾ�������
        sprintf(dispBuff, "%.3f k��", r_out_value);
        LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)dispBuff);
    }
    else
    {
        // ��ʾδ����״̬
        LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)"--- k��");
    }
    
    /* ��ʾVout(RL) - �޸�ȱʧ����ʾ */
    if(r_out_measured)
    {
        // ����Ѳ�������迹����ʾ���ص�ѹ��������Ҫ�������ʱ��ֵ��
        LCD_Fill(100, 160, 230, 172, WHITE);
        sprintf(dispBuff, "%.3f V", v_out.amplitude, "(measured)");
        LCD_ShowString(100, 160, 120, 12, 12, (uint8_t*)dispBuff);
    }
    else
    {
        // δ����ʱ��ʾռλ��
        LCD_Fill(100, 160, 230, 172, WHITE);
        LCD_ShowString(100, 160, 120, 12, 12, (uint8_t*)"--- V");
    }
}

/**
 * @brief ����迹��ʾ����
 */
static void DisplayRout(float Rout)
{
    char dispBuff[48];  // �ֲ��ַ���������
    
    // ������ʾ��ɫ
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    
    // �������迹��ʾ����
    LCD_Fill(60, 190, 230, 202, WHITE);
    
    // ��ʽ������ʾ����迹
    sprintf(dispBuff, "%.3f k��", Rout);
    LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)dispBuff);
}


/* ============ ɨƵ�������� ============ */

/**
 * @brief �Զ�Ƶ�����Բ���������
 */
void Auto_Frequency_Response_Measurement(void)
{
    static uint8_t first_refresh = 1;  // ����ɨƵ����ҳ���ʼˢ��
    
    // ����Ƿ������ģʽ�л���ɨƵģʽ
    if (sweep_freq_response_flag)
    {
        // ǿ��ֹͣ���������еĲ���
        Force_Stop_All_Operations();
        
        first_refresh = 1;  // �������³�ʼ��
        sweep_freq_response_flag = 0;  // �����־λ
    }
    
    if(first_refresh)
    {
        // ��ȫ�����Ļ���ݣ�ȷ��û�в�����ʾ
        LCD_Clear(WHITE);
        HAL_Delay(50);  // ������ʱ��ȷ��LCD������
        
        // ȷ��AD9851�˳�����ģʽ����������
        AD9851_Exit_Power_Down();
        
        // ��ʼ��ҳ����ʾ
        LCD_Display_Title_Center("Frequency Response", 10);
        Draw_Coordinate_System();
        
        // *** ������ɨƵ��ʼ������Ϣ ***
        printf("\r\n=== FREQUENCY SWEEP START ===\r\n");
        printf("Frequency Range: %.1f Hz ~ %.1f Hz\r\n", FREQ_START, FREQ_STOP);
        printf("Total Points: %d\r\n", FREQ_POINTS);
        printf("Format: [Point] Set_Freq, Measured_In/Out_Freq, Vin, Vout, Gain, Sampling_Freq\r\n");
        printf("===============================\r\n");
        
        first_refresh = 0;
        measurement_complete = 0;  // ���ò�����ɱ�־
        current_point = 0;         // ���ò���������
    }

    // ���������δ��ɣ�����ɨƵ����
    if (!measurement_complete && current_point < FREQ_POINTS)
    {
        // ���㵱ǰƵ�ʵ�
        float input_freq = Calculate_Log_Frequency(current_point);
        
        // ����DDS����Ƶ��
        AD9851_Set_Frequency((uint32_t)input_freq);
        
        // ֻ���ö�ʱ����������Ӳ������Process_ADC_Data_F32����������
        float actual_fs = Tim2_Config_AutoFs(input_freq);
        
        // ������ǰƵ�ʵ㣬����ʵ�ʲ�����
        Measure_Single_Point(input_freq, &freq_response[current_point], actual_fs);
        
        // ���Ƶ�ǰ������
        plot_Frequency_Response_Point(freq_response[current_point]);
        
        // ���½�����ʾ
        Update_Measurement_Progress(current_point + 1, FREQ_POINTS);
        
        // �ƶ�����һ��Ƶ�ʵ�
        current_point++;
        
        // ����Ƿ�������в���
        if (current_point >= FREQ_POINTS)
        {
            measurement_complete = 1;
            
            // �ҳ�-3dBƵ�㲢��ʾ
            float freq_3db = Find_3dB_Frequency(freq_response, FREQ_POINTS);
            Display_3dB_Frequency(freq_3db);
            
            // ��ʾ����ͳ����Ϣ
            Display_Measurement_Statistics(freq_response, FREQ_POINTS);
            
            // *** ������ɨƵ��ɵ����ܽ� ***
            printf("\r\n=== FREQUENCY SWEEP COMPLETED ===\r\n");
            printf("-3dB Frequency: %.1f Hz\r\n", freq_3db);
            
            // ���㲢��ʾ���/��С����
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

/* ====== ���ݴ����� ====== */

/**
 * @brief ��������Ƶ�ʵ����Ӧ
 * @param frequency ����Ƶ��
 * @param result ��������洢
 * @param fs ʵ�ʲ���Ƶ��
 */
static void Measure_Single_Point(float frequency, FreqResponse_t* result, float fs)
{
    static SignalAnalysisResult_t single_result;
    
    // �ȴ��µ�ADC����
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
        // �洢�������
        result->frequency = frequency;
        result->input_amp = single_result.adc_in_Result.amplitude;
        result->output_amp = single_result.adc_ac_out_Result.amplitude;
        result->dc_out = single_result.adc_dc_out_Result;
        
        // ��������(dB)
        if (result->input_amp > 0.001f)
        {
            result->gain_db = 20.0f * log10f(result->output_amp / ( V_s *1e-3 - result->input_amp/V_Rs_Gain ));
        }
        else
        {
            result->gain_db = -100.0f;  // ��Сֵ
        }
        
        // ��λ����(�򻯴���)
        result->phase_deg = 0.0f;
        
        // *** ������ɨƵ����������� ***
        printf("SWEEP[%02d]: Set=%.1fHz, Measured=%.1fHz/%.1fHz, Vin=%.3fV, Vout=%.3fV, Vout_DC=%.3fV, Gain=%.2fdB, Fs=%.0fHz\r\n",
               current_point + 1,                                    // ���������
               frequency,                                             // �趨Ƶ��
               single_result.adc_in_Result.frequency,                 // �������Ƶ��
               single_result.adc_ac_out_Result.frequency,             // �������Ƶ��
               result->input_amp,                                     // �������
               result->output_amp,                                    // �������
               result->dc_out,                                        // ���ֱ����ѹ
               result->gain_db,                                       // ����(dB)
               fs,                                                    // ����Ƶ��
               );                                      
    }
    else
    {
        // ��ʱ����µ�Ĭ��ֵ
        result->frequency = frequency;
        result->input_amp = 0.0f;
        result->output_amp = 0.0f;
        result->gain_db = -100.0f;
        result->phase_deg = 0.0f;
        
        // *** ��������ʱ���������� ***
        printf("SWEEP[%02d]: TIMEOUT! Set=%.1fHz, Fs=%.0fHz (No valid data after %.1fs)\r\n",
               current_point + 1,
               frequency,
               fs,
               timeout / 1000.0f);
    }
}

/**
 * @brief ��������ֲ���Ƶ�ʵ�
 * @param point_index Ƶ�ʵ����� (0 �� FREQ_POINTS-1)
 * @retval ��Ӧ��Ƶ��ֵ (Hz)
 */
static float Calculate_Log_Frequency(uint8_t point_index)
{
    if (point_index >= FREQ_POINTS) {
        return FREQ_STOP;
    }
    
    // �����ֲ���freq = FREQ_START * (FREQ_STOP/FREQ_START)^(index/(FREQ_POINTS-1))
    float log_ratio = (float)point_index / (float)(FREQ_POINTS - 1);
    float frequency = FREQ_START * powf(FREQ_STOP / FREQ_START, log_ratio);
    
    return frequency;
}

/**
 * @brief ����-3dBƵ��
 * @param freq_response Ƶ����Ӧ��������
 * @param len ���鳤��
 * @retval -3dBƵ�㣬���δ�ҵ�����0
 */
static float Find_3dB_Frequency(FreqResponse_t* freq_response, int len)
{
    if (len < 2) return 0.0f;
    
    // �ҵ��������
    float max_gain = -100.0f;
	uint8_t max_index = 0;
    for (int i = 0; i < len; i++) {
        if (freq_response[i].gain_db > max_gain) {
            max_gain = freq_response[i].gain_db;
			max_index = i;
        }
    }
    
    // ����-3dBĿ��ֵ
    float target_gain = max_gain - 3.0f;
    
    // ���ҵ�һ������-3dB���Ƶ��
    for (int i = max_index; i < len; i++) {
        if (freq_response[i].gain_db < target_gain) {
            return freq_response[i].frequency;
        }
    }
    
    return 0.0f;  // δ�ҵ�
}



/**
 * @brief ���²���������ʾ
 * @param current_point ��ǰ������
 * @param total_points �ܲ�������
 */
static void Update_Measurement_Progress(uint8_t current_point, uint8_t total_points)
{
    char progress_text[30];
    sprintf(progress_text, "Progress: %d/%d", current_point, total_points);
    
    // ������ʾλ�ú���ɫ
    POINT_COLOR = BLUE;
    BACK_COLOR = WHITE;
    
    // ���������ʾ����
    LCD_Fill(10, 25, 200, 35, WHITE);
    
    // ��ʾ������Ϣ
    LCD_ShowString(10, 25, 180, 12, 12, (uint8_t*)progress_text);
}

/**
 * @brief ��������������ϵͳ
 */
static void Draw_Coordinate_System(void)
{
    // �����ͼ����
    uint16_t plot_x_start = 30;
    uint16_t plot_x_end = 210;
    uint16_t plot_y_start = 40;
    uint16_t plot_y_end = 180;
    
    // ���û�ͼ��ɫ
    POINT_COLOR = BLACK;
    
    // ����������
    LCD_DrawLine(plot_x_start, plot_y_end, plot_x_end, plot_y_end);    // X��
    LCD_DrawLine(plot_x_start, plot_y_start, plot_x_start, plot_y_end); // Y��
    
    // ����X��̶Ⱥͱ�ǩ(�����̶�)
    float log_min = log10f(FREQ_START);     // log10(100) = 2.0
    float log_max = log10f(FREQ_STOP);      // log10(200000) = 5.301
    
    // ��Ҫ�̶ȵ�: 100Hz, 1kHz, 10kHz, 100kHz
    float major_freqs[] = {100.0f, 1000.0f, 10000.0f, 100000.0f};
    char* major_labels[] = {"100", "1k", "10k", "100k"};
    
    for(int i = 0; i < 4; i++)
    {
        if(major_freqs[i] >= FREQ_START && major_freqs[i] <= FREQ_STOP)
        {
            float log_freq = log10f(major_freqs[i]);
            uint16_t x_pos = plot_x_start + (log_freq - log_min) * (plot_x_end - plot_x_start) / (log_max - log_min);
            
            // ���ƿ̶���
            LCD_DrawLine(x_pos, plot_y_end, x_pos, plot_y_end - 5);
            
            // ���Ʊ�ǩ
            POINT_COLOR = BLACK;
            BACK_COLOR = WHITE;
            LCD_ShowString(x_pos - 10, plot_y_end + 5, 30, 12, 12, (uint8_t*)major_labels[i]);
        }
    }
    
    // ����Y��̶Ⱥͱ�ǩ(����dB)
    int16_t gain_range[] = {-20, 0, 20, 40};
    char gain_labels[][5] = {"-20", "0", "20", "+40"};
    
    for(int i = 0; i < 4; i++)
    {
        // �������淶ΧΪ-30dB��+50dBdB
        uint16_t y_pos = plot_y_end - (gain_range[i] + 30) * (plot_y_end - plot_y_start) / 80;
        
        if(y_pos >= plot_y_start && y_pos <= plot_y_end)
        {
            // ���ƿ̶���
            LCD_DrawLine(plot_x_start, y_pos, plot_x_start + 5, y_pos);
            
            // ���Ʊ�ǩ
            POINT_COLOR = BLACK;
            BACK_COLOR = WHITE;
            LCD_ShowString(5, y_pos - 6, 25, 12, 12, (uint8_t*)gain_labels[i]);
        }
    }
    
    // �������������
    POINT_COLOR = BLACK;
    BACK_COLOR = WHITE;
    LCD_ShowString(plot_x_start + 60, plot_y_end + 20, 80, 12, 12, (uint8_t*)"Frequency");
    
    // ��ת���ֽϸ��ӣ����ü򵥵�Y�����
    LCD_ShowString(30, plot_y_start, 20, 12, 12, (uint8_t*)"dB");
}

/**
 * @brief ���Ƶ���Ƶ����Ӧ���ݵ�
 * @param point Ƶ����Ӧ���ݵ�
 */
void plot_Frequency_Response_Point(FreqResponse_t point)
{
    // �����ͼ����(������ϵ����һ��)
    uint16_t plot_x_start = 30;
    uint16_t plot_x_end = 210;
    uint16_t plot_y_start = 40;
    uint16_t plot_y_end = 180;
    
    // �������Ƶ������
    float log_min = log10f(FREQ_START);
    float log_max = log10f(FREQ_STOP);
    float log_freq = log10f(point.frequency);
    
    // ����X����
    uint16_t x_pos = plot_x_start + (log_freq - log_min) * (plot_x_end - plot_x_start) / (log_max - log_min);
    
    // ����Y����(���淶Χ-30dB��+50dB)
    float gain_min = -30.0f;
    float gain_max = 50.0f;
    uint16_t y_pos = plot_y_end - (point.gain_db - gain_min) * (plot_y_end - plot_y_start) / (gain_max - gain_min);
    
    // �߽���
    if(x_pos >= plot_x_start && x_pos <= plot_x_end && 
       y_pos >= plot_y_start && y_pos <= plot_y_end)
    {
        // ���û�ͼ��ɫ
        POINT_COLOR = RED;
        
        // �������ݵ�(ʮ�ֱ��)
        LCD_DrawLine(x_pos - 2, y_pos, x_pos + 2, y_pos);    // ˮƽ��
        LCD_DrawLine(x_pos, y_pos - 2, x_pos, y_pos + 2);    // ��ֱ��
        
        // ����СԲ��
        LCD_DrawPoint(x_pos, y_pos);
        LCD_DrawPoint(x_pos + 1, y_pos);
        LCD_DrawPoint(x_pos - 1, y_pos);
        LCD_DrawPoint(x_pos, y_pos + 1);
        LCD_DrawPoint(x_pos, y_pos - 1);
    }
}

/**
 * @brief ��ʾ-3dB��ֹƵ��
 * @param freq_3db -3dB��ֹƵ��ֵ��Hz��
 * @retval None
 */
static void Display_3dB_Frequency(float freq_3db)
{
    char freq_display[30];
    
    // ������ʾλ�ã���ͼ���·���
    uint16_t display_x = 10;
    uint16_t display_y = 200;
    
    // ������ʾ��ɫ
    POINT_COLOR = YELLOW;  // ��ɫ������ʾ
    BACK_COLOR = WHITE;    // ��ɫ����

    // ���Ƶ���Ƿ���Ч
    if(freq_3db > 0)
    {
        // ��ʽ��Ƶ����ʾ
        if(freq_3db >= 1000000.0f)  // MHz����
        {
            sprintf(freq_display, "-3dB Freq: %.2f MHz", freq_3db / 1000000.0f);
        }
        else if(freq_3db >= 1000.0f)  // kHz����
        {
            sprintf(freq_display, "-3dB Freq: %.2f kHz", freq_3db / 1000.0f);
        }
        else  // Hz����
        {
            sprintf(freq_display, "-3dB Freq: %.1f Hz", freq_3db);
        }
    }
    else
    {
        // δ�ҵ�-3dBƵ��
        sprintf(freq_display, "-3dB Freq: Not Found");
    }
    
    // �����ʾ����
    LCD_Fill(display_x, display_y, 230, display_y + 16, WHITE);
    
    // ��ʾ-3dBƵ��
    LCD_ShowString(display_x, display_y, 220, 16, 16, (uint8_t*)freq_display);
    
    // ����ҵ�����Ч��-3dBƵ�㣬��ͼ�ϱ��
    if(freq_3db > 0)
    {
        // �����ͼ����������ϵ����һ�£�
        uint16_t plot_x_start = 30;
        uint16_t plot_x_end = 210;
        uint16_t plot_y_start = 40;
        uint16_t plot_y_end = 180;
        
        // ���������Χ
        float log_min = log10f(FREQ_START);     // log10(100) = 2.0
        float log_max = log10f(FREQ_STOP);      // log10(200000) = 5.301
        
        // ����-3dBƵ����ͼ�ϵ�X����
        if(freq_3db >= FREQ_START && freq_3db <= FREQ_STOP)
        {
            float log_freq_3db = log10f(freq_3db);
            uint16_t x_3db = plot_x_start + (log_freq_3db - log_min) * (plot_x_end - plot_x_start) / (log_max - log_min);
            
            // ��ͼ�ϻ���-3dB��ֱ�����
            POINT_COLOR = YELLOW;
            for(uint16_t y = plot_y_start; y <= plot_y_end; y += 5)
            {
                // ��������
                LCD_Fast_DrawPoint(x_3db, y, YELLOW);
                LCD_Fast_DrawPoint(x_3db, y+1, YELLOW);
            }
            
            // ��-3dB������ӱ��
            uint16_t y_3db = plot_y_end - (3.0f + 30.0f) * (plot_y_end - plot_y_start) / 60.0f;  // -3dB��Ӧ��Y����
            
            // ���������ǣ�СԲȦ��
            POINT_COLOR = YELLOW;
            LCD_DrawLine(x_3db - 3, y_3db, x_3db + 3, y_3db);     // ˮƽ��
            LCD_DrawLine(x_3db, y_3db - 3, x_3db, y_3db + 3);     // ��ֱ��
            
            // ����Բ�α��
            for(int i = -2; i <= 2; i++)
            {
                for(int j = -2; j <= 2; j++)
                {
                    if(i*i + j*j <= 4)  // Բ�η�Χ
                    {
                        LCD_Fast_DrawPoint(x_3db + i, y_3db + j, YELLOW);
                    }
                }
            }
        }
    }
}

/**
 * @brief ��ʾ����ͳ����Ϣ
 * @param freq_response_data Ƶ����Ӧ��������
 * @param len ���鳤��
 * @retval None
 */
static void Display_Measurement_Statistics(FreqResponse_t* freq_response_data, int len)
{
    char info_display[40];
    
    // ������ʾλ��
    uint16_t display_x = 10;
    uint16_t display_y = 220;
    
    // ������ʾ��ɫ
    POINT_COLOR = CYAN;
    BACK_COLOR = BLACK;
    
    // �ҵ��������Ͷ�ӦƵ��
    float max_gain = -100.0f;
    float max_gain_freq = 0.0f;
    float min_gain = 100.0f;
    
    for(int i = 0; i < len; i++)
    {
        if(freq_response_data[i].frequency > 0)  // ��Ч����
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
    
    // �����ʾ����
    LCD_Fill(display_x, display_y, 230, display_y + 32, WHITE);
    
    // ��ʾ��ֵ������Ϣ
    if(max_gain_freq >= 1000.0f)
    {
        sprintf(info_display, "Peak: %.1fdB @ %.1fkHz", max_gain, max_gain_freq/1000.0f);
    }
    else
    {
        sprintf(info_display, "Peak: %.1fdB @ %.1fHz", max_gain, max_gain_freq);
    }
    LCD_ShowString(display_x, display_y, 220, 16, 12, (uint8_t*)info_display);
    
    // ��ʾ��̬��Χ
    sprintf(info_display, "Range: %.1f ~ %.1f dB", min_gain, max_gain);
    LCD_ShowString(display_x, display_y + 16, 220, 16, 12, (uint8_t*)info_display);
}


/* ===== ϵͳ���ƺ��� ===== */
/**
 * @brief ����ADC+DMA+TIMϵͳ
 * @param frequency Ŀ�����Ƶ�ʣ�Hz��
 * @retval ʵ�ʲ���Ƶ��
 */
float Start_ADC_DMA_TIM_System(float frequency)
{
    // 1. ֹͣ�����������е�ADC�Ͷ�ʱ��
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    
    // 2. ���ö�ʱ����Ŀ��Ƶ�ʶ�Ӧ�Ĳ����ʣ�����ȡʵ�ʲ�����
    float actual_fs = Tim2_Config_AutoFs(frequency);
    
    // 3. ˫�ر�����ȷ�������ʲ������
    if (actual_fs < 10000.0f) {
        // ǿ��ʹ�ð�ȫ�Ĳ�����
        actual_fs = Tim2_Config_AutoFs(20000.0f);  // ��������Ϊ20kHz
    }
    
    // 4. ����ADC+DMA��������ADC+DMA��
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
    
    // ������ܵ�DMA��־λ
    __HAL_DMA_CLEAR_FLAG(&hdma_adc1, DMA_FLAG_TCIF0_4);
    
    // 5. ������ʱ������������ʱ��������
    HAL_TIM_Base_Start(&htim2);
    
    // 6. ����ʵ�ʲ�����
    return actual_fs;
}

/**
 * @brief ֹͣADC+DMA+TIMϵͳ
 * @retval None
 */
void Stop_ADC_DMA_TIM_System(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
    ADC_BufferReadyFlag = BUFFER_READY_FLAG_NONE;
}

// ���ϼ�⺯��
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
        measurement_complete = 0;  // ���ò�����ɱ�־
        current_point = 0;         // ���ò���������
        LCD_Clear(WHITE);

        HAL_Delay(50);  // ������ʱ��ȷ��LCD������

        // ��ʼ��ҳ����ʾ
        LCD_Display_Title_Center("Fault Detection", 10);
    }

    // ---- ��������£�1kHz������ɨ�� ----
    current_fs = Tim2_Config_AutoFs(200000.0f);  // ���ó�������µĲ�����Ϊ200kHz
    
    Process_ADC_Data_F32(&data_at_1k.adc_in_Result, &data_at_1k.adc_ac_out_Result, &data_at_1k.adc_dc_out_Result, current_fs);

    // ---- ���ϼ�� ----
    // 1. ���������źŵķ�ֵ
    float input_amp = V_s * 1e-3 * 0.5 - data_at_1k.adc_in_Result.amplitude/V_Rs_Gain;

    // 2. ��������źŵķ�ֵ
    float output_amp = data_at_1k.adc_ac_out_Result.max_amp;

    // 3. ��������
    float gain = output_amp / input_amp;

    // 4. ���������迹
    float R_in = CalculateRin(&data_at_1k);

    // 5. �ж�������Χ
    if ((fabs(gain - NORMAL_GAIN_AT_1K) <= NORMAL_GAIN_RANGE)
    &&  (fabs(R_in - NORMAL_R_IN_AT_1K) <= NORMAL_R_IN_RANGE)
    &&  (fabs(output_amp - NORMAL_VOUT_AC_AT_1K) <= NORMAL_VOUT_AC_RANGE))
    {
        // ����
        LCD_ShowString(10, 40, 220, 16, 12, (uint8_t*)"Normal");
        fault_detected = 0;
    }
    else
    {
        fault_detected = 1;
    }

    if(fault_detected)
    {
        // ---- ��ʼɨƵ ----
        // ���������δ��ɣ�����ɨƵ����
        if (!measurement_complete && current_point < FREQ_POINTS)
        {
            // ���㵱ǰƵ�ʵ�
            float input_freq = Calculate_Log_Frequency(current_point);
            
            // ����DDS����Ƶ��
            AD9851_Set_Frequency((uint32_t)input_freq);
            
            // ֻ���ö�ʱ����������Ӳ������Process_ADC_Data_F32����������
            float actual_fs = Tim2_Config_AutoFs(input_freq);
            
            // ������ǰƵ�ʵ㣬����ʵ�ʲ�����
            Measure_Single_Point(input_freq, &freq_response[current_point], actual_fs);
            
            // ���Ƶ�ǰ������
            plot_Frequency_Response_Point(freq_response[current_point]);
            
            // ���½�����ʾ
            Update_Measurement_Progress(current_point + 1, FREQ_POINTS);
            
            // �ƶ�����һ��Ƶ�ʵ�
            current_point++;
        }

        if (current_point >= FREQ_POINTS)
        {
            measurement_complete = 1;

            // �ж��ǵ����쳣�����ǵ����쳣
            float gain_variance = Calculate_Gain_Variance(freq_response, FREQ_POINTS);
            if(gain_variance < R_Gain_Variance_Threshold)
            {
                // ��������, �����쳣
                fault_type = C_FAULT_TYPE;
            }
            else if(gain_variance < C_Gain_Variance_Threshold)
            {
                // �����쳣, ��������
                fault_type = R_FAULT_TYPE;
            }

            if(fault_type == R_FAULT_TYPE)
            {
                // ϸ���жϵ����������, ʹ�����������1khzʱ��õ�ֱ�������ѹ�ж�
                if( fabs()   0.9f)

            }
            else if(fault_type == C_FAULT_TYPE)
            {

            }
        }
    }

}

/*
 * @brief ��������ķ���
 * @param freq_response Ƶ����Ӧ��������
 * @param len ���鳤��
 * @retval ����ķ���
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








