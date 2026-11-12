#include "stm32f10x.h"

// 共阴极数码管段码表 0~9，带小数点0~9
const uint8_t tab[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,  // 0 - 9
    0xBF, 0x86, 0xDB, 0xCF, 0xE6, 0xED, 0xFD, 0x87, 0xFF, 0xEF,  // 0. - 9. (带小数点)
    0x00                                                         // 全灭 (高位消影)
};

// 显示缓存区：[0]第一位, [1]第二位, [2]第三位(带点), [3]第四位
uint16_t huancun[4] = {20, 20, 20, 20}; 

/* 软件延时 */
void Delay_us(uint32_t us)
{
    uint32_t i;
    for(i = 0; i < us * 8; i++) {
        __NOP();
    }
}

/* GPIO & ADC 初始化 */
void System_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    // 1. 使能 GPIOA, GPIOB, ADC1 和 AFIO 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | 
                           RCC_APB2Periph_ADC1 | RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. 禁用 JTAG，释放 PB3、PB4 供段选使用
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    // 3. 配置 PA0 为模拟输入 (内部 ADC 采集端)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 4. 配置 PA8, PA9 为推挽输出 (前两位位选)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 5. 配置 PB2, PB3 (后两位位选) 及 PB4~PB11 (段选) 为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 |
                                  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 |
                                  GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 初始化位选引脚全为高电平（关闭显示）
    GPIO_SetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9);
    GPIO_SetBits(GPIOB, GPIO_Pin_2 | GPIO_Pin_3);

    // 6. 配置 ADC1
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; // 开启连续转换
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);

    ADC_Cmd(ADC1, ENABLE);                  // 开启 ADC
    ADC_SoftwareStartConvCmd(ADC1, ENABLE); // 启动转换
}

/* 动态扫描显示函数 */
void Display(void)
{
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        // 1. 先关闭所有 4 个位选（拉高高电平，消影）
        GPIO_SetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9);
        GPIO_SetBits(GPIOB, GPIO_Pin_2 | GPIO_Pin_3);

        // 2. 将段码输出到 PB4 ~ PB11
        uint16_t port_val = GPIO_ReadOutputData(GPIOB);
        port_val &= ~(0xFF << 4);                   // 清空 PB4~PB11
        port_val |= (tab[huancun[i]] & 0xFF) << 4;  // 写入新段码
        GPIO_Write(GPIOB, port_val);

        // 3. 打开当前位的位选（拉低低电平）
        switch(i)
        {
            case 0: GPIO_ResetBits(GPIOA, GPIO_Pin_8); break; // 第1位 -> PA8
            case 1: GPIO_ResetBits(GPIOA, GPIO_Pin_9); break; // 第2位 -> PA9
            case 2: GPIO_ResetBits(GPIOB, GPIO_Pin_2); break; // 第3位 -> PB2
            case 3: GPIO_ResetBits(GPIOB, GPIO_Pin_3); break; // 第4位 -> PB3
        }

        Delay_us(1000); // 维持显示时间
    }
}

/* 主函数 */
int main(void)
{
    uint16_t cnt = 0;
    double date = 0;
    int32_t val = 0;

    System_Configuration(); // 硬件配置

    while(1)
    {
        cnt++;
        if(cnt >= 10)
        {
            cnt = 0;
            
            // 读取 ADC1 转换值
            uint16_t adc_val = ADC_GetConversionValue(ADC1);
            date = (double)adc_val * 500.0 / 4095.0;

            // 原51程序的分段拟合计算公式
            if(date > 461 && date <= 494)      { date = (date * -3.1818 + 1576.35) / 10.0; }
            else if(date > 438 && date <= 461) { date = (date * -3.6364 + 1782.74) / 10.0; }
            else if(date > 401 && date <= 438) { date = (date * -4.8571 + 2319.98) / 10.0; }
            else if(date > 373 && date <= 401) { date = (date * -6.0000 + 2782.00) / 10.0; }
            else if(date > 341 && date <= 373) { date = (date * -8.0000 + 3528.00) / 10.0; }
            else if(date > 300 && date <= 341) { date = (date * -9.3750 + 3990.63) / 10.0; }
            else if(date > 263 && date <= 300) { date = (date * -12.500 + 4950.00) / 10.0; }
            else if(date > 222 && date <= 263) { date = (date * -19.512 + 6831.66) / 10.0; }
            else if(date > 137 && date <= 222) { date = (date * -41.1765 + 11641.18) / 10.0; }
            else if(date > 91 && date <= 137)  { date = (date * -100.000 + 19700.00) / 10.0; }
            else if(date > 66 && date <= 91)   { date = (date * -235.2941 + 32294.12) / 10.0; }
            else if(date > 50 && date <= 66)   { date = (date * -454.5455 + 47000.00) / 10.0; }
            else if(date > 39 && date <= 50)   { date = (date * -727.2727 + 62000.00) / 10.0; }
            else if(date > 33 && date <= 39)   { date = (date * -1000.000 + 74000.00) / 10.0; }
            else if(date > 25 && date <= 33)   { date = (date * -2166.6667 + 111500.00) / 10.0; }
            else if(date > 17 && date <= 25)   { date = (date * -4848.4211) / 10.0 + 17736.253; }
            else if(date >= 14 && date <= 17)  { date = (date * -3846.1538) / 10.0 + 15692.308; }
            else                               { date = 0; }

            val = (int32_t)date;
            if(val < 0) val = 0;

            // 分解数值到显示缓存：如 val = 191，显示 " 19.1"
            uint8_t shi = (val / 100) % 10;    // 十位
            uint8_t ge  = (val / 10) % 10;     // 个位
            uint8_t dec = val % 10;            // 小数位

            huancun[0] = 20;                   // 第一位 (PA8) 全灭
            
            // 第二位 (PA9)：十位 (为0时消影)
            if(shi == 0) {
                huancun[1] = 20;
            } else {
                huancun[1] = shi;
            }

            huancun[2] = ge + 10;              // 第三位 (PB2)：个位带小数点
            huancun[3] = dec;                  // 第四位 (PB3)：小数位
        }

        Display(); // 持续动态扫描
    }
}