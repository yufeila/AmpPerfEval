# Basic_Measurement �����߼������Ż�����

## ��ǰʵ�ַ���

### 1. ��������ṹ
```c
void Basic_Measurement(void)
{
    static uint8_t first_refresh = 1;
    static float current_fs = 0.0f;
    
    // ģʽ״̬����
    // ��ʼ������
    // ���ݲɼ�����
    // ��ʾ����
    // ����迹����
}
```

### 2. �ؼ���������
```
DMA������ -> Process_ADC_Data_F32() -> data_at_1k -> Basic_Measurement_Page_Update() -> LCD��ʾ
```

### 3. ��ǰ�������

#### A. ���ݾ�����һ��������
- **����**: `data_at_1k` �ڲ����ڼ���ܱ���������
- **Ӱ��**: ����迹����ʱ����·��ѹֵ���ܲ��ȶ�
- **����**: ���������׼ȷ���û������

#### B. ����ʽ��ʱ����
- **����**: `HAL_Delay(1000)` ����������ϵͳ
- **Ӱ��**: ������Ӧ�ٻ�����ʾ����ֹͣ
- **����**: �û��о�ϵͳ����

#### C. �쳣ֵ��������
- **����**: ȱ�ٶ��쳣���ݵļ��͹���
- **Ӱ��**: ������ʾ������Ĳ������
- **����**: �û���ϵͳ�ɿ��Բ�������

#### D. ���������Ѻ�
- **����**: ���������ֻ����-1���û��޷��˽����ԭ��
- **Ӱ��**: ���Ժ�ά������
- **����**: �û��޷��жϲ���ʧ�ܵ�ԭ��

## �Ż�����

### 1. ����һ�����Ż�

#### ����A: ������������
```c
typedef struct {
    SignalAnalysisResult_t data;
    uint8_t data_valid;
    uint8_t lock_flag;
} ProtectedData_t;

static ProtectedData_t protected_data_at_1k;

// ��Process_ADC_Data_F32������������
if (protected_data_at_1k.lock_flag == 0) {
    // ��������
    protected_data_at_1k.data = new_data;
    protected_data_at_1k.data_valid = 1;
}
```

#### ����B: ���ݿ��ջ���
```c
void Basic_Measurement(void) {
    // ������迹������ʼǰ��ȡ���ݿ���
    if(measure_r_out_flag) {
        static SignalAnalysisResult_t data_snapshot;
        static uint8_t snapshot_taken = 0;
        
        if(!snapshot_taken) {
            data_snapshot = data_at_1k;  // ������ǰ����
            snapshot_taken = 1;
        }
        
        // ʹ�ÿ������ݽ��в���
        SpectrumResult_t v_open_drain_out = data_snapshot.adc_ac_out_Result;
        // ... ��������
    }
}
```

### 2. ������ʽ��ʱ�Ż�

```c
typedef enum {
    ROUT_IDLE = 0,
    ROUT_RELAY_ON,
    ROUT_WAITING,
    ROUT_MEASURING,
    ROUT_CALCULATING,
    ROUT_DISPLAY,
    ROUT_CLEANUP
} RoutMeasureState_t;

static RoutMeasureState_t rout_state = ROUT_IDLE;
static uint32_t rout_timer = 0;
static uint8_t rout_measure_count = 0;

void Basic_Measurement(void) {
    // ��Ҫ�����߼�
    Process_ADC_Data_F32(&data_at_1k.adc_in_Result, 
                        &data_at_1k.adc_ac_out_Result, 
                        &data_at_1k.adc_dc_out_Result, current_fs);
    
    Basic_Measurement_Page_Update();
    
    // ������ʽ����迹����״̬��
    Handle_Rout_Measurement_NonBlocking();
}

void Handle_Rout_Measurement_NonBlocking(void) {
    static SpectrumResult_t v_open_drain_out;
    static SpectrumResult_t v_out_with_load[PROCESS_ARRAY_SIZE_WITH_R_L];
    
    switch(rout_state) {
        case ROUT_IDLE:
            if(measure_r_out_flag) {
                // ���濪·��ѹ
                v_open_drain_out = data_at_1k.adc_ac_out_Result;
                RELAY_ON;
                rout_timer = HAL_GetTick();
                rout_state = ROUT_RELAY_ON;
                rout_measure_count = 0;
            }
            break;
            
        case ROUT_RELAY_ON:
            // �ȴ��̵����ȶ�
            if(HAL_GetTick() - rout_timer >= 500) {  // 500ms��ʱ
                rout_state = ROUT_MEASURING;
            }
            break;
            
        case ROUT_MEASURING:
            // ����ɼ����ݵ�
            if(rout_measure_count < PROCESS_ARRAY_SIZE_WITH_R_L) {
                if(Process_ADC_Data_F32(&v_out_with_load[rout_measure_count].adc_in_Result,
                                      &v_out_with_load[rout_measure_count].adc_ac_out_Result,
                                      &v_out_with_load[rout_measure_count].adc_dc_out_Result,
                                      current_fs)) {
                    rout_measure_count++;
                }
            } else {
                rout_state = ROUT_CALCULATING;
            }
            break;
            
        case ROUT_CALCULATING:
            {
                SpectrumResult_t v_out_avg;
                average_spectrumresult_array(v_out_with_load, &v_out_avg, PROCESS_ARRAY_SIZE_WITH_R_L);
                
                float R_out = CalculateRout(&v_out_avg, &v_open_drain_out);
                
                if(R_out >= 0) {
                    DisplayRout(R_out);
                } else {
                    DisplayRout_Error("����ʧ��");
                }
                
                rout_state = ROUT_CLEANUP;
            }
            break;
            
        case ROUT_CLEANUP:
            RELAY_OFF;
            measure_r_out_flag = 0;
            rout_state = ROUT_IDLE;
            break;
    }
}
```

