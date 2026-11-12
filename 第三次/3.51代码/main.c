#include <reg52.h>        // 单片机SFR定义
#include <intrins.h>      // 内部函数定义

#define uchar unsigned char  // 无符号字符型变量
#define uint  unsigned int   // 无符号整型变量

/* ADC0831 引脚定义 */
sbit CS   = P1^0;   // P1.0 片选
sbit CLK  = P1^1;   // P1.1 时钟
sbit DO   = P1^2;   // P1.2 数据

// 共阴极数码管 0～9，带小数点0～9，'-'、'U'、全灭
uchar code tab[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f,
    0xbf, 0x86, 0xdb, 0xcf, 0xe6, 0xed, 0xfd, 0x87, 0xff, 0xef,
    0x40, 0x3e, 0x00
};
uchar code du[] = {0xfe, 0xfd, 0xfb, 0xf7};  // 位选控制码
uint huancun[]  = {0, 0, 0, 0};               // 显示数据缓存

void delay(void);
void display(void);
uchar Read_ADC0831(void);

/********************************************************
* 主程序
********************************************************/
void main(void)
{
    uint i;
    long date;
    while(1)
    {
        i++;
        if(i == 100)
        {
            i = 0;
            date = Read_ADC0831() * 100;
            date = date / 51;    // 5V对应255，date=255*100/51=500

            if(date > 461 && date <= 494)
            {
                date = date * (-3.1818);
                date = date + 1576.35;
                date = date / 10;
            }
            else if(date > 438 && date <= 461)
            {
                date = date * (-3.6364);
                date = date + 1782.74;
                date = date / 10;
            }
            else if(date > 401 && date <= 438)
            {
                date = date * (-4.8571);
                date = date + 2319.98;
                date = date / 10;
            }
            else if(date > 373 && date <= 401)
            {
                date = date * (-6);
                date = date + 2782;
                date = date / 10;
            }
            else if(date > 341 && date <= 373)
            {
                date = date * (-8);
                date = date + 3528;
                date = date / 10;
            }
            else if(date > 300 && date <= 341)
            {
                date = date * (-9.375);
                date = date + 3990.63;
                date = date / 10;
            }
            else if(date > 263 && date <= 300)
            {
                date = date * (-12.5);
                date = date + 4950;
                date = date / 10;
            }
            else if(date > 222 && date <= 263)
            {
                date = date * (-19.512);
                date = date + 6831.66;
                date = date / 10;
            }
            else if(date > 137 && date <= 222)
            {
                date = date * (-41.1765);
                date = date + 11641.18;
                date = date / 10;
            }
            else if(date > 91 && date <= 137)
            {
                date = date * (-100);
                date = date + 19700;
                date = date / 10;
            }
            else if(date > 66 && date <= 91)
            {
                date = date * (-235.2941);
                date = date + 32294.12;
                date = date / 10;
            }
            else if(date > 50 && date <= 66)
            {
                date = date * (-454.5455);
                date = date + 47000;
                date = date / 10;
            }
            else if(date > 39 && date <= 50)
            {
                date = date * (-727.2727);
                date = date + 62000;
                date = date / 10;
            }
            else if(date > 33 && date <= 39)
            {
                date = date * (-1000);
                date = date + 74000;
                date = date / 10;
            }
            else if(date > 25 && date <= 33)
            {
                date = date * (-2166.6667);
                date = date + 111500;
                date = date / 10;
            }
            else if(date > 17 && date <= 25)
            {
                date = date * (-4848.4211);
                date = date / 10;
                date = date + 17736.253;
            }
            else if(date >= 14 && date <= 17)
            {
                date = date * (-3846.1538);
                date = date / 10;
                date = date + 15692.308;
            }

            huancun[0] = date / 1000;        // 千位
            huancun[1] = date % 1000 / 100;  // 百位
            huancun[2] = date % 100 / 10;    // 十位
            huancun[2] += 10;                // 开启小数点
            huancun[3] = date % 10;          // 小数第一位
        }
        display();
    }
}

/********************************************************
* 延时函数
********************************************************/
void delay(void)
{
    uchar i;
    for(i = 0; i < 180; i++);
}

/********************************************************
* 数码管动态显示
* 位选：P3，段选：P2
********************************************************/
void display(void)
{
    uchar i;
    for(i = 0; i < 4; i++)
    {
        P3 = du[i];
        P2 = tab[huancun[i]];
        delay();
    }
}

/********************************************************
* 读取ADC0831 （保留你原版时序）
********************************************************/
uchar Read_ADC0831(void)
{
    uchar i, temp;
    DO = 1;
    _nop_();
    _nop_();
    CS = 0;
    _nop_();
    _nop_();
    CLK = 0;
    _nop_();
    _nop_();
    CLK = 1;
    _nop_();
    _nop_();
    CLK = 0;
    _nop_();
    _nop_();
    CLK = 1;
    _nop_();
    _nop_();
    CLK = 0;
    _nop_();
    _nop_();

    for(i = 0; i < 8; i++)
    {
        CLK = 1;
        _nop_();
        _nop_();
        temp <<= 1;
        if(DO)
        {
            temp++;
        }
        CLK = 0;
        _nop_();
        _nop_();
    }
    CS = 1;
    _nop_();
    _nop_();
    return temp;
}