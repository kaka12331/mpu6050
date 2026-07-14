#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>

/*引脚配置*/
#define Serial_TX        GPIO_Pin_9     //PA9
#define Serial_RX        GPIO_Pin_10    //PA10

uint8_t Serial_RxData;    //接收数据
uint8_t Serial_RxFlag;    //接收完成标志位

/**
  * @brief  USART串口初始化函数，初始化PA9为发送，PA10为接收 
  * @param  无
  * @retval 无
  */
void Serial_Init(void)
{
	//配置RCC
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	//配置GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   //复用推挽输出模式
	GPIO_InitStructure.GPIO_Pin = Serial_TX;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;     //上拉输入模式
	GPIO_InitStructure.GPIO_Pin = Serial_RX;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//配置USART
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 9600;                                         //设置波特率9600
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;    //不使用数据流
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                    //发送+接收
	USART_InitStructure.USART_Parity = USART_Parity_No;                                //无校验位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;                             //停止位1
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;                        //数据位8
	USART_Init(USART1, &USART_InitStructure);
	
	//配置中断
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);    //使能USART串口接收中断 
	
	//配置NVIC
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);     //分组2：2位抢占优先级，2位响应优先级
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;    //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           //响应优先级1
	NVIC_Init(&NVIC_InitStructure);
	
	//使能USART
	USART_Cmd(USART1, ENABLE);  
}
/**
  * @brief  USART串口发送一个字节数据函数
  * @param  Byte 要发送的一个字节数据
  * @retval 无
  */
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);                                   //写TDR数据
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);    //等待TDR空，发送完成
}

/**
  * @brief  USART串口发送多个字节数据函数
  * @param  Array 要发送的多个字节数据起始地址
  * @param  Length 要发送的字节数据个数
  * @retval 无
  */
void Serial_SendArry(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	
	for(i = 0; i < Length; i++)
	{
		Serial_SendByte(Array[i]);
	}
}

/**
  * @brief  USART串口发送字符串函数
  * @param  String 要发送的字符串，范围：ASCII可见字符
  * @retval 无
  */
void Serial_SendString(char *String)
{
	uint8_t i;

	for(i = 0; String[i] != '\0'; i++)
	{
		Serial_SendByte(String[i]);
	}
}

/**
  * @brief  USART串口次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;

	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  USART串口发送数字函数，以字符形式显示
  * @param  Number 要发送的数字，范围：0 ~ 4294967295
  * @param  Length 要发送数字的长度，范围：1 ~ 10
  * @retval 无
  */
void Serial_SendNumber(uint32_t Number,uint8_t Length)
{
	uint8_t i;
	for(i = 0; i < Length; i++)
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  USART串口使用printf函数打印格式化字符串函数
  * @param  format 指定要显示的格式化字符串，范围：ASCII码可见字符或中文字符组成的字符串
  * @param  ... 格式化字符串参数列表
  */
 void Serial_Printf(char *format, ...)
 {
	 char String[100];    //定义字符数组
	 va_list arg;         //定义可变参数列表数据类型的变量arg
	 
	 va_start(arg, format);            //从format开始，接收参数列表到arg变量
	 vsprintf(String, format, arg);    //使用vsprintf打印格式化字符串和参数列表到字符数组中
	 va_end(arg);                      //结束变量arg
	 Serial_SendString(String);        //USART串口发送字符串
 }

/**
  * @brief  USART串口获取接收完成标志位函数，读后清零
  * @param  无
  * @retval 返回接收完成标志位，1可读取数据，0接收未完成
  */
uint8_t Serial_GetRxFlag(void)
{
	if(Serial_RxFlag == 1)
	{
		Serial_RxFlag = 0;
		return 1;
	}
	return 0;
}

/**
  * @brief  USART串口获取接收数据函数
  * @param  无
  * @retval 返回接收数据
  */
uint8_t Serial_GetRxData(void)
{
	return Serial_RxData;
}

/**
  * @brief  USART串口接收中断函数
  * @param  无
  * @retval 无
  */
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)    //判断是否为USART串口接收中断
	{
		Serial_RxData = USART_ReceiveData(USART1);         //读RDR数据
		Serial_RxFlag = 1;
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);    //清除USART串口接收中断标志位 
	}
}
