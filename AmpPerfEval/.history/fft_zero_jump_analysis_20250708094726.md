# FFT测量数值间歇性跳零问题分析

## 问题现象
- FFT结果大部分时间稳定在正确值附近（如0.50V、0.837V）
- 偶尔会跳转到0附近（每3-5次显示出现一次）
- 与输入信号幅度无关，任何幅度都会出现此现象
- 当前FFT_SIZE = 1024

## 可能原因分析

### 1. 采样不完整或数据缺失
**原因**：
- ADC/DMA采集过程中出现中断或数据丢失
- 缓冲区在FFT处理前被其他操作覆盖
- 定时器触发不稳定导致某些周期采样点数不足

**特征**：
- 随机性出现，与信号幅度无关
- 通常表现为FFT输出频率和幅度都接近0

### 2. 采样与信号频率不同步
**原因**：
- 1024点采样无法完整覆盖整数个信号周期
- 采样窗口偶尔落在信号的过零点附近
- 频率漂移导致的周期性不匹配

**分析**：
对于1kHz信号，如果采样率为20kHz：
- 每周期采样点数：20000/1000 = 20点
- 1024点对应：1024/20 = 51.2个周期
- 非整数周期可能导致频谱泄漏和幅度估计误差

### 3. DC分量去除过度
**原因**：
- DemuxADCData中去除DC分量的算法在某些情况下过度补偿
- 当输入信号的DC偏置变化时，去零点算法可能误判

### 4. FFT窗口效应
**原因**：
- 没有使用窗函数，矩形窗导致的频谱泄漏
- 信号不是整数周期时，主频能量分散到多个频点

### 5. 硬件干扰或供电不稳定
**原因**：
- 系统时钟抖动
- 电源纹波影响ADC精度
- 其他硬件模块的干扰

## 解决方案

### 立即可测试的方案

#### 方案1：增加采样点数统计
在ProcessSampleData_F32函数中添加有效性检查：
```c
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes, float fs)
{
    // 检查输入数据有效性
    float data_sum = 0.0f, data_max = -999.0f, data_min = 999.0f;
    for(int i = 0; i < FFT_SIZE; i++) {
        data_sum += fabsf(sampleData[i]);
        if(sampleData[i] > data_max) data_max = sampleData[i];
        if(sampleData[i] < data_min) data_min = sampleData[i];
    }
    
    float data_range = data_max - data_min;
    
    // 如果数据范围太小，可能是无效采样
    if(data_range < 0.01f) {  // 小于10mV认为异常
        // 返回上一次有效结果或设置错误标志
        pRes->amplitude = 0.0f;
        pRes->frequency = 0.0f;
        return;
    }
    
    spectrum_analysis(sampleData, FFT_SIZE, fs, pRes);
}
```

#### 方案2：结果滤波和有效性检查
添加结果有效性检查和简单滤波：
```c
static SpectrumResult_t last_valid_result = {0};

// 在基本测量函数中
if(pRes->amplitude > 0.05f && pRes->frequency > 100.0f) {
    // 结果看起来合理，更新last_valid_result
    last_valid_result = *pRes;
} else {
    // 结果异常，使用上次有效结果
    *pRes = last_valid_result;
}
```

#### 方案3：增加数据采集稳定性检查
在Process_ADC_Data_F32中增加多重验证：
```c
// 在数据采集完成后
static uint32_t consecutive_valid_samples = 0;

// 检查ADC数据质量
bool data_valid = true;
for(int i = 0; i < 100; i++) {
    if(adc_buffer[3*i] == 0 || adc_buffer[3*i+1] == 0) {
        data_valid = false;
        break;
    }
}

if(data_valid) {
    consecutive_valid_samples++;
    // 只有连续几次采样都有效才处理
    if(consecutive_valid_samples >= 2) {
        // 正常处理
    }
} else {
    consecutive_valid_samples = 0;
    return 0; // 跳过这次处理
}
```

### 长期优化方案

#### 方案A：改进采样参数
- 调整采样率确保覆盖整数个信号周期
- 对于1kHz信号，使用25.6kHz采样率（25.6个点/周期，1024点=40个完整周期）

#### 方案B：添加窗函数
- 在FFT前应用汉宁窗或布莱克曼窗减少频谱泄漏

#### 方案C：增加FFT点数
- 如果内存允许，考虑使用2048点FFT提高频率分辨率

## 建议的实施顺序

1. **? 已实施方案1+2+3组合**：
   - ? 方案1：在ProcessSampleData_F32中添加输入数据有效性检查
   - ? 方案2：添加结果有效性检查和简单滤波
   - ? 方案3：在Process_ADC_Data_F32中增加数据采集稳定性检查

2. **测试验证**：编译烧录后观察FFT跳零现象是否改善

3. **如果问题持续，实施长期优化方案**：根据测试结果决定是否需要调整采样参数或算法

## 已实施的修复内容

### 输入数据有效性检查（方案1）
- 检查数据范围（最大值-最小值）是否 ≥ 10mV
- 检查数据平均值是否 ≥ 1mV
- 无效数据时返回上次有效结果

### 结果有效性检查和滤波（方案2）
- 幅度范围检查：0.01V ~ 5V
- 频率范围检查：10Hz ~ 100kHz
- 变化率检查：相比上次结果，幅度变化不超过50%，频率变化不超过100Hz
- 数据与结果一致性检查：有明显AC信号但频率为0时判为异常

### 数据采集稳定性检查（方案3）
- 检查前100个采样点的质量
- 统计零值和饱和值数量
- 连续有效采样计数，确保数据稳定性

这种多层防护机制既能快速改善用户体验，又能系统性地解决根本问题。
