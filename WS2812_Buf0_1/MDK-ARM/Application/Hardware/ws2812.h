#ifndef _WS2812_H_
#define _WS2812_H_
#include "main.h"
#include "usart.h"

/*
���ģ��ʱ��
��Ҫ
    1.GPIO
        ��©�������ģʽ����Ҫ���5V������ע����Ҫѡ���ֲ��б���FT�Ĺܽ�
        ��������Ϊ��߹�����
*/
#define WS2812_GPIO_Group GPIOA
#define WS2812_GPIO_Pin GPIO_PIN_0
// #define WS2812_Software

/*
Ӳ����ʱ��PWM+DMA:
��Ҫ:
    1.��ʱ��:
        PWM���һ��ͨ��
        ����Ƶ
        ����ֵΪ 1.25us(��ʽ: 1.25 *ϵͳƵ��(��λMHz))
        ���IO����Ϊ��©�������(���5V����)
    2.DMA
        �ڴ浽����
        ��(word)ģʽ
        ����DMA�ж�
0����� 0.4us,1����0.8us
��ʽ�� t(us)*ϵͳƵ��(��λMHz)
 */
typedef enum ledMode{
	Breath,     //ö��Ĭ�ϵ�һ��Ԫ��Ϊ0,������Զ���Ϊ1,���ĳ�led_mode_breath=1,
	Marquee,    //ö��Ԫ��֮���ö��ţ����Ƿֺ�.���Ĭ���ڵ�һ��Ԫ�غ�ÿ������1.
	Music
}ledMode_typ;    //��������ledģʽ�ı仯

typedef enum{
	LED_RED=0,      //�����ư�R->G->B��˳���л�������
	LED_GREEN,
	LED_BLUE,
}LedState;

extern TIM_HandleTypeDef htim1;
extern uint32_t g_current_led_cnt;
extern uint8_t RX_buff[2];    //Uart1�Ľ��ջ�������2���ֽ�
extern ledMode_typ g_mode; 


#define WS2812_TIM htim1
#define WS2812_TIM_Channel TIM_CHANNEL_1
#define WS2812_Code_0 (22u)      //������72M��Ƶ��ԭ��TIM1�ķ�Ƶ105.����趨ֵ�õ���32��71; ���˽���72M�����Ƶ��90����0���1��ıȽ�ֵ�ֱ���22��57�������,���Ҳ��Բ���
#define WS2812_Code_1 (57u)        //�Ƚ������趨ֵ,����71��Ϊ�͵�ƽ�����ע�����ֵ�趨�Ĳ����ʣ����ڵ����������֮�󣬵��ɫ����Ԥ�ڲ���.

#define WS2812_Num 30



extern const uint32_t color[];
extern uint32_t WS2812_Data[WS2812_Num];
void WS2812_Code_Reset(void);
void WS2812_Send(void);
void WS2812_Start(void);

void led_off(void);     //ȫ��ledϨ��,����
//void mono_breath_change(const uint32_t *mono_color,uint8_t index);   //��ɫ������Ч��
//void led_breath_R(void);  //static����
//void led_breath_G(void);
//void led_breath_B(void);
void led_mode_breath(void);     //������ģʽ
void led_mode_marquee(void);    //��ˮ��ģʽ
void led_light_count(uint32_t cnt);    //�����趨��Ƹ���
void led_mode_music(void);   //���ַ�Χ�ȵ�Ч��
void led_mode_select(void);    //�û�ѡ������ģʽ

#endif
