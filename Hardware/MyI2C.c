#include "stm32f10x.h"                  // Device header
#include "Delay.h"

/*引脚配置*/
#define MyI2C_SCL        GPIO_Pin_10   //PB10
#define MyI2C_SDA        GPIO_Pin_11   //PB11

/**
  * @brief  I2C写SCL函数
  * @param  BitValue 要写入的值，范围：0、1
  * @retval 无
  */
void MyI2C_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOB, MyI2C_SCL, (BitAction)BitValue);
	Delay_us(10);
}

/**
  * @brief  I2C写SDA函数
  * @param  BitValue 要写入的值，范围：0、1
  * @retval 无
  */
void MyI2C_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOB, MyI2C_SDA, (BitAction)BitValue);
	Delay_us(10);
}

/**
  * @brief  I2C读SDA函数
  * @param  无
  * @retval 返回读取的值，范围：0、1
  */
uint8_t MyI2C_R_SDA(void)
{
	uint8_t BitValue;

	BitValue = GPIO_ReadInputDataBit(GPIOB, MyI2C_SDA);
	Delay_us(10);
	return BitValue;
}

/**
  * @brief  I2C初始化函数，初始化PB10为I2C_SCL输出，PB11为I2C_SDA输出
  * @param  无
  * @retval 无
  */
void MyI2C_Init(void)
{
	//使能GPIOB时钟和复用功能（要先打开复用才能修改复用功能）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);	 
	//要先开时钟，再重映射；这句表示关闭JTAG，使能SWD。
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);						 
	//配置GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;   //开漏输出模式
	GPIO_InitStructure.GPIO_Pin = MyI2C_SCL | MyI2C_SDA;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOB, MyI2C_SCL | MyI2C_SDA);
}

/**
  * @brief  I2C起始函数，SCL高电平期间，SDA由高电平切换到低电平
  * @param  无
  * @retval 无
  */
void MyI2C_Start(void)
{
	MyI2C_W_SDA(1);
	MyI2C_W_SCL(1);
	MyI2C_W_SDA(0);
	MyI2C_W_SCL(0);
}

/**
  * @brief  I2C终止函数，SCL高电平期间，SDA由低电平切换到高电平
  * @param  无
  * @retval 无
  */
void MyI2C_Stop(void)
{
	MyI2C_W_SDA(0);
	MyI2C_W_SCL(1);
	MyI2C_W_SDA(1);
}

/**
  * @brief  I2C发送一个字节函数，SCL低电平期间，主机将数据位依次放到SDA上（高位在前），拉高SCL，循环8次，
  *         从机在SCL高电平期间读取数据位
  * @param  Byte 要发送的字节
  * @retval 无
  */
void MyI2C_SendByte(uint8_t Byte)
{
	uint8_t i;

	for(i = 0; i < 8; i++)
	{
		MyI2C_W_SDA(Byte & (0x80 >> i));
		MyI2C_W_SCL(1);
		MyI2C_W_SCL(0);
	}
}

/**
  * @brief  I2C接收一个字节函数，SCL低电平期间，从机将数据位依次放到SDA上（高位在前），拉高SCL，循环8次，
  *         主机在SCL高电平期间读取数据位（主机在接收之前，需要释放SDA）
  * @param  无
  * @retval 返回接收的字节
  */
uint8_t MyI2C_ReceiveByte(void)
{
	uint8_t i, Byte = 0x00;

	MyI2C_W_SDA(1);
	for(i = 0; i < 8; i++)
	{
		MyI2C_W_SCL(1);
		if(MyI2C_R_SDA() == 1)  {Byte |= (0x80 >> i);}
		MyI2C_W_SCL(0);
	}
	return Byte;
}

/**
  * @brief  I2C发送应答函数，在接收完一个字节之后，主机在下一个时钟发送应答
  * @param  AckBit 要发送的应答位，0表示应答，1表示非应答
  * @retval 
  */
void MyI2C_SendAck(uint8_t AckBit)
{
	MyI2C_W_SDA(AckBit);
	MyI2C_W_SCL(1);
	MyI2C_W_SCL(0);
}

/**
  * @brief  I2C接收应答函数，在发送完一个字节之后，主机在下一个时钟接收一位数据，判断从机是否应答
  *         （主机在接收之前，需要释放SDA）
  * @param  无
  * @retval 返回接收的应答位，0表示应答，1表示非应答
  */
uint8_t MyI2C_ReceiveAck(void)
{
	uint8_t AckBit;
	
	MyI2C_W_SDA(1);
	MyI2C_W_SCL(1);
	AckBit = MyI2C_R_SDA();
	MyI2C_W_SCL(0);
	return AckBit;
}