### 3. �쳣ֵ���͹���

```c
// �쳣ֵ���ṹ
typedef struct {
    float min_reasonable;
    float max_reasonable;
    float last_valid_value;
    uint8_t consecutive_errors;
    uint8_t max_consecutive_errors;
} DataValidator_t;

static DataValidator_t voltage_validator = {
    .min_reasonable = 0.001f,      // ��С�����ѹ 1mV
    .max_reasonable = 5.0f,        // �������ѹ 5V
    .last_valid_value = 0.0f,
    .consecutive_errors = 0,
    .max_consecutive_errors = 3
};

uint8_t ValidateVoltageData(float voltage, DataValidator_t* validator) {
    // ������Χ���
    if(voltage < validator->min_reasonable || voltage > validator->max_reasonable) {
        validator->consecutive_errors++;
        return 0;  // ��Ч����
    }
    
    // �仯�ʼ�飨��ֹͻ�䣩
    if(validator->last_valid_value > 0) {
        float change_rate = fabsf(voltage - validator->last_valid_value) / validator->last_valid_value;
        if(change_rate > 0.5f) {  // �仯����50%
            validator->consecutive_errors++;
            return 0;  // ���ܵ��쳣����
        }
    }
    
    // ������Ч�����ô������
    validator->consecutive_errors = 0;
    validator->last_valid_value = voltage;
    return 1;  // ��Ч����
}

// �����ݴ�����Ӧ����֤
void Basic_Measurement_Page_Update(void) {
    // ��֤�����ѹ
    if(ValidateVoltageData(data_at_1k.adc_in_Result.amplitude, &voltage_validator)) {
        // ��ʾ��Ч����
        sprintf(dispBuff, "%.3f V", data_at_1k.adc_in_Result.amplitude);
    } else {
        // ��ʾ����ָʾ
        sprintf(dispBuff, "ERR V");
    }
    LCD_ShowString(60, 70, 120, 12, 12, (uint8_t*)dispBuff);
}
```

### 4. �û��ѺõĴ�����

```c
typedef enum {
    ROUT_ERROR_NONE = 0,
    ROUT_ERROR_INVALID_PARAMS,
    ROUT_ERROR_VOLTAGE_TOO_LOW,
    ROUT_ERROR_VOLTAGE_ABNORMAL,
    ROUT_ERROR_RESULT_UNREASONABLE,
    ROUT_ERROR_HARDWARE_FAULT
} RoutErrorCode_t;

typedef struct {
    RoutErrorCode_t error_code;
    const char* error_message;
    const char* user_suggestion;
} RoutErrorInfo_t;

static const RoutErrorInfo_t rout_error_table[] = {
    {ROUT_ERROR_NONE, "����", ""},
    {ROUT_ERROR_INVALID_PARAMS, "��������", "�������"},
    {ROUT_ERROR_VOLTAGE_TOO_LOW, "��ѹ����", "����ź�Դ"},
    {ROUT_ERROR_VOLTAGE_ABNORMAL, "��ѹ�쳣", "����·"},
    {ROUT_ERROR_RESULT_UNREASONABLE, "���������", "���²���"},
    {ROUT_ERROR_HARDWARE_FAULT, "Ӳ������", "��ϵ����֧��"}
};

float CalculateRout_Enhanced(SpectrumResult_t* v_out_with_load, 
                           SpectrumResult_t* v_out_open_drain,
                           RoutErrorCode_t* error_code) {
    *error_code = ROUT_ERROR_NONE;
    
    // ����������
    if(v_out_with_load == NULL || v_out_open_drain == NULL) {
        *error_code = ROUT_ERROR_INVALID_PARAMS;
        return -1.0f;
    }
    
    // ��鿪·��ѹ
    if(v_out_open_drain->amplitude < 0.001f) {
        *error_code = ROUT_ERROR_VOLTAGE_TOO_LOW;
        return -1.0f;
    }
    
    // ��鸺�ص�ѹ
    if(v_out_with_load->amplitude < 0.001f) {
        *error_code = ROUT_ERROR_VOLTAGE_TOO_LOW;
        return -1.0f;
    }
    
    // ����ѹ��ϵ
    if(v_out_with_load->amplitude >= v_out_open_drain->amplitude) {
        *error_code = ROUT_ERROR_VOLTAGE_ABNORMAL;
        return -1.0f;
    }
    
    // ��������迹
    float rout = (v_out_open_drain->amplitude - v_out_with_load->amplitude) 
                 / v_out_with_load->amplitude * R_L;
    
    // �����������
    if(rout < 0.0f || rout > 1000.0f) {
        *error_code = ROUT_ERROR_RESULT_UNREASONABLE;
        return -1.0f;
    }
    
    return rout;
}

void DisplayRout_Enhanced(float Rout, RoutErrorCode_t error_code) {
    char dispBuff[48];
    
    POINT_COLOR = DATA_COLOR;
    BACK_COLOR = WHITE;
    LCD_Fill(60, 190, 230, 202, WHITE);
    
    if(error_code == ROUT_ERROR_NONE) {
        // ������ʾ
        sprintf(dispBuff, "%.3f k��", Rout);
        LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)dispBuff);
    } else {
        // ������ʾ
        POINT_COLOR = RED;
        sprintf(dispBuff, "����:%s", rout_error_table[error_code].error_message);
        LCD_ShowString(60, 190, 120, 12, 12, (uint8_t*)dispBuff);
        
        // ����һ����ʾ����
        sprintf(dispBuff, "%s", rout_error_table[error_code].user_suggestion);
        LCD_ShowString(60, 205, 120, 12, 12, (uint8_t*)dispBuff);
    }
}
```

