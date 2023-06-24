#include "microphone.h"


void mic_get_adcValue(void)
{
	uint32_t adc_value = 0;
	HAL_ADC_Start(&hadc1);      //启动AD转换
	adc_value = HAL_ADC_GetValue(&hadc1);  
	
	if(adc_value > 5000)         //从高往低设if-else分支
	{
		g_current_led_cnt = WS2812_Num;
	}
	else if(adc_value > 4000)    //4000~5000之间，不会把5000以上也算进去的
	{
		g_current_led_cnt = 20;
	}
	else if(adc_value > 2500)
	{
		g_current_led_cnt = 10;
	}
	else if(adc_value > 2300)
	{
		g_current_led_cnt = 9;
	}
	else if(adc_value > 2200)
	{
		g_current_led_cnt = 8;
	}
	else if(adc_value > 2100)
	{
		g_current_led_cnt = 7;
	}
	else if(adc_value > 2000)
	{
		g_current_led_cnt = 6;
	}
	else if(adc_value > 1900)
	{
		g_current_led_cnt = 5;
	}
	else if(adc_value > 1800)
	{
		g_current_led_cnt = 4;
	}
	else if(adc_value > 1700)
	{
		g_current_led_cnt = 3;
	}
	else
		g_current_led_cnt = 2;
}
