#include "WS2812.h"
#include "microphone.h"      //����mic_get_adcValue()����������

uint32_t WS2812_Data[WS2812_Num] = {0};   //�û�������,WS2812_Num�ǲʴ��ϵ�LED����

uint32_t WS2812_SendBuf0[25] = {0};   //���ͻ�����0���ڷ�������ʱ����������������ʹ��;
uint32_t WS2812_SendBuf1[25] = {0};   //���ͻ�����1������A����������һ��LED��������ʱ������һ��B������׼�����ݣ����ڽ��������ڶ���LED�ķ���
const uint32_t WS2812_Rst[240] = {0}; //��λ�뻺����
uint32_t WS2812_En = 0;               //����ʹ��

LedState ledState = LED_RED;     //������״̬�����ĳ�ʼ��

/**
 * @brief ��uint32תΪ���͵����ݣ���24bits����ɫֵת����pwmռ�ձ�
 * @param Data:��ɫ����
 * @param Ret:����������(PWMռ�ձ�)
 * @return
 * @author HZ12138
 * @date 2022-10-03 18:03:17
 */
void WS2812_uint32ToData(uint32_t Data, uint32_t *Ret)
{
    uint32_t zj = Data;
    uint8_t *p = (uint8_t *)&zj;
    uint8_t R = 0, G = 0, B = 0;
    B = *(p);     // B
    G = *(p + 1); // G
    R = *(p + 2); // R
    zj = (G << 16) | (R << 8) | B;       //���Ͱ�main�������趨��RGB˳�������LEDӲ��Ҫ���GRB˳����
    for (int i = 0; i < 24; i++)     //������ɫ����0xff0000��ɫ����ʱzj�Ѿ��������uint8_t��ϳɵ�������;0xFF0000��
    {
        if (zj & (1 << 23))
            Ret[i] = WS2812_Code_1;  //��ʼʱ,0xFF0000��(1<<23)��λ��,���Ϊ1����Ret[0]=71
        else
            Ret[i] = WS2812_Code_0;   //���Ƶ���9λʱ��λ��Ľ��Ϊ0,���������֧,��Ret[8]=32
        zj <<= 1;   //����ɫ��������һλ�����λ��
    }
    Ret[24] = 0;      //����25λ���ֲ�һ��0,��Ϊ���ڷ�����ÿһ֡���ݺ󣬱���һ��ʱ���϶������CPU�Ѿ�������.
}
/**
 * @brief ���ͺ���(DMA�жϵ���)
 * @param ��
 * @return ��
 * @author HZ12138
 * @date 2022-10-03 18:04:50
 */
void WS2812_Send(void)     //�ڶ�ʱ��PWM�ж��ڱ�����ִ��
{
    static uint32_t j = 0;
    static uint32_t ins = 0;
    if (WS2812_En == 1)
    {
			if (j == WS2812_Num)    //�����е�LED���ֱ�ֵ����Ӧ����ɫֵ��ȫ��LED��������������
        {
            j = 0;
            HAL_TIM_PWM_Stop_DMA(&WS2812_TIM, WS2812_TIM_Channel);   //ֹͣDMA
					  WS2812_En = 0;    //��־λ���㣬��Ҫ�ٴη�����
            return;
        }
        j++;     //LED������¼
        if (ins == 0)   //�ڸ���һ��LED��������(Buf0)��ͬʱ,�ѵڶ���LED������ת����ŵ���һ��������(Buf1),�ȴ��±߷���
        {
						HAL_TIM_PWM_Stop_DMA(&WS2812_TIM, WS2812_TIM_Channel);   //ֹͣDMA,�����д�����ʱ�����ɫ���趨����.  ��ʵ�����ɫ����Ԥ��Ҳ��������Ϊ0���1������ڼ��Ƚϼ��������ò����й�,�����Ȱѵ��Ƶ������Ϊ800K,���޸�ws2812.h�е�#define WS2812_Code_0 (32u) 0���1��ıȽ�ֵ
            HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_Channel, WS2812_SendBuf0, 25);
            WS2812_uint32ToData(WS2812_Data[j], WS2812_SendBuf1);
            ins = 1;
        }
        else     //���淢��,����Ƿ����ϱ�׼�����˵�buf1�����ݣ�ͬʱ�ѵ�3��LED������׼����buf0��...��˽���
        {
						HAL_TIM_PWM_Stop_DMA(&WS2812_TIM, WS2812_TIM_Channel);   //�������ɫ���趨ֵ����������Ҫ��ֹͣDMA�ٿ���.��ʵ�������ŵ���TIM1��800KHZ��"ws2812.h"��0��1��ıȽ�ֵΪ���ʵ�ֵ���Ҳ��Բ��У�����ֻ���ڴ˴��ء���MDA��
            HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_Channel, WS2812_SendBuf1, 25);
            WS2812_uint32ToData(WS2812_Data[j], WS2812_SendBuf0);
            ins = 0;
        }
    }
}
/**
 * @brief ��ʼ������ɫ����
 * @param ��
 * @return ��
 * @author HZ12138
 * @date 2022-10-03 18:05:13
 */
void WS2812_Start(void)
{
		HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_Channel, (uint32_t *)WS2812_Rst, 240);    //��������֮ǰ��Ҫ�ȷ�һ����λ��
	  WS2812_uint32ToData(WS2812_Data[0], WS2812_SendBuf0);     //�ѵ�һ��LEDӦ�ý��յ�����׼����Buf0�У��ȴ�����
    WS2812_En = 1;
}
/**
 * @brief ���͸�λ��
 * @param ��
 * @return ��
 * @author HZ12138
 * @date 2022-10-03 18:05:33
 */
void WS2812_Code_Reset(void)
{
    HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_Channel, (uint32_t *)WS2812_Rst, 240);
    WS2812_En = 0;
}

/*����LED�Ƶĸ���ģʽ���淨*/
//��ɫLED������,�ڲ�������װ
void led_off(void)
{
	for(int i=0;i<WS2812_Num;i++) WS2812_Data[i] = 0;
	WS2812_Start();
}
static void led_breath_R(void)
{
	static uint32_t color_step_R = 0;
	static int32_t step =0;    //Ϊ��ʵ�ֺ����ƴӴ�С�Լ����ٴ�С��������������������;
	if(0==color_step_R)  step = (0x05<<16);    //��������0x050000   
	else if(0xFF0000 == color_step_R) step = -0x050000;  //���ӵ�������𽥱�С���Լ�
		for(int i=0;i<255;i+=5)    //һ������ѭ��,����ú�����main��while���þͿ��Բ������ѭ��,�������Ҫ��RGB������Ͼ���Ҫ���������ѭ��,����RGBֻ��һ��ѭ���ͻ��,û�к���Ч��.    
		{                         //���������ѭ���������������������������һ�²��������Ч��.
			color_step_R += step;
			for(int i=0;i<WS2812_Num;i++) WS2812_Data[i] = color_step_R;
			WS2812_Start();
			HAL_Delay(30);
		}
	
}

static void led_breath_G(void)
{
	static uint32_t color_step_G = 0;
	static int32_t step =0;    
	if(0==color_step_G)  step = 0x000500;    //����   
	else if(0x00FF00 == color_step_G) step = -0x000500;  //���ӵ�������𽥱�С,�Լ�
	for(int i=0;i<255;i+=5)     //����ѭ��
	{
		color_step_G += step;  
		for(int i=0;i<WS2812_Num;i++) WS2812_Data[i] = color_step_G;
		WS2812_Start();     //����������,�ú����Ϳ���ֱ�ӱ�����
		HAL_Delay(30);
	}
	
}

static void led_breath_B(void)
{
	static uint32_t color_step_B = 0;
	static int32_t step =0;    
	if(0==color_step_B)  step = 0x000005;    //����   
	else if(0x0000FF == color_step_B) step = -0x000005; 
	for(int i=0;i<255;i+=5)    //�������������iȡ���ֵ,����ֻ�к���û����...
	{
		color_step_B += step;  
		for(int i=0;i<WS2812_Num;i++) WS2812_Data[i] = color_step_B;
		WS2812_Start();
		HAL_Delay(30);
	}
	
}


