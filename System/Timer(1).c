#include "stm32f10x.h"                  // Device header

/**
  * @brief  定时器初始化函数，初始化TIM2为使用内部时钟的50ms定时器
  * @param  无
  * @retval 无
  */
void Timer_Init(void)
{
	//配置RCC
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    //通用计时器TIM2
	
	//选择时钟
	TIM_InternalClockConfig(TIM2);    //内部时钟
	
	//配置时基单元，20Hz，Feq = 72M / (PSC + 1) / (ARR + 1)
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;        //不分频
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;    //向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;                   //ARR自动重装器值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;                 //PSC预分频器值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;               //重复计数器值（高级定时器）
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	
	//配置中断
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);          //防止复位后立刻进入中断，清除中断标志位
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);     //使能TIM2更新中断
	
	//配置NVIC
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);     //分组2：2位抢占优先级，2位响应优先级
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;    //抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           //响应优先级1
	NVIC_Init(&NVIC_InitStructure);
	
	//使能定时器
	TIM_Cmd(TIM2, ENABLE);
}

/**
  * @brief  TIM2定时中断函数，复制到main
  * @param  无
  * @retval 无
  */
//void TIM2_IRQHandler(void)
//{
//	if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)    //判断是否为定时器TIM2更新中断
//	{
//		
//		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);    //清除定时器TIM2更新中断标志位 
//	}
//}
