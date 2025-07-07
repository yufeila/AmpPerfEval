# ADC/DMA采样数据内容检查分析

## 问题现象总结

### 1. 显示问题
- **只有DC通道有值**：`Vout (DC): 0.866 V` 能正常变化
- **AC/频率信号测量为0**：
  - Input Signal: `Vs: 0.000 V`, `fs: 0.00 Hz`
  - Output Signal (AC): `Vout (Open): 0.000 V`, `fs: 0.00 Hz`
- **异常数值**：`Rin: 34028236692093848000`（疑似浮点溢出）

### 2. 调试现象
- **Keil调试**：`data_at_1k` 确实只有 `adc_dc_out_Result` 有值
- **adc_buffer无法查看**：在Keil Watch窗口中看不到 `adc_buffer[BUF_SIZE]` 的值

## 技术配置分析

### ADC配置
```c
// ADC通道配置（正确）
ADC_CHANNEL_2 (PA2) -> Rank 1 -> 输入信号通道
ADC_CHANNEL_4 (PA4) -> Rank 2 -> AC输出信号通道  
ADC_CHANNEL_6 (PA6) -> Rank 3 -> DC输出信号通道

// DMA配置
BUF_SIZE = FFT_SIZE * ADC_CHANNELS = 2048 * 3 = 6144
数据存储格式：[CH2, CH4, CH6, CH2, CH4, CH6, ...]
```

### 数据分离逻辑分析
```c
// DemuxADCData函数
for(uint16_t i = 0; i < len; i++)
{
    buf1[i] = ((float)src[3*i] - ADC_ZERO_CODE)* ADC_LSB_VOLT;     // CH2 (输入)
    buf2[i] = ((float)src[3*i+1] - ADC_ZERO_CODE)* ADC_LSB_VOLT;   // CH4 (AC输出)
    buf3[i] = ((float)src[3*i+2])* ADC_LSB_VOLT;                   // CH6 (DC输出，不减零点)
}
```

**注意**：通道3 (DC输出) 不减零点偏移，这是合理的，因为DC测量需要绝对值。

### 常量定义检查
```c
#define ADC_ZERO_CODE    2048.0f    // 12位ADC中点：2^12/2 = 2048
#define ADC_LSB_VOLT     0.0008058f // LSB电压：3.3V / 4096 = 0.0008058V
#define FFT_SIZE         2048U      // FFT点数
#define ADC_CHANNELS     3          // ADC通道数
```

## 可能原因分析

### 1. Keil调试中看不到adc_buffer的原因

#### 原因A：编译器优化
- `adc_buffer` 声明为 `__IO uint16_t`，但可能被优化器处理
- **解决方案**：在Watch窗口中使用 `&adc_buffer[0],x` 或 `(uint16_t*)adc_buffer,x` 格式

#### 原因B：DMA未完成或缓冲区未初始化
- DMA传输可能尚未开始或完成
- **检查方法**：查看 `ADC_BufferReadyFlag` 状态

#### 原因C：内存地址问题
- 缓冲区可能被分配到特殊内存区域
- **检查方法**：查看MAP文件中adc_buffer的内存地址

### 2. AC/频率信号测量为0的原因

#### 原因A：ADC/DMA配置问题
- **时序问题**：ADC、DMA、TIM同步可能有问题
- **触发源**：`ADC_EXTERNALTRIGCONV_T2_TRGO` 配置是否正确
- **采样时序**：3个通道的采样时序可能不同步

#### 原因B：信号路径问题
- **硬件连接**：PA2、PA4引脚是否真正有信号输入
- **信号幅度**：输入信号幅度可能超出ADC测量范围
- **信号偏置**：AC信号可能需要偏置到ADC输入范围内

#### 原因C：数据处理问题
- **DemuxADCData**：通道映射可能错误
- **FFT处理**：`spectrum_analysis` 函数可能有问题
- **处理间隔**：`PROCESS_INTERVAL` 可能导致数据丢失

