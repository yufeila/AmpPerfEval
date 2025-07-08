# 数据处理问题诊断报告

## ? ADC数据分析（基于串口输出）

### 原始ADC数据特征
```
[ADC] CH2: 1009(3129) CH4: 827(1317) CH6: 898(464)
```
- **CH2 (输入通道)**: 平均值≈1009, 动态范围≈3129 ? 有明显AC信号
- **CH4 (AC输出通道)**: 平均值≈827, 动态范围≈1317 ? 有明显AC信号  
- **CH6 (DC输出通道)**: 平均值≈898, 动态范围≈464 ? 相对稳定的DC信号

**结论：ADC/DMA采集完全正常，问题在数据处理环节！**

## ? 问题定位

### 1. 数据转换检查

当前的`DemuxADCData`函数：
```c
buf1[i] = ((float)src[3*i] - ADC_ZERO_CODE) * ADC_LSB_VOLT;      // CH2(输入)
buf2[i] = ((float)src[3*i+1] - ADC_ZERO_CODE) * ADC_LSB_VOLT;   // CH4(AC输出) 
buf3[i] = ((float)src[3*i+2]) * ADC_LSB_VOLT;                   // CH6(DC输出)
```

**常量定义**：
- `ADC_ZERO_CODE = 2048.0f` (12位ADC中点)
- `ADC_LSB_VOLT = 0.0008058f` (3.3V/4096)

**计算验证**：
以CH2为例，ADC值1009：
```
电压值 = (1009 - 2048) * 0.0008058 = -1039 * 0.0008058 = -0.837V
```

**?? 发现问题1：负电压**
- ADC值1009小于零点2048，转换后为负电压
- FFT分析可能不适合处理负电压信号
- 需要检查信号是否正确偏置

### 2. FFT处理间隔检查

`ProcessSampleData_F32`函数中：
```c
const uint8_t process_interval = PROCESS_INTERVAL;  // = 1
if (++process_counter >= process_interval) {
    // 只有满足条件时才执行FFT
    spectrum_analysis(sampleData, FFT_SIZE, fs, pRes);
}
```

**?? 发现问题2：静态变量未初始化**
- `process_counter`是静态变量，可能没有被初始化为0
- 如果初值不是0，可能导致第一次调用不执行FFT

### 3. spectrum_analysis函数检查

需要验证：
- FFT函数是否正确初始化
- 输入数据格式是否符合要求
- 采样率参数是否正确传递

## ?? 修复建议

### 立即修复1：信号偏置问题
```c
// 修改DemuxADCData函数，确保正确的信号处理
for(uint16_t i = 0; i < len; i++)
{
    // 直接转换为电压，保持原始DC偏置
    buf1[i] = (float)src[3*i] * ADC_LSB_VOLT;
    buf2[i] = (float)src[3*i+1] * ADC_LSB_VOLT;   
    buf3[i] = (float)src[3*i+2] * ADC_LSB_VOLT;   
}
```

### 立即修复2：确保FFT每次执行
```c
static void ProcessSampleData_F32(float *sampleData, SpectrumResult_t *pRes, float fs)
{
    // 临时移除间隔控制，确保每次都执行FFT
    spectrum_analysis(sampleData, FFT_SIZE, fs, pRes);
}
```

### 立即修复3：添加调试输出
在每个关键步骤添加串口调试，确认：
1. `DemuxADCData`后的电压值范围
2. FFT函数的输入和输出
3. 最终显示的数值

## ? 验证步骤

1. **先测试DC通道**：确认0.866V的DC值计算是否正确
   ```
   预期：898 * 0.0008058 ≈ 0.724V (与显示的0.866V不符？)
   ```

2. **测试AC通道转换**：
   ```
   CH2: 1009 → 1009 * 0.0008058 ≈ 0.813V
   CH4: 827 → 827 * 0.0008058 ≈ 0.667V
   ```

3. **检查FFT输入数据范围**：FFT通常期望以0为中心的AC信号

## ? 最可能的根本原因

**推测：信号偏置处理错误**
- 当前减去`ADC_ZERO_CODE`可能不适合所有通道
- CH6(DC)不减零点是对的，但CH2/CH4可能需要不同的处理
- FFT可能需要去除DC分量后的纯AC信号

**建议：分别处理DC和AC信号**
- DC通道：直接转换电压
- AC通道：去除DC分量，只保留AC分量用于FFT分析
