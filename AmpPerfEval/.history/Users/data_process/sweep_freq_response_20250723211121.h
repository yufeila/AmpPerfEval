/*
 * @Author: yyf 17786321727@163.com
 * @Date: 2025-07-09 09:18:07
 * @LastEditors: yyf 17786321727@163.com
 * @LastEditTime: 2025-07-23 21:11:05
 * @FilePath: /AmpPerfEval/Users/data_process/sweep_freq_response.h
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __SWEEP_FREQ_RESPONSE_H
#define __SWEEP_FREQ_RESPONSE_H

#include "main.h"
#include <math.h>  
#include "tim.h"
#include "./ad9851/bsp_ad9851.h"
#include "./spectrum_analysis/spectrum_analysis.h"
#include "./lcd/lcd.h"
#include "./key/key.h"
#include "adc.h"

#define FREQ_START      100.0f   // ��ʼɨ��Ƶ�� 100Hz
#define FREQ_STOP       200000.0f   // ��ֹɨ��Ƶ�� 200 kHz
#define FREQ_POINTS     50         
#define SETTLE_TIME_MS  100       
#define ARROW_SIZE      8          


// Ƶ������趨
#define ADC_TOTAL_FS       1354838.7f   // ADC ����Ƶ�� (ȫ��ͨ��)
#define ADC_NUM_CHANNELS   3
#define F_s   (ADC_TOTAL_FS / ADC_NUM_CHANNELS)

// LCD��ʾ��غ궨��
#define LCD_X_LENGTH    240
#define LCD_Y_LENGTH    320
#define LINE(x)         ((x) * 16)  // ÿ��16���ظ߶�

// ���ݴ���(���¿���)��غ궨��
#define PROCESS_INTERVAL 1
#define PROCESS_ARRAY_SIZE_WITH_R_L 8

// ADC��غ궨��
#define ADC_ZERO_CODE           2048.0f     // 12λADC�����ֵ (2^12/2 = 2048)
#define ADC_LSB_VOLT            0.0008058f  // ADC��LSB��ѹֵ (3.3V / 4096 = 0.0008058V)

// ��ɫ��غ궨��
#define TITLE_COLOR             BLUE
#define SUBTITLE_COLOR          GREEN  
#define DATA_COLOR              BLACK

// ��·��ز�������
#define V_s                     20.8f //��λ��mV
#define V_Rs_Gain               110.0f
#define R_s                     6.2f //��λ��k��
#define R_L                     570.0f   //��λ��k��     

// ���ϼ����ز�������
#define NORMAL_GAIN_AT_1K    2.3f  // ��λ: k��
#define NORMAL_GAIN_RANGE    0.2f  // ��λ: k��
#define NORMAL_R_IN_AT_1K    2.3f  // ��λ: k��
#define NORMAL_R_IN_RANGE    0.2f  // ��λ: k��
#define NORMAL_VOUT

typedef struct {
    SpectrumResult_t adc_in_Result;
    SpectrumResult_t adc_ac_out_Result;
    float adc_dc_out_Result;     
} SignalAnalysisResult_t;

// ��Ƶ�������ߵ�ṹ��
typedef struct {
    float frequency;   
    float input_amp;    
    float output_amp;   
    float gain_db;       
    float phase_deg;     
} FreqResponse_t;


// ��������
void Basic_Measurement(void);
void Auto_Frequency_Response_Measurement(void);

// ϵͳ���ƺ���
float Start_ADC_DMA_TIM_System(float frequency);
void Stop_ADC_DMA_TIM_System(void);


// ����״̬��ģʽ����
extern uint8_t basic_measurement_flag;
extern uint8_t sweep_freq_response_flag;  
extern uint8_t measure_r_out_flag;
extern uint8_t current_system_state;

// �̵������ƺ�
#define RELAY_ON  HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_SET)
#define RELAY_OFF HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET)

#endif /* __SWEEP_FREQ_RESPONSE_H */