#### 原因D：浮点计算溢出
- `Rin: 34028236692093848000` 明显是计算溢出
- 可能除零或除以极小数导致

## 调试建议

### 1. 立即检查项目

#### A. 验证adc_buffer内容
```c
// 在Keil调试器中尝试：
&adc_buffer[0],100     // 查看前100个数据
(uint16_t*)0x20000XXX,100  // 使用具体内存地址
```

#### B. 检查ADC_BufferReadyFlag状态
```c
// 确认DMA是否真正完成
if(ADC_BufferReadyFlag == BUFFER_READY_FLAG_FULL) {
    // 查看adc_buffer此时的内容
}
```

#### C. 添加原始数据日志
```c
// 在DemuxADCData函数中添加调试输出
for(uint16_t i = 0; i < 10; i++) {  // 只看前10个数据点
    printf("ADC[%d]: CH2=%d, CH4=%d, CH6=%d\n", 
           i, src[3*i], src[3*i+1], src[3*i+2]);
}
```

### 2. 系统性排查

#### 第一步：验证DMA数据采集
1. 确认 `adc_buffer` 中确实有非零数据
2. 验证3个通道的数据是否按预期交错存储
3. 检查数据范围是否在0-4095之间

#### 第二步：验证数据分离
1. 在 `DemuxADCData` 后检查3个float缓冲区内容
2. 确认电压转换计算是否正确
3. 验证AC通道(buf1, buf2)是否有有效的交流分量

#### 第三步：验证FFT处理
1. 检查传递给 `spectrum_analysis` 的数据是否有效
2. 验证FFT函数返回的频率和幅度结果
3. 确认 `PROCESS_INTERVAL` 机制是否工作正常

#### 第四步：验证硬件信号
1. 用示波器确认PA2、PA4、PA6引脚的信号
2. 验证信号幅度是否在ADC测量范围内(0-3.3V)
3. 检查信号是否需要适当的偏置

## 紧急修复方案

### 方案1：增加调试输出
```c
// 在Process_ADC_Data_F32函数中增加调试代码
uint8_t Process_ADC_Data_F32(SpectrumResult_t* pRes1, SpectrumResult_t* pRes2, float* pRes3, float fs)
{
    // ... 现有代码 ...
    
    // 调试：检查原始ADC数据
    uint32_t sum_ch2 = 0, sum_ch4 = 0, sum_ch6 = 0;
    for(int i = 0; i < 100; i++) {  // 检查前100个采样点
        sum_ch2 += adc_buffer[3*i];
        sum_ch4 += adc_buffer[3*i+1]; 
        sum_ch6 += adc_buffer[3*i+2];
    }
    
    // 通过UART或LCD显示统计信息
    printf("ADC Debug: CH2_avg=%lu, CH4_avg=%lu, CH6_avg=%lu\n", 
           sum_ch2/100, sum_ch4/100, sum_ch6/100);
    
    // ... 继续原有处理 ...
}
```

### 方案2：强制数据可见性
```c
// 在adc.c中修改adc_buffer声明
volatile uint16_t adc_buffer[BUF_SIZE] __attribute__((section(".bss")));
```

### 方案3：简化测试
```c
// 临时跳过FFT，直接测试数据采集
static void Simple_ADC_Test(void) {
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUF_SIZE);
    HAL_TIM_Base_Start(&htim2);
    
    while(ADC_BufferReadyFlag != BUFFER_READY_FLAG_FULL);
    
    // 直接显示原始ADC值
    LCD_Show_ADC_Raw_Data(adc_buffer);
    
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_TIM_Base_Stop(&htim2);
}
```

## 预期结果

修复后应该能看到：
1. **adc_buffer有效数据**：每个通道都应该有0-4095范围内的数据
2. **AC通道有交流分量**：输入1kHz，2V信号应该在FFT中检测到1kHz峰值
3. **频率测量正确**：显示应该接近输入的1kHz
4. **幅度测量正确**：显示应该接近输入的2V幅度
5. **Rin计算正常**：不再显示溢出的极大数值