#if 0 //ʵ�ʲ��Է��֣����ǰ�RGB�ĺ���������װ�ɶ����ĺ��������������ʱ��forѭ���ķ�ʽ����Ч.����±����������û����.
const uint32_t color[] ={0xFF0000,0x00FF00,0x0000FF};     //extern��main����Ҳ��ʹ�ø�����

static void mono_breath_change(const uint32_t *mono_color,uint8_t index)
{
	static uint8_t step_state=0;    //�������������Լ���״̬,Ҳ����static,����ÿ�����㵽����1��״̬.
	static uint32_t color_step = 0; //����������,�������ó�static���ͣ�����ÿ�ν���������;��Ҫע����������Զ������,���ں�ƺ���������̵ƺ����ƺ���,�������һֱ�ӵ�0xFFFFFF���ǰ׵ƺ���(���Ǻ�����˳��),����������������
	                                //��˱�����ÿ�κ�LED������ɺ����������������.������led_mode_breath()������Ҫ��������,ֻ���ó�ȥ��Ϊȫ�ֱ�����.�����ֻ�ǵ�ɫ����������Ϊ�ֲ�static���ɣ�

	switch(step_state)
	{
		case 0:
			if(0xFF0000 == mono_color[index]){     //���
				color_step += 0x050000;
			  if(0xFF0000 == color_step) step_state = 1;
			}
			else if(0x00FF00 == mono_color[index]){   //�̵�
			color_step +=0x000500;
			if(0x00FF00 == color_step) step_state =1;    //�ı�״̬����״̬ 
			}
			else if(0x0000FF == mono_color[index])   //����
			{
				color_step +=0x000005;
				if(color_step == 0x0000FF) step_state = 1;   //���������255��Ҫ���Լ���0
			}		
			break; 
			
		case 1:
			if(0xFF0000 == mono_color[index])
			{
				color_step -= 0x050000;
				if(0 == color_step) step_state = 0;  //�Լ���0��Ҫ�ٴ�����
			}				
			else if(0x00FF00 == mono_color[index])
			{
				color_step -= 0x000500;
				if(0 == color_step) step_state = 0;
			}				
			else if(0x0000FF == mono_color[index])
			{
				color_step -=0x000005;
				if(0 == color_step) step_state = 0;
			}				
			break;
	}
	
	for(int i=0;i<WS2812_Num;i++) WS2812_Data[i] = color_step;
	//color_step = 0;	
	//WS2812_Start();      //���Ҫ�������øú���,û�����ϵͳû��Ӧ��;���˴��ú���ֻ���ڲ���װ,���Ӧ�÷�װ���±߱����ú����ڲ��Ÿ�����.
}
	#endif
	
//�����ƣ�����ǿ����ʱ������(RGBÿ����ɫ������0~255֮��仯;����0xFF0000��������״̬)
//�����ƣ�ʵ��RGB����ɫ�ʰ�˳�����
#if 0
void led_mode_breath(void)        //�汾A�������������ʵ��RGB˳�������ʵ����Ϻ��ɰ׵ƺ���
{
	static uint8_t flag = 0;
	//static uint32_t color = 0;    //�������ó�static���У�������ÿ��ѭ�����������㣬ֻ�ܼ�һ����Զ������255
	
	switch(flag)       //��ʾ�׹����,���ƺ�ֻ�������ĺ���û���Լ�������Ч��.
	{
		case 0:{led_off();mono_breath_change(color,0); flag=1;}
			break;
		case 1:{led_off();mono_breath_change(color,1); flag=2;}
			break;
		case 2:{led_off();mono_breath_change(color,2); flag=0;}
			break;
	}
	

	//������switch-caseЧ��һģһ����ֻ�а׹����
	/*if(!flag)  mono_breath_change(color,0);     //��ɫLED����	
	else if(flag == 1) mono_breath_change(color,1); //��ɫ����
	else if(flag == 2) mono_breath_change(color,2);	
	flag++;
	if(flag == 3) flag=0; */
	
	WS2812_Start();
	//HAL_Delay(50);     
}
#endif




#if 1
void led_mode_breath(void)    //RGB�������;�ص���led_breath_R()�����е�forѭ��,��Ȼ�����forѭ��(������ʱ)Ҳ�Ǳ�Ҫ��.
{
	static uint8_t flag = 0;
	//static uint32_t color = 0;    //�������ó�static���У�������ÿ��ѭ�����������㣬ֻ�ܼ�һ����Զ������255
	if(ledState == LED_RED)
	{
		//led_off();
		for(int i=0;i<6;i++)     //һ��һ��������,��������Ǻ�ƺ�������
		{
			led_breath_R();
		}
		led_off();     //�ر����е�LED,����.ʵ�ʲ��Է��ּӲ������ûɶ���.
	}		
	else if(ledState == LED_GREEN) {for(int j=0;j<6;j++)led_breath_G();}
	else if(ledState == LED_BLUE) {for(int k=0;k<6;k++)led_breath_B();}
	
	if(ledState == LED_RED) ledState=LED_GREEN;
	else if(ledState == LED_GREEN) ledState=LED_BLUE;
	else if(ledState == LED_BLUE) ledState=LED_RED;
	
	
	/*switch(flag)
	{
		case 0:led_breath_R();//flag=1;
			break;
		case 1:led_breath_B();flag=2;
			break;
		case 2:led_breath_G(); flag=0;
			break;
	}
	//flag++;*/
	
	
	/*
	if(flag == 3) flag=0; 
	if(!flag)led_breath_R();//��ɫLED����
	else if(flag == 1)led_breath_G(); 
	else if(flag == 2)led_breath_B();
	flag++;
	//if(flag == 3) flag=0; 
	*/

	//WS2812_Start();      //�Ѿ���Ƕ��led_breath_R()����
	//HAL_Delay(100);     
}
#endif

#if 0   //indexȫ�ֻ�����
void led_mode_breath(void)    //ֻ�ܵ�ɫ����,�����Զ�ת��������,��Ϊ�ֲ�������index���ܿ���ȫ������
{
	static uint8_t index = 0;  //����static
	static uint32_t color_step = 0; //����������,�������ó�static����
	static int32_t step = 0;  // �з�����ȷ����������
	
	switch(color[index])     //ֻ�ܺ�ɫ����,index�����Լ�������ɫ����ɫ����
	{
		case 0xff0000:               //R����
		{
			if(color_step==0) step=0x050000;
			else if(color_step==0xFF0000) step=-0x050000;
			color_step += step;
		}
		break; 
		case 0x00ff00:               //G����
			{
			if(color_step==0) step=0x000500;
			else if(color_step==0x00FF00) step=-0x000500;
			color_step += step;
		}
			break;
		case 0x0000FF:               //B����
			{
			if(color_step==0) step=0x000005;
			else if(color_step==0x0000FF) step=-0x000005;
			color_step += step;
		}
			break; 
	}
	index++;if(index == 3) index=0;       //Note:���index�Ǿֲ�������color[index]զ�����ⲿ�����飿
	for(int i=0;i<WS2812_Num;i++)WS2812_Data[i] = color_step;		
	WS2812_Start();  
	//HAL_Delay(50);	
}

#endif




//��ˮ��:RGBÿ����һ���ƺ���ÿ��ѭ���ѵ�ǰLED״̬��ɫ�ı�һ�ξ��γ���ˮ��Ч����
void led_mode_marquee(void)
{
	static uint32_t first = 0,second = 0,third = 0,flag = 0;
	if(3==flag)
	{
		flag = 0;
	}
	if(!flag)      //��ʼ״̬RGB 
	{
		first = 0xff0000;
		second= 0x00ff00;
		third = 0x0000ff;
	}
	if(1==flag)    //��һ��״̬GBR
	{
		first = 0x00ff00;
		second= 0x0000ff;
		third = 0xff0000;
	}
	else if(2==flag)  //���¸�״̬BRG
	{
		first = 0x0000ff;
		second= 0xff0000;
		third = 0x00ff00;
	}
	flag ++;
	for(int i=0;i<WS2812_Num;i+=3)     //RGBÿ����һ��
	{
		WS2812_Data[i+0] = first;
		WS2812_Data[i+1] = second;
		WS2812_Data[i+2] = third;
	}
	WS2812_Start();
	HAL_Delay(200);       //Ϊ�˺�����װ�Ķ�����,���ǰ������ʱ��������Ϻ�.���û����ʱ,RGBת��̫��ͳ��ְ�ɫ��
}

//�����趨LED�ĵ�Ƹ���,һ�����WS2812_Data[i]�о�����ֵ��,����Ҫ���i��ledϨ������Ҫ�����ر������
void led_light_count(uint32_t cnt)
{
	static uint32_t color[] = {0xff0000,0x00ff00,0x0000ff};    //�����ɫ������������ǰ�趨��,���ζ�ȡ�����е�ֵȥ��ʾ����.������ֻ��Blue��ʾ
	for (int i=0;i<WS2812_Num;i++)
	{
		WS2812_Data[i] = 0;
	}
	if(cnt > WS2812_Num)     
	{
		cnt = WS2812_Num;
	}
	
	for(int i=0;i<cnt;i+=3)     
	//for(int i=0;i<cnt;i++)     //Ҳ���Ե�ɫ,����ֱ��ָ����ɫ0x0000ff
	{
		//WS2812_Data[i] = color;
		WS2812_Data[i] = color[0];
		WS2812_Data[i+1] = color[1];
		WS2812_Data[i+2] = color[2];
	}
	WS2812_Start();
	HAL_Delay(50);     //���ڵ�ƵĿ���(��׽��������������ʱ50ms),�����ʱҲ����Ҫ,��÷�װ�ڴ�ʱ,������main���������Ͳ���
}

//��ʱ��(1ms)������ж�,_weak��������ʵ�֣�ע���������������HAL_TIM_Base_Start_IT(&htim2)����
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	static uint32_t cnt = 0;
	cnt++;
	if(cnt == 10)   //ÿ1ms�����ж�һ��,�˴���ʱ10ms
	{
		cnt = 0;
		//g_current_led_cnt--;    //�������������һ���Ʋ���Ϩ��,��Ϊһ����0������Ϊ10...
		if(g_current_led_cnt == 0)    //���ѭ������.
		{
			g_current_led_cnt = 10;     //WS2812_Num;         //10;
		} 
		g_current_led_cnt--;    //������������Ǹ���Ҳ��Ϩ���
	}
	
}
//��װ���ַ�Χ�Ƶĺ���
void led_mode_music(void)
{
	mic_get_adcValue();       //��microphone.h������
	led_light_count(g_current_led_cnt);    //���������������TIM2���жϷ���������޸ĵ�
}

void led_mode_select(void)
{
	ledMode_typ mode = g_mode;    //�Ѵ����жϽ��յ���ȫ�ֱ�����ֵ���ֲ�����
	switch(mode)
	{
		case Breath:
			led_mode_breath();
			break;
		
		case Marquee:
			led_mode_marquee();
			break;
		
		case Music:
			led_mode_music();   
		  break;
		
		default:
			printf("miss input!");
	}
		
}

//����Uart1�Ľ����ж�
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//��ʵ������һ��Ӧ�����ж��Ƿ���жϱ�־λ...�˴�ʡ����һ��
	if(RX_buff[0] == 'B')          //������
		g_mode = Breath;
	else if(RX_buff[0] == 'A')    //��ˮ��
		g_mode = Marquee;
	else if(RX_buff[0] == 'M')    //���ַ�Χ��
		g_mode = Music;
	else{}   //�û�ָ��δ�����򲻴���
		
	//�жϽ���һ�δ������,Ĭ�ϻ�ر��жϣ����������Ҫ�ٴο������ܽ�����һ���ж�
	HAL_UART_Receive_IT(&huart1,RX_buff,1);
}
