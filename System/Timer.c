#include "stm32f10x.h"                  // Device header

/**
  * @brief  ��ʱ����ʼ����������ʼ��TIM2Ϊʹ���ڲ�ʱ�ӵ�50ms��ʱ��
  * @param  ��
  * @retval ��
  */
void Timer_Init(void)
{
	//����RCC
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    //ͨ�ü�ʱ��TIM2
	
	//ѡ��ʱ��
	TIM_InternalClockConfig(TIM2);    //�ڲ�ʱ��
	
	/* 定时频率：72MHz / (PSC+1) / (ARR+1)
	 * 原先 PSC=71, ARR=999 → 1000Hz，中断里跑软件 I2C 会导致 OLED 严重卡顿
	 * 现改为 100Hz（10ms），与 DMP 100Hz 采样匹配 */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;      /* ARR */
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;    /* PSC: 72M/720/1000 = 100Hz */
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	
	//�����ж�
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);          //��ֹ��λ�����̽����жϣ�����жϱ�־λ
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);     //ʹ��TIM2�����ж�
	
	//����NVIC
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);     //����2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;    //��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           //��Ӧ���ȼ�1
	NVIC_Init(&NVIC_InitStructure);
	
	//ʹ�ܶ�ʱ��
	TIM_Cmd(TIM2, ENABLE);
}

/**
  * @brief  TIM2��ʱ�жϺ��������Ƶ�main
  * @param  ��
  * @retval ��
  */
//void TIM2_IRQHandler(void)
//{
//	if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)    //�ж��Ƿ�Ϊ��ʱ��TIM2�����ж�
//	{
//		
//		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);    //�����ʱ��TIM2�����жϱ�־λ 
//	}
//}