### 5. �����Ż�����

#### A. FFT����Ƶ���Ż�
```c
// ���ݲ���ģʽ����FFT����Ƶ��
#define PROCESS_INTERVAL_NORMAL     1    // ����ģʽ��ÿ�ζ�����
#define PROCESS_INTERVAL_ROUT       5    // ����迹����������Ƶ�ʼ��ٸ���

static uint8_t get_process_interval(void) {
    if(rout_state != ROUT_IDLE) {
        return PROCESS_INTERVAL_ROUT;
    }
    return PROCESS_INTERVAL_NORMAL;
}
```

#### B. ��ʾ�����Ż�
```c
// ֻ�����ݱ仯ʱ������ʾ
static float last_displayed_values[6];  // �洢�ϴ���ʾ��ֵ
static uint8_t display_changed_flag = 0;

void Basic_Measurement_Page_Update_Smart(void) {
    float current_values[6] = {
        data_at_1k.adc_in_Result.amplitude,
        data_at_1k.adc_in_Result.frequency,
        data_at_1k.adc_ac_out_Result.amplitude,
        data_at_1k.adc_ac_out_Result.frequency,
        data_at_1k.adc_dc_out_Result,
        CalculateRin(&data_at_1k)
    };
    
    // ����Ƿ��������仯
    for(int i = 0; i < 6; i++) {
        if(fabsf(current_values[i] - last_displayed_values[i]) > 0.001f) {
            display_changed_flag = 1;
            break;
        }
    }
    
    if(display_changed_flag) {
        // ������ʾ
        Basic_Measurement_Page_Update();
        
        // ���浱ǰֵ
        memcpy(last_displayed_values, current_values, sizeof(current_values));
        display_changed_flag = 0;
    }
}
```

## ʵ�����ȼ�

### �����ȼ�������ʵ�֣�
1. **���ݿ��ջ���** - �������һ��������
2. **������ʽ��ʱ** - �����û�����
3. **�����쳣ֵ���** - ��߲����ɿ���

### �����ȼ�������ʵ�֣�
1. **��ǿ�Ĵ�����** - �ṩ�û��ѺõĴ�����Ϣ
2. **�����Ż�** - ���ϵͳ��Ӧ�ٶ�
3. **��ʾ�����Ż�** - ���ٲ���Ҫ��ˢ��

### �����ȼ��������Ż���
1. **�߼�������֤** - �����ӵ��쳣����㷨
2. **����Ӧ����Ƶ��** - ����ϵͳ���ص�������Ƶ��
3. **��ʷ���ݼ�¼** - �������Ʒ������쳣���

## ���Խ���

### 1. ���ܲ���
- �����������̲���
- ����迹��������
- ������Ӧ����
- �쳣����������

### 2. ���ܲ���
- ϵͳ��Ӧʱ�����
- �ڴ�ʹ�ò���
- ��ʱ�������ȶ��Բ���

### 3. �û��������
- ��ʾ���������Ȳ���
- ������ʾ�����Ȳ���
- ���������Բ���

## �ܽ�

��ǰ��`Basic_Measurement`�����Ѿ�ʵ���˻������ܣ���������һ���ԡ��û�����ʹ������滹�������ռ䡣ͨ�������Ż����飬�����������ϵͳ�Ŀɿ��Ժ��û����顣���鰴�����ȼ���ʵʩ��ȷ��ÿ���Ľ���������ֲ��ԡ�
