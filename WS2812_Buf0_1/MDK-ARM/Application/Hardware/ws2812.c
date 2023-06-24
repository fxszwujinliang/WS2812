#include "WS2812.h"
#include "microphone.h"      //用于mic_get_adcValue()函数的声明

uint32_t WS2812_Data[WS2812_Num] = {0};   //用户数据区,WS2812_Num是彩带上的LED个数

uint32_t WS2812_SendBuf0[25] = {0};   //发送缓冲区0；在发送数据时，两个缓冲区交替使用;
uint32_t WS2812_SendBuf1[25] = {0};   //发送缓冲区1；在用A缓冲区给第一个LED发送数据时，用另一个B缓冲区准备数据，用于接下来给第二个LED的发送
const uint32_t WS2812_Rst[240] = {0}; //复位码缓冲区
uint32_t WS2812_En = 0;               //发送使能

LedState ledState = LED_RED;     //呼吸灯状态变量的初始化

/**
 * @brief 将uint32转为发送的数据；将24bits的颜色值转换成pwm占空比
 * @param Data:颜色数据
 * @param Ret:解码后的数据(PWM占空比)
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
    zj = (G << 16) | (R << 8) | B;       //这句就把main函数中设定的RGB顺序调整成LED硬件要求的GRB顺序了
    for (int i = 0; i < 24; i++)     //假设颜色数据0xff0000红色（此时zj已经变成三个uint8_t组合成的数据了;0xFF0000）
    {
        if (zj & (1 << 23))
            Ret[i] = WS2812_Code_1;  //开始时,0xFF0000与(1<<23)按位与,结果为1，则Ret[0]=71
        else
            Ret[i] = WS2812_Code_0;   //左移到第9位时，位与的结果为0,进入这个分支,则Ret[8]=32
        zj <<= 1;   //把颜色数据左移一位后继续位与
    }
    Ret[24] = 0;      //最后第25位上又补一个0,是为了在发送完每一帧数据后，保留一定时间空隙；告诉CPU已经结束了.
}
/**
 * @brief 发送函数(DMA中断调用)
 * @param 无
 * @return 无
 * @author HZ12138
 * @date 2022-10-03 18:04:50
 */
void WS2812_Send(void)     //在定时器PWM中断内被调用执行
{
    static uint32_t j = 0;
    static uint32_t ins = 0;
    if (WS2812_En == 1)
    {
			if (j == WS2812_Num)    //把所有的LED都分别赋值了相应的颜色值，全部LED都发送数据完了
        {
            j = 0;
            HAL_TIM_PWM_Stop_DMA(&WS2812_TIM, WS2812_TIM_Channel);   //停止DMA
					  WS2812_En = 0;    //标志位清零，不要再次发送了
            return;
        }
        j++;     //LED个数记录
        if (ins == 0)   //在给第一个LED发送数据(Buf0)的同时,把第二个LED的数据转换后放到另一个缓冲区(Buf1),等待下边发送
        {
						HAL_TIM_PWM_Stop_DMA(&WS2812_TIM, WS2812_TIM_Channel);   //停止DMA,如果不写这句有时点灯颜色与设定不符.  其实点灯颜色不符预期也可能是因为0码和1码的周期及比较计数器设置不对有关,可以先把点灯频率设置为800K,再修改ws2812.h中的#define WS2812_Code_0 (32u) 0码和1码的比较值
            HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_Channel, WS2812_SendBuf0, 25);
            WS2812_uint32ToData(WS2812_Data[j], WS2812_SendBuf1);
            ins = 1;
        }
        else     //交替发送,这段是发送上边准备好了的buf1中数据，同时把第3个LED的数据准备到buf0中...如此交替
        {
						HAL_TIM_PWM_Stop_DMA(&WS2812_TIM, WS2812_TIM_Channel);   //若点灯颜色与设定值不符，则需要先停止DMA再开启.其实可以试着调整TIM1的800KHZ和"ws2812.h"中0码1码的比较值为合适的值（我测试不行，所以只能在此处关、开MDA）
            HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_Channel, WS2812_SendBuf1, 25);
            WS2812_uint32ToData(WS2812_Data[j], WS2812_SendBuf0);
            ins = 0;
        }
    }
}
/**
 * @brief 开始发送颜色数据
 * @param 无
 * @return 无
 * @author HZ12138
 * @date 2022-10-03 18:05:13
 */
void WS2812_Start(void)
{
		HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_Channel, (uint32_t *)WS2812_Rst, 240);    //发送数据之前需要先发一个复位码
	  WS2812_uint32ToData(WS2812_Data[0], WS2812_SendBuf0);     //把第一个LED应该接收的数据准备到Buf0中，等待发送
    WS2812_En = 1;
}
/**
 * @brief 发送复位码
 * @param 无
 * @return 无
 * @author HZ12138
 * @date 2022-10-03 18:05:33
 */
void WS2812_Code_Reset(void)
{
    HAL_TIM_PWM_Start_DMA(&WS2812_TIM, WS2812_TIM_Channel, (uint32_t *)WS2812_Rst, 240);
    WS2812_En = 0;
}

/*操作LED灯的各种模式、玩法*/
//纯色LED呼吸灯,内部函数封装
void led_off(void)
{
	for(int i=0;i<WS2812_Num;i++) WS2812_Data[i] = 0;
	WS2812_Start();
}
static void led_breath_R(void)
{
	static uint32_t color_step_R = 0;
	static int32_t step =0;    //为了实现呼吸灯从大到小自减，再从小到大自增的正反两个数;
	if(0==color_step_R)  step = (0x05<<16);    //自增幅度0x050000   
	else if(0xFF0000 == color_step_R) step = -0x050000;  //增加到最大再逐渐变小，自减
		for(int i=0;i<255;i+=5)    //一个呼吸循环,如果该函数被main中while调用就可以不用这个循环,但是如果要和RGB呼吸组合就需要在这里加入循环,否则RGB只有一个循环就混合,没有呼吸效果.    
		{                         //经测试这个循环变量好像必须和上面的亮度增量一致才有理想的效果.
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
	if(0==color_step_G)  step = 0x000500;    //自增   
	else if(0x00FF00 == color_step_G) step = -0x000500;  //增加到最大再逐渐变小,自减
	for(int i=0;i<255;i+=5)     //呼吸循环
	{
		color_step_G += step;  
		for(int i=0;i<WS2812_Num;i++) WS2812_Data[i] = color_step_G;
		WS2812_Start();     //这句放在这里,该函数就可以直接被调用
		HAL_Delay(30);
	}
	
}

static void led_breath_B(void)
{
	static uint32_t color_step_B = 0;
	static int32_t step =0;    
	if(0==color_step_B)  step = 0x000005;    //自增   
	else if(0x0000FF == color_step_B) step = -0x000005; 
	for(int i=0;i<255;i+=5)    //经测试这里如果i取别的值,好像只有呼而没有吸...
	{
		color_step_B += step;  
		for(int i=0;i<WS2812_Num;i++) WS2812_Data[i] = color_step_B;
		WS2812_Start();
		HAL_Delay(30);
	}
	
}


#if 0 //实际测试发现，还是把RGB的呼吸单独封装成独立的函数，另调用它的时候for循环的方式才有效.因此下边这个函数就没用了.
const uint32_t color[] ={0xFF0000,0x00FF00,0x0000FF};     //extern让main函数也能使用该数组

static void mono_breath_change(const uint32_t *mono_color,uint8_t index)
{
	static uint8_t step_state=0;    //表征自增还是自减的状态,也必须static,否则每次清零到不了1的状态.
	static uint32_t color_step = 0; //自增减变量,必须设置成static类型，避免每次进来被清零;但要注意正是它永远不清零,你在红灯呼吸后继续绿灯和蓝灯呼吸,这个变量一直加到0xFFFFFF就是白灯呼吸(不是红绿蓝顺序),与我们期望不符合
	                                //因此必须在每次红LED呼吸完成后把这个变量清零才行.由于在led_mode_breath()函数中要对它清零,只好拿出去作为全局变量了.（如果只是单色呼吸可以作为局部static即可）

	switch(step_state)
	{
		case 0:
			if(0xFF0000 == mono_color[index]){     //红灯
				color_step += 0x050000;
			  if(0xFF0000 == color_step) step_state = 1;
			}
			else if(0x00FF00 == mono_color[index]){   //绿灯
			color_step +=0x000500;
			if(0x00FF00 == color_step) step_state =1;    //改变状态机的状态 
			}
			else if(0x0000FF == mono_color[index])   //蓝灯
			{
				color_step +=0x000005;
				if(color_step == 0x0000FF) step_state = 1;   //自增到最大255就要再自减到0
			}		
			break; 
			
		case 1:
			if(0xFF0000 == mono_color[index])
			{
				color_step -= 0x050000;
				if(0 == color_step) step_state = 0;  //自减到0就要再次自增
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
	//WS2812_Start();      //如果要单独调用该函数,没有这句系统没反应的;但此处该函数只能内部封装,这句应该封装到下边被调用函数内部才更合理.
}
	#endif
	
//呼吸灯：发光强度随时间增加(RGB每种颜色都可以0~255之间变化;比如0xFF0000就是最红的状态)
//呼吸灯：实现RGB三种色彩按顺序呼吸
#if 0
void led_mode_breath(void)        //版本A：这个函数不能实现RGB顺序呼吸，实测组合后变成白灯呼吸
{
	static uint8_t flag = 0;
	//static uint32_t color = 0;    //必须设置成static才行！！否则每次循环进来都清零，只能加一次永远到不了255
	
	switch(flag)       //显示白光呼吸,但似乎只有自增的呼，没有自减的吸的效果.
	{
		case 0:{led_off();mono_breath_change(color,0); flag=1;}
			break;
		case 1:{led_off();mono_breath_change(color,1); flag=2;}
			break;
		case 2:{led_off();mono_breath_change(color,2); flag=0;}
			break;
	}
	

	//与上面switch-case效果一模一样：只有白光呼吸
	/*if(!flag)  mono_breath_change(color,0);     //红色LED呼吸	
	else if(flag == 1) mono_breath_change(color,1); //绿色呼吸
	else if(flag == 2) mono_breath_change(color,2);	
	flag++;
	if(flag == 3) flag=0; */
	
	WS2812_Start();
	//HAL_Delay(50);     
}
#endif




#if 1
void led_mode_breath(void)    //RGB交替呼吸;重点是led_breath_R()函数中的for循环,当然这里的for循环(呼吸此时)也是必要的.
{
	static uint8_t flag = 0;
	//static uint32_t color = 0;    //必须设置成static才行！！否则每次循环进来都清零，只能加一次永远到不了255
	if(ledState == LED_RED)
	{
		//led_off();
		for(int i=0;i<6;i++)     //一呼一吸算两次,因此这里是红灯呼吸三次
		{
			led_breath_R();
		}
		led_off();     //关闭所有的LED,清零.实际测试发现加不加这句没啥差别.
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
	if(!flag)led_breath_R();//红色LED呼吸
	else if(flag == 1)led_breath_G(); 
	else if(flag == 2)led_breath_B();
	flag++;
	//if(flag == 3) flag=0; 
	*/

	//WS2812_Start();      //已经内嵌到led_breath_R()中了
	//HAL_Delay(100);     
}
#endif

#if 0   //index全局化？？
void led_mode_breath(void)    //只能单色呼吸,不会自动转换到绿蓝,因为局部变量的index不能控制全局数组
{
	static uint8_t index = 0;  //必须static
	static uint32_t color_step = 0; //自增减变量,必须设置成static类型
	static int32_t step = 0;  // 有符号数确定增减方向
	
	switch(color[index])     //只能红色呼吸,index不会自己跳到绿色和蓝色处！
	{
		case 0xff0000:               //R呼吸
		{
			if(color_step==0) step=0x050000;
			else if(color_step==0xFF0000) step=-0x050000;
			color_step += step;
		}
		break; 
		case 0x00ff00:               //G呼吸
			{
			if(color_step==0) step=0x000500;
			else if(color_step==0x00FF00) step=-0x000500;
			color_step += step;
		}
			break;
		case 0x0000FF:               //B呼吸
			{
			if(color_step==0) step=0x000005;
			else if(color_step==0x0000FF) step=-0x000005;
			color_step += step;
		}
			break; 
	}
	index++;if(index == 3) index=0;       //Note:这个index是局部变量，color[index]咋控制外部的数组？
	for(int i=0;i<WS2812_Num;i++)WS2812_Data[i] = color_step;		
	WS2812_Start();  
	//HAL_Delay(50);	
}

#endif




//流水灯:RGB每三个一组点灯后再每次循环把当前LED状态颜色改变一次就形成流水灯效果了
void led_mode_marquee(void)
{
	static uint32_t first = 0,second = 0,third = 0,flag = 0;
	if(3==flag)
	{
		flag = 0;
	}
	if(!flag)      //初始状态RGB 
	{
		first = 0xff0000;
		second= 0x00ff00;
		third = 0x0000ff;
	}
	if(1==flag)    //下一种状态GBR
	{
		first = 0x00ff00;
		second= 0x0000ff;
		third = 0xff0000;
	}
	else if(2==flag)  //再下个状态BRG
	{
		first = 0x0000ff;
		second= 0xff0000;
		third = 0x00ff00;
	}
	flag ++;
	for(int i=0;i<WS2812_Num;i+=3)     //RGB每三个一组
	{
		WS2812_Data[i+0] = first;
		WS2812_Data[i+1] = second;
		WS2812_Data[i+2] = third;
	}
	WS2812_Start();
	HAL_Delay(200);       //为了函数封装的独立性,还是把这个延时放在这里较好.如果没有延时,RGB转的太快就呈现白色了
}

//自由设定LED的点灯个数,一旦点灯WS2812_Data[i]中就有数值了,若需要这个i的led熄灭则需要另行特别操作它
void led_light_count(uint32_t cnt)
{
	static uint32_t color[] = {0xff0000,0x00ff00,0x0000ff};    //点灯颜色可以用数组提前设定好,依次读取数组中的值去显示即可.这例子只用Blue演示
	for (int i=0;i<WS2812_Num;i++)
	{
		WS2812_Data[i] = 0;
	}
	if(cnt > WS2812_Num)     
	{
		cnt = WS2812_Num;
	}
	
	for(int i=0;i<cnt;i+=3)     
	//for(int i=0;i<cnt;i++)     //也可以单色,比如直接指定蓝色0x0000ff
	{
		//WS2812_Data[i] = color;
		WS2812_Data[i] = color[0];
		WS2812_Data[i+1] = color[1];
		WS2812_Data[i+2] = color[2];
	}
	WS2812_Start();
	HAL_Delay(50);     //调节点灯的快慢(捕捉声音的灵敏度延时50ms),这个延时也很重要,最好封装在此时,避免在main中忘记这句就不行
}

//定时器(1ms)溢出的中断,_weak函数的再实现；注意必须在主函数中HAL_TIM_Base_Start_IT(&htim2)才行
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	static uint32_t cnt = 0;
	cnt++;
	if(cnt == 10)   //每1ms进来中断一次,此处延时10ms
	{
		cnt = 0;
		//g_current_led_cnt--;    //放在这里则最后一个灯不会熄灭,因为一旦归0就设置为10...
		if(g_current_led_cnt == 0)    //如此循环起来.
		{
			g_current_led_cnt = 10;     //WS2812_Num;         //10;
		} 
		g_current_led_cnt--;    //放在这里最后那个灯也会熄灭的
	}
	
}
//封装音乐氛围灯的函数
void led_mode_music(void)
{
	mic_get_adcValue();       //在microphone.h中声明
	led_light_count(g_current_led_cnt);    //这个参数是在上面TIM2的中断服务程序中修改的
}

void led_mode_select(void)
{
	ledMode_typ mode = g_mode;    //把串口中断接收到的全局变量赋值给局部变量
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

//串口Uart1的接收中断
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//其实进来第一步应该先判断是否该中断标志位...此处省略这一步
	if(RX_buff[0] == 'B')          //呼吸灯
		g_mode = Breath;
	else if(RX_buff[0] == 'A')    //流水灯
		g_mode = Marquee;
	else if(RX_buff[0] == 'M')    //音乐氛围灯
		g_mode = Music;
	else{}   //用户指令未定义则不处理
		
	//中断进来一次处理完后,默认会关闭中断，因此这里需要再次开启才能接收下一次中断
	HAL_UART_Receive_IT(&huart1,RX_buff,1);
}
