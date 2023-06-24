#ifndef _WS2812_H_
#define _WS2812_H_
#include "main.h"
#include "usart.h"

/*
软件模拟时序
需要
    1.GPIO
        开漏浮空输出模式，需要外接5V上拉，注意需要选择手册中标有FT的管脚
        将其设置为最高规格输出
*/
#define WS2812_GPIO_Group GPIOA
#define WS2812_GPIO_Pin GPIO_PIN_0
// #define WS2812_Software

/*
硬件定时器PWM+DMA:
需要:
    1.定时器:
        PWM输出一个通道
        不分频
        计数值为 1.25us(公式: 1.25 *系统频率(单位MHz))
        输出IO设置为开漏浮空输出(外接5V上拉)
    2.DMA
        内存到外设
        字(word)模式
        开启DMA中断
0码的是 0.4us,1码是0.8us
公式是 t(us)*系统频率(单位MHz)
 */
typedef enum ledMode{
	Breath,     //枚举默认第一个元素为0,如果想自定义为1,这句改成led_mode_breath=1,
	Marquee,    //枚举元素之间用逗号，不是分号.编号默认在第一个元素后每个自增1.
	Music
}ledMode_typ;    //蓝牙控制led模式的变化

typedef enum{
	LED_RED=0,      //呼吸灯按R->G->B的顺序切换并呼吸
	LED_GREEN,
	LED_BLUE,
}LedState;

extern TIM_HandleTypeDef htim1;
extern uint32_t g_current_led_cnt;
extern uint8_t RX_buff[2];    //Uart1的接收缓冲区，2个字节
extern ledMode_typ g_mode; 


#define WS2812_TIM htim1
#define WS2812_TIM_Channel TIM_CHANNEL_1
#define WS2812_Code_0 (22u)      //我这里72M主频，原来TIM1的分频105.这个设定值用的是32和71; 有人建议72M如果分频用90则这0码和1码的比较值分别是22和57会更合适,但我测试不行
#define WS2812_Code_1 (57u)        //比较器的设定值,超过71就为低电平输出；注意这个值设定的不合适，会在点灯数量多了之后，点灯色彩与预期不符.

#define WS2812_Num 30



extern const uint32_t color[];
extern uint32_t WS2812_Data[WS2812_Num];
void WS2812_Code_Reset(void);
void WS2812_Send(void);
void WS2812_Start(void);

void led_off(void);     //全部led熄灭,清零
//void mono_breath_change(const uint32_t *mono_color,uint8_t index);   //单色呼吸灯效果
//void led_breath_R(void);  //static函数
//void led_breath_G(void);
//void led_breath_B(void);
void led_mode_breath(void);     //呼吸灯模式
void led_mode_marquee(void);    //流水灯模式
void led_light_count(uint32_t cnt);    //自由设定点灯个数
void led_mode_music(void);   //音乐氛围等的效果
void led_mode_select(void);    //用户选择亮灯模式

#endif
