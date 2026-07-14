#include "stm32f10x.h"                  // Device header
#include "Key.h"

/*引脚配置*/
#define Key1        GPIO_Pin_0     //PB0
#define Key2        GPIO_Pin_10    //PB10

#define Key_N            2    //按键数量
#define Key_GPIOA        1             
#define Key_GPIOB        2

uint8_t  Key_GPIO[Key_N + 1];
uint8_t  Key_Code[Key_N + 1];
uint16_t Key_Pin[Key_N + 1];

/**
  * @brief  按键初始化函数，初始化PB0、PB10为按键输入
  * @param  无
  * @retval 无
  */
void Key_Init(void)
{
	//配置RCC
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	//配置GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	 //上拉输入模式
	GPIO_InitStructure.GPIO_Pin = Key1 | Key2;       //多个端口之间相或
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	GPIO_InitStructure.GPIO_Pin = Key;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	Key_GPIO[1] = Key_GPIOB;  Key_Pin[1] = Key1;
	Key_GPIO[2] = Key_GPIOB;  Key_Pin[2] = Key2;
//	Key_GPIO[] = Key_GPIO;  Key_Pin[] = Key;        //多个按键复制此处
}

/**
  * @brief  读取指定按键引脚电平函数
  * @param  Num 指定按键编码
  * @retval 返回指定按键的引脚电平
  */
uint8_t Key_ReadPin(uint8_t Num)
{
	if(Num >= 1 && Num <= Key_N)
	{
		switch(Key_GPIO[Num])
		{
			case Key_GPIOA: {return GPIO_ReadInputDataBit(GPIOA, Key_Pin[Num]);}
			case Key_GPIOB: {return GPIO_ReadInputDataBit(GPIOB, Key_Pin[Num]);}
		}
	}
	return 1;
}

/**
  * @brief  查看指定按键事件是否发生函数
  * @param  Num 指定按键编码
  * @param  Event 按键事件
  * @retval 返回指定按键事件是否发生，1表示发生，0表示未发生
  */
uint8_t Key_Check(uint8_t Num, uint8_t Event)
{
	if(Num >= 1 && Num <= Key_N)
	{
		if(Key_Code[Num] & Event)    //发生指定按键事件
		{
			Key_Code[Num] &= ~Event;    //清除事件
			return 1;
		}
	}
	return 0;
}

/**
  * @brief  按键事件清空函数
  * @param  无
  * @retval 无
  */
void Key_Clear(void)
{
	uint8_t i;
	
	for (i = 1; i <= Key_N; i++)
	{
		Key_Code[i] = 0;
	}
}

/**
  * @brief  按键定时中断函数，由main中的1ms定时中断调用
  * @param  无
  * @retval 无
  */
void Key_Tick(void)
{
	static uint8_t Count;
	static uint8_t CurrState[Key_N + 1];
	static uint8_t PrevState[Key_N + 1];
	static uint8_t S[Key_N + 1];
	static uint8_t Key_Count[Key_N + 1];
	uint8_t i;
	
	Count++;
	if(Count >= 20)    //每隔20ms执行一次
	{
		Count = 0;
		
		for(i = 1; i <= Key_N; i++)
		{
			PrevState[i] = CurrState[i];         //保存上次状态
			CurrState[i] = Key_ReadPin(i);       //保存本次状态
			
			//状态机思想实现按键事件判别
			switch(S[i])
			{
				case 0:    //S = 0：初始状态，等待按键按下
				{
					if(PrevState[i] == 1 && CurrState[i] == 0)    //按键按下
					{
						S[i] = 1;
						Key_Count[i] = 0;
					}
					break;
				}
				case 1:    //S = 1：按键按下状态
				{
					Key_Count[i]++;
					if(Key_Count[i] >= 50)    //按键按下超过50 * 20 = 1000ms，判定为长按
					{
						S[i] = 0;
						Key_Code[i] |= KEY_LONG;
					}
					if(PrevState[i] == 0 && CurrState[i] == 1)    //在1000ms内松手
					{
						S[i] = 2;
						Key_Count[i] = 0;
					}
					break;
				}
				case 2:    //S = 2：单双击判断状态
				{
					Key_Count[i]++;
					if(Key_Count[i] >= 10)    //超过10 * 20 = 200ms未按下按键，判定为单击
					{
						S[i] = 0;
						Key_Code[i] |= KEY_CLICK;
					}
					if(PrevState[i] == 0 && CurrState[i] == 1)    //在200ms内再次按下按键并松手，判定为双击
					{
						S[i] = 0;
						Key_Code[i] |= KEY_DOUBLE;
					}
					break;
				}
			}
		}
	}
}
