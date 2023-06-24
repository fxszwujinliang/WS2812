#ifndef _MICROPHONE_H_
#define _MICROPHONE_H_
#include "main.h"
#include "ws2812.h"        //为了使用宏"WS2812_Num",必须引入这个头文件


extern ADC_HandleTypeDef hadc1;
extern uint32_t g_current_led_cnt;
void mic_get_adcValue(void);


#endif
