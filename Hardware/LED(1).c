#include "stm32f10x.h"                  // Device header

/*引脚配置*/
#define LED1         GPIO_Pin_0    //PA0
#define LED2         GPIO_Pin_2    //PA2

#define LED_N            2    //LED数量
#define LED_GPIOA        1             
#define LED_GPIOB        2
#define LED_GPIOC        3
#define LEDi             0x0001 << (i - 1)

uint8_t  LED_GPIO[LED_N + 1];
uint8_t  LED_Select[LED_N + 1];
uint8_t  LED_Mode[LED_N + 1];
uint16_t LED_Pin[LED_N + 1];
uint16_t LED_Count[LED_N + 1];

/**
  * @brief  LED初始化函数，初始化PA0、PA2为LED输出
  * @param  无
  * @retval 无
  */
void LED_Init(void)
{
	//配置RCC
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	//配置GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    //推挽输出模式
	GPIO_InitStructure.GPIO_Pin = LED1 | LED2;          //多个端口之间相或
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	GPIO_InitStructure.GPIO_Pin = LED;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	GPIO_InitStructure.GPIO_Pin = LED;
//	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOA, LED1 | LED2);    //多个端口之间相或
//	GPIO_SetBits(GPIOB, LED);
//	GPIO_SetBits(GPIOC, LED);
	
	LED_GPIO[1] = LED_GPIOA;  LED_Pin[1] = LED1;
	LED_GPIO[2] = LED_GPIOA;  LED_Pin[2] = LED2;
//	LED_GPIO[] = LED_GPIO;  LED_Pin[] = LED;        //多个LED复制此处
}

/**
  * @brief  LED解码函数
  * @param  Code LED编码
  * @retval 无
  */
void LED_Decode(uint16_t Code)
{
	uint8_t i, p = 0;

	for(i = 0; i < LED_N; i++)
    {
		if(Code & (0x0001 << i))
		{
			LED_Select[p++] = i + 1;
		}
	}
	LED_Select[p] = 0;
}

/**
  * @brief  点亮LED函数
  * @param  Code LED编码
  * @retval 无
  */
void LED_ON(uint16_t Code)
{
	uint8_t i, Num;

	LED_Decode(Code);
	for(i = 0; i < LED_N; i++)
	{
		Num = LED_Select[i];
		if(Num == 0)  {break;}
		switch(LED_GPIO[Num])
		{
			case LED_GPIOA: {GPIO_ResetBits(GPIOA, LED_Pin[Num]); break;}
			case LED_GPIOB: {GPIO_ResetBits(GPIOB, LED_Pin[Num]); break;}
			case LED_GPIOC: {GPIO_ResetBits(GPIOC, LED_Pin[Num]); break;}
		}
	}
}

/**
  * @brief  熄灭LED函数
  * @param  Code LED编码
  * @retval 无
  */
void LED_OFF(uint16_t Code)
{
	uint8_t i, Num;

	LED_Decode(Code);
	for(i = 0; i < LED_N; i++)
	{
		Num = LED_Select[i];
		if(Num == 0)  {break;}
		switch(LED_GPIO[Num])
		{
			case LED_GPIOA: {GPIO_SetBits(GPIOA, LED_Pin[Num]); break;}
			case LED_GPIOB: {GPIO_SetBits(GPIOB, LED_Pin[Num]); break;}
			case LED_GPIOC: {GPIO_SetBits(GPIOC, LED_Pin[Num]); break;}
		}
	}
}

/**
  * @brief  反转LED函数
  * @param  Code LED编码
  * @retval 无
  */
void LED_Turn(uint16_t Code)
{
	uint8_t i, Num;

	LED_Decode(Code);
	for(i = 0; i < LED_N; i++)
	{
		Num = LED_Select[i];
		if(Num == 0)  {break;}
		switch(LED_GPIO[Num])
		{
			case LED_GPIOA:
			{
				if(GPIO_ReadOutputDataBit(GPIOA, LED_Pin[Num]) == 0)  {GPIO_SetBits(GPIOA, LED_Pin[Num]);}
				else  {GPIO_ResetBits(GPIOA, LED_Pin[Num]);} break;
			}
			case LED_GPIOB:
			{
				if(GPIO_ReadOutputDataBit(GPIOB, LED_Pin[Num]) == 0)  {GPIO_SetBits(GPIOB, LED_Pin[Num]);}
				else  {GPIO_ResetBits(GPIOB, LED_Pin[Num]);} break;
			}
			case LED_GPIOC:
			{
				if(GPIO_ReadOutputDataBit(GPIOC, LED_Pin[Num]) == 0)  {GPIO_SetBits(GPIOC, LED_Pin[Num]);}
				else  {GPIO_ResetBits(GPIOC, LED_Pin[Num]);} break;
			}
		}
	}
}

/**
  * @brief  LED模式切换函数
  * @param  GPIO_Pin LED所在端口号
  * @param  Mode 要切换的模式
  * @retval 无
  */
void LED_SetMode(uint16_t Code, uint8_t Mode)
{
	uint8_t i, Num;

	LED_Decode(Code);
	for(i = 0; i < LED_N; i++)
	{
		Num = LED_Select[i];
		if(Mode != LED_Mode[Num])
		{
			LED_Mode[Num] = Mode;
			LED_Count[Num] = 0;
		}
	}
}

/**
  * @brief  LED定时中断函数，由main中的1ms定时中断调用
  * @param  无
  * @retval 无
  */
void LED_Tick(void)
{
	uint8_t i;
	
	for(i = 1; i <= LED_N; i++)
	{
		switch(LED_Mode[i])
		{
			case 0:  {LED_OFF(LEDi); break;}    //模式0：常灭
			case 1:  {LED_ON(LEDi); break;}     //模式1：常亮
			case 2:    //模式2：慢闪模式，亮500ms，灭500ms
			{
				LED_Count[i]++;
				LED_Count[i] %= 1000;
				if(LED_Count[i] < 500)  {LED_ON(LEDi);}
				else                    {LED_OFF(LEDi);}
				break;
			}
			case 3:    //模式3：快闪模式，亮50ms，灭50ms
			{
				LED_Count[i]++;
				LED_Count[i] %= 100;
				if(LED_Count[i] < 50)  {LED_ON(LEDi);}
				else                   {LED_OFF(LEDi);}
				break;
			}
			case 4:    //模式4：点闪模式，亮100ms，灭900ms
			{
				LED_Count[i]++;
				LED_Count[i] %= 1000;
				if(LED_Count[i] < 100)  {LED_ON(LEDi);}
				else                    {LED_OFF(LEDi);}
				break;
			}
		}
	}
}
