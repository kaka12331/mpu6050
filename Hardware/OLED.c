#include "stm32f10x.h"
#include "OLED.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

/**
  * ���ݴ洢��ʽ��
  * ����8�㣬��λ���£��ȴ����ң��ٴ��ϵ���
  * ÿһ��Bit��Ӧһ�����ص�
  * 
  *      B0 B0                  B0 B0
  *      B1 B1                  B1 B1
  *      B2 B2                  B2 B2
  *      B3 B3  ------------->  B3 B3 --
  *      B4 B4                  B4 B4  |
  *      B5 B5                  B5 B5  |
  *      B6 B6                  B6 B6  |
  *      B7 B7                  B7 B7  |
  *                                    |
  *  -----------------------------------
  *  |   
  *  |   B0 B0                  B0 B0
  *  |   B1 B1                  B1 B1
  *  |   B2 B2                  B2 B2
  *  --> B3 B3  ------------->  B3 B3
  *      B4 B4                  B4 B4
  *      B5 B5                  B5 B5
  *      B6 B6                  B6 B6
  *      B7 B7                  B7 B7
  * 
  * �����ᶨ�壺
  * ���Ͻ�Ϊ(0, 0)��
  * ��������ΪX�ᣬȡֵ��Χ��0~127
  * ��������ΪY�ᣬȡֵ��Χ��0~63
  * 
  *       0             X��           127 
  *      .------------------------------->
  *    0 |
  *      |
  *      |
  *      |
  *  Y�� |
  *      |
  *      |
  *      |
  *   63 |
  *      v
  * 
  */


/*ȫ�ֱ���*********************/

/**
  * OLED�Դ�����
  * ���е���ʾ��������ֻ�ǶԴ��Դ�������ж�д
  * ������OLED_Update������OLED_UpdateArea����
  * �ŻὫ�Դ���������ݷ��͵�OLEDӲ����������ʾ
  */
uint8_t OLED_DisplayBuf[8][128];

/*********************ȫ�ֱ���*/


/*��������*********************/

/*��������*/
#define OLED_SCL       GPIO_Pin_8    //PB8
#define OLED_SDA       GPIO_Pin_9    //PB9

/**
  * @brief  OLEDдSCL����
  * @param  Ҫд��SCL�ĵ�ƽֵ����Χ��0/1
  * @retval ��
  */
void OLED_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOB, OLED_SCL, (BitAction)BitValue);
	/* 极短延时，提高软件 I2C 兼容性（部分 OLED 模块较慢） */
	{
		volatile uint8_t t = 2;
		while (t--);
	}
}

/**
  * @brief  OLED写SDA电平
  * @param  要写入SDA的电平值，范围：0/1
  * @retval 无
  */
void OLED_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOB, OLED_SDA, (BitAction)BitValue);
	{
		volatile uint8_t t = 2;
		while (t--);
	}
}

/**
  * @brief  I2C��ʼ����������ʼ��PB8ΪI2C_SCL�����PB9ΪI2C_SDA���
  * @param  ��
  * @retval ��
  */
void OLED_I2C_Init(void)
{
	uint32_t i,j;
	
	//��ʼ��ǰ������ʸ����ʱ����OLED�����ȶ�
	for(i = 0; i < 1000; i++)
	{
		for(j = 0; j < 1000; j++);
	}
	
	//����RCC
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	//����GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;    //��©���ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = OLED_SCL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = OLED_SDA;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	//�ͷ�SCL��SDA
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/*********************��������*/


/*ͨ��Э��*********************/

/**
  * @brief  I2C��ʼ����
  * @param  ��
  * @retval ��
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
  * @brief  I2Cֹͣ����
  * @param  ��
  * @retval ��
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C����һ���ֽں���
  * @param  Byte Ҫ���͵�һ���ֽ�
  * @retval ��
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	
	for(i = 0; i < 8; i++)
	{
		OLED_W_SDA(Byte & (0x80 >> i));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}
	OLED_W_SCL(1);    //�����һ��ʱ�ӣ�������Ӧ���ź� 
	OLED_W_SCL(0);
}

/**
  * @brief  OLEDд�����
  * @param  Command Ҫд�������
  * @retval ��
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);    //�ӻ���ַ
	OLED_I2C_SendByte(0x00);    //д����
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
  * @brief  OLEDд���ݺ���
  * @param  Data Ҫд�����ݵ���ʼ��ַ
  * @param  Count Ҫд�����ݵ�����
  * @retval ��
  */
void OLED_WriteData(uint8_t *Data, uint8_t Count)
{
	uint8_t i;
	
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);    //�ӻ���ַ
	OLED_I2C_SendByte(0x40);    //д����
	for(i = 0; i < Count; i++)
	{
		OLED_I2C_SendByte(Data[i]);
	}
	OLED_I2C_Stop();
}

/*ͨ��Э��*********************/


/*Ӳ������*********************/

/**
  * @brief  OLED��ʼ������ 
  * @param  �� 
  * @retval �� 
  */
void OLED_Init(void)
{	
	OLED_I2C_Init();            //�˿ڳ�ʼ��
	
	OLED_WriteCommand(0xAE);	//������ʾ����/�رգ�0xAE�رգ�0xAF����
	
	OLED_WriteCommand(0xD5);    //������ʾʱ�ӷ�Ƶ��/����Ƶ��
	OLED_WriteCommand(0x80);    //0x00 ~ 0xFF
	
	OLED_WriteCommand(0xA8);    //���ö�·������
	OLED_WriteCommand(0x3F);    //0x0E ~ 0x3F
	
	OLED_WriteCommand(0xD3);    //������ʾƫ�� 
	OLED_WriteCommand(0x00);    //0x00 ~ 0x7F
	
	OLED_WriteCommand(0x40);    //������ʾ��ʼ�У�0x40 ~ 0x7F
	
	OLED_WriteCommand(0xA1);    //�������ҷ���0xA1������0xA0���ҷ���
	
	OLED_WriteCommand(0xC8);    //�������·���0xC8������0xC0���·���

	OLED_WriteCommand(0xDA);    //����COM����Ӳ������
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);    //���öԱȶȿ���
	OLED_WriteCommand(0xCF);    //0x00 ~ 0xFF

	OLED_WriteCommand(0xD9);    //����Ԥ������� 
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);    //����VCOMHȡ��ѡ�񼶱� 
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);    //����������ʾ��/�ر� 

	OLED_WriteCommand(0xA6);    //��������/��ɫ��ʾ��0xA6������0xA7��ɫ

	OLED_WriteCommand(0x8D);    //���ó���
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);    //������ʾ
		
	OLED_Clear();               //����Դ�����
	OLED_Update();              //������ʾ����������ֹ��ʼ����δ��ʾ����ʱ����
}

/**
  * @brief  OLED���ù��λ�ú��� 
  * @param  Page ָ���������ҳ����Χ��0 ~ 7
  * @param  X ָ��������ڵ�X�����꣬��Χ��0 ~ 127
  * @retval ��
  */
void OLED_SetCursor(uint8_t Page, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Page);					//����Yλ�� 
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//����Xλ�ø�4λ 
	OLED_WriteCommand(0x00 | (X & 0x0F));			//����Xλ�õ�4λ 
}

/*********************Ӳ������*/


/*���ߺ���*********************/

/*���ߺ��������ڲ����ֺ���ʹ��*/

/**
  * @brief  OLED�η�����
  * @param  X ����
  * @param  Y ָ��
  * @retval ����X��Y�η� 
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  �ж�ָ�����Ƿ���ָ��������ڲ������㷨��W. Randolph Franklin���
  * @param  nvert ����εĶ�����
  * @param  vertx verty ��������ζ����x��y���������
  * @param  testx testy ���Ե��X��y����
  * @retval ָ�����Ƿ���ָ��������ڲ���1���ڲ���0�����ڲ�
  */
uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
	int16_t i, j, c = 0;
	
	for(i = 0, j = nvert - 1; i < nvert; j = i++)
	{
		if(((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
		{
			c = !c;
		}
	}
	return c;
}

/**
  * @brief  �ж�ָ�����Ƿ���ָ���Ƕ��ڲ�
  * @param  X Y ָ���������
  * @param  StartAngle EndAngle ��ʼ�ǶȺ���ֹ�Ƕȣ���Χ��-180 ~ 180
  *         ˮƽ����Ϊ0�ȣ�ˮƽ����Ϊ180�Ȼ�-180�ȣ��·�Ϊ�������Ϸ�Ϊ������˳ʱ����ת
  * @retval ָ�����Ƿ���ָ���Ƕ��ڲ���1���ڲ���0�����ڲ���Ĭ��ԭ���ڽǶ��ڲ�
  */
uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
	int16_t PointAngle;
	
	PointAngle = atan2(Y, X) / 3.14 * 180;    //����ָ����Ļ��ȣ���ת��Ϊ�Ƕȱ�ʾ
	if(StartAngle > EndAngle)    //ȷ��StartAngleС�ڵ���EndAngle
	{
		StartAngle = -StartAngle;
		EndAngle = -EndAngle;
		PointAngle = -PointAngle;
	}
	if((PointAngle >= StartAngle && PointAngle <= EndAngle) || (X == 0 && Y == 0))  {return 1;}
	return 0;
}

/*********************���ߺ���*/


/*���ܺ���*********************/

/**
  * @brief  OLED���º�������OLED�Դ�������µ�OLED��Ļ
  * @param  ��
  * @retval ��
  */
void OLED_Update(void)
{
	uint8_t j;
	
	for(j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		OLED_WriteData(OLED_DisplayBuf[j], 128);
	}
}

/**
  * @brief  OLED������º�������OLED�Դ����鲿�ָ��µ�OLED��Ļ��
  *         �˺��������ٸ��²���ָ�������������������Y��ֻ��������ҳ����ͬһҳ��ʣ�ಿ�ֻ����һ�����
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Width ָ������Ŀ��ȣ���Χ��0 ~ 128
  * @param  Height ָ������ĸ߶ȣ���Χ��0 ~ 64
  * @retval ��
  */
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t j;
	int16_t Page, Page1;
	
	Page = Y / 8;
	Page1 = (Y + Height - 1) / 8 + 1;
	if(Y < 0)    //���������ڼ���ҳ��ַʱ��Ҫ��һ��ƫ��
	{
		Page -= 1;
		Page1 -= 1;
	}
	
	for(j = Page; j < Page1; j++)    //����ָ�������漰�����ҳ
	{
		if(X >= 0 && X <= 127 && j >= 0 && j <= 7)    //������Ļ�����ݲ���ʾ
		{
			OLED_SetCursor(j, X);
			OLED_WriteData(&OLED_DisplayBuf[j][X], Width);
		}
	}
}

/**
  * @brief  OLED�����������OLED�Դ�����ȫ�����㣬
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  ��
  * @retval ��
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	
	for (j = 0; j < 8; j++)    //����ָ��ҳ
	{
		for(i = 0; i < 128; i++)    //����ָ����
		{
			OLED_DisplayBuf[j][i] = 0x00;
		}
	}
}

/**
  * @brief   OLED���������������OLED�Դ����鲿�����㣬
  *          ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Width ָ������Ŀ��ȣ���Χ��0 ~ 128
  * @param  Height ָ������ĸ߶ȣ���Χ��0 ~ 64
  * @retval ��
  */
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t i, j;
	
	for(j = Y; j < Y + Height; j++)    //����ָ��ҳ
	{
		for(i = X; i < X + Width; i++)    //����ָ����
		{
			if(i >= 0 && i <= 127 && j >= 0 && j <= 63)    //������Ļ���ݲ���ʾ
			{
				OLED_DisplayBuf[j / 8][i] &= ~(0x01 << (j % 8));
			}
		}
	}
}

/**
  * @brief  OLED��ɫ��������OLED�Դ�����ȫ��ȡ��
            ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  ��
  * @retval ��
  */
void OLED_Reverse(void)
{
	uint8_t i, j;
	
	for(j = 0; j < 8; j++)    //����ָ��ҳ
	{
		for(i = 0; i < 128; i++)    //����ָ����
		{
			OLED_DisplayBuf[j][i] ^= 0xFF;
		}
	}
}

/**
  * @brief   OLED����ɫ��������OLED�Դ����鲿�����㣬
             ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Width ָ������Ŀ��ȣ���Χ��0 ~ 128
  * @param  Height ָ������ĸ߶ȣ���Χ��0 ~ 64
  * @retval ��
  */
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t i, j;
	
	for(j = Y; j < Y + Height; j++)    //����ָ��ҳ
	{
		for(i = X; i < X + Width; i++)    //����ָ����
		{
			if(i >= 0 && i <= 127 && j >= 0 && j <= 63)    //������Ļ���ݲ���ʾ
			{
				OLED_DisplayBuf[j / 8][i] ^= 0x01 << (j % 8);
			}
		}
	}
}

/**
  * @brief  OLED��ʾһ���ַ�����
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���ַ����Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���ַ����Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Char ָ��Ҫ��ʾ���ַ�����Χ��ASCII�ɼ��ַ�
  * @param  FontSize ָ�������С
  *         ��Χ��OLED_8X16		��8���أ���16����
  *               OLED_6X8		��6���أ���8����
  * @retval ��
  */
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize)
{      	
	if(FontSize == OLED_8X16)		//����Ϊ��8���أ���16����
	{
		OLED_ShowImage(X, Y, 8, 16, OLED_F8x16[Char - ' ']);
	}
	else if(FontSize == OLED_6X8)	//����Ϊ��6���أ���8����
	{
		OLED_ShowImage(X, Y, 6, 8, OLED_F6x8[Char - ' ']);
	}
}

/**
  * @brief  OLED��ʾ�ַ���������֧��ASCII������Ļ��д�룬�����ַ�����ΪGB2312��
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  *         ��ʾ�������ַ���Ҫ��OLED_Data.c���OLED_CF16x16���鶨��
  *         δ�ҵ�ָ�������ַ�ʱ������ʾĬ��ͼ�Σ�һ�������ڲ�һ���ʺţ�
  *         �������СΪOLED_8X16ʱ�������ַ���16*16����������ʾ
  *         �������СΪOLED_6X8ʱ�������ַ���6*8������ʾ'?'
  * @param  X ָ���ַ������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���ַ������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  String ָ��Ҫ��ʾ���ַ�������Χ��ASCII��ɼ��ַ���GB2312�����ַ���ɵ��ַ���
  * @param  FontSize ָ�������С
  *         ��Χ��OLED_8X16		��8���أ���16����
  *               OLED_6X8		��6���أ���8����
  * @retval �� 
  */
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
	uint16_t i = 0;
	char SingleChar[3];
	uint8_t CharLength = 0;
	uint16_t XOffset = 0;
	uint16_t pIndex;
	
	while(String[i] != '\0')	//�����ַ���
	{
		//�˶δ����Ŀ���ǣ���ȡGB2312�ַ����е�һ���ַ���ת�浽SingleChar���ַ�����
		//�ж�GB2312�ֽڵ����λ��־λ
		if((String[i] & 0x80) == 0x00)    //���λΪ0
		{
			CharLength = 1;                 //�ַ�Ϊ1�ֽ�
			SingleChar[0] = String[i++];    //����һ���ֽ�д��SingleChar��0��λ�ã����iָ����һ���ֽ�
			SingleChar[1] = '\0';           //ΪSingleChar�����ַ���������־λ
		}
		else    //���λΪ1
		{
			CharLength = 2;                    //�ַ�Ϊ2�ֽ�
			SingleChar[0] = String[i++];       //����һ���ֽ�д��SingleChar��0��λ�ã����iָ����һ���ֽ�
			if(String[i] == '\0')  {break;}    //�������������ѭ����������ʾ
			SingleChar[1] = String[i++];       //���ڶ����ֽ�д��SingleChar��1��λ�ã����iָ����һ���ֽ�
			SingleChar[2] = '\0';              //ΪSingleChar�����ַ���������־λ
		}
		
		//��ʾ����������ȡ����SingleChar
		if(CharLength == 1)    //����ǵ��ֽ��ַ�
		{
			OLED_ShowChar(X + XOffset, Y, SingleChar[0], FontSize);
			XOffset += FontSize;
		}
		else    //���򣬼����ֽ��ַ�
		{
			//����������ģ�⣬����ģ����Ѱ�Ҵ��ַ�������
			//����ҵ����һ���ַ�������Ϊ���ַ����������ʾ�ַ�δ����ģ�ⶨ�壬ֹͣѰ��
			for(pIndex = 0; strcmp(OLED_CF16x16[pIndex].Index, "") != 0; pIndex++)
			{
				//�ҵ�ƥ����ַ���������ѭ������ʱpIndex��ֵΪָ���ַ�������
				if(strcmp(OLED_CF16x16[pIndex].Index, SingleChar) == 0)  {break;}
			}
			if(FontSize == OLED_8X16)    //��������Ϊ8*16����
			{
				OLED_ShowImage(X + XOffset, Y, 16, 16, OLED_CF16x16[pIndex].Data);
				XOffset += 16;
			}
			else if(FontSize == OLED_6X8)    //��������Ϊ6*8����
			{
				//�ռ䲻�㣬��λ����ʾ'?'
				OLED_ShowChar(X + XOffset, Y, '?', OLED_6X8);
				XOffset += OLED_6X8;
			}
		}
	}
}

/**
  * @brief  OLED��ʾ���ֺ�����ʮ���ƣ���������
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Number ָ��Ҫ��ʾ�����֣���Χ��0 ~ 4294967295
  * @param  Length ָ�����ֵĳ��ȣ���Χ��1 ~ 10
  * @param  FontSize ָ�������С
  *         ��Χ��OLED_8X16		��8���أ���16����
  *               OLED_6X8		��6���أ���8����
  * @retval �� 
  */
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	
	for(i = 0; i < Length; i++)    //����ÿһλ����
	{
		OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
	}
}

/**
  * @brief  OLED��ʾ���ֺ�����ʮ���ƣ�������
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Number ָ��Ҫ��ʾ�����֣���Χ��-2147483648 ~ 2147483647
  * @param  Length ָ�����ֵĳ��ȣ���Χ��1 ~ 10
  * @param  FontSize ָ�������С
  *         ��Χ��OLED_8X16		��8���أ���16����
  *               OLED_6X8		��6���أ���8����
  * @retval �� 
  */
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	
	if(Number >= 0)
	{
		OLED_ShowChar(X, Y, '+', FontSize);    //��ʾ����
	}
	else
	{
		OLED_ShowChar(X, Y, '-', FontSize);    //��ʾ����
		Number = -Number;
	}
	for(i = 0; i < Length; i++)    //����ÿһλ����
	{
		OLED_ShowChar(X + (i + 1) * FontSize, Y, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
	}
}

/**
  * @brief  OLED��ʾ���ֺ�����ʮ�����ƣ��������� 
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Number ָ��Ҫ��ʾ�����֣���Χ��0x00000000 ~ 0xFFFFFFFF
  * @param  Length ָ�����ֵĳ��ȣ���Χ��0 ~ 8
  * @param  FontSize ָ�������С
  *         ��Χ��OLED_8X16		��8���أ���16����
  *               OLED_6X8		��6���أ���
  * @retval �� 
  */
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i, SingleNumber;
	
	for(i = 0; i < Length; i++)    //����ÿһλ����
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if(SingleNumber < 10)    //��������С��10
		{
			OLED_ShowChar(X + i * FontSize, Y, SingleNumber + '0', FontSize);
		}
		else    //�������ִ��ڵ���10
		{
			OLED_ShowChar(X + i * FontSize, Y, SingleNumber - 10 + 'A', FontSize);
		}
	}
}

/**
  * @brief  OLED��ʾ���ֺ����������ƣ��������� 
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Number ָ��Ҫ��ʾ�����֣���Χ��0x00000000 ~ 0xFFFFFFFF
  * @param  Length ָ�����ֵĳ��ȣ���Χ��0 ~ 16
  * @param  FontSize ָ�������С
  *         ��Χ��OLED_8X16		��8���أ���16����
  *               OLED_6X8		��6���أ���
  * @retval �� 
  */
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	
	for(i = 0; i < Length; i++)    //����ÿһλ����
	{
		OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
	}
}

/**
  * @brief  OLED��ʾ������������ʮ���ƣ�С���� 
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Number ָ��Ҫ��ʾ�����֣���Χ��-4294967295.0~4294967295.0
  * @param  IntLength ָ�����ֵ�����λ���ȣ���Χ��0 ~ 10
  * @param  FraLength ָ�����ֵ�С��λ���ȣ���Χ��0 ~ 9��С����������������ʾ
  * @param  FontSize ָ�������С
  *         ��Χ��OLED_8X16		��8���أ���16����
  *               OLED_6X8		��6���أ���
  * @retval �� 
  */
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
	uint32_t PowNum, IntNum, FraNum;
	
	if (Number >= 0)
	{
		OLED_ShowChar(X, Y, '+', FontSize);    //��ʾ����
	}
	else
	{
		OLED_ShowChar(X, Y, '-', FontSize);    //��ʾ����
		Number = -Number;
	}
	
	//��ȡ�������ֺ�С������
	IntNum = Number;                     //ֱ�Ӹ�ֵ�����ͱ�������ȡ����
	Number -= IntNum;                    //��Number��������������ֹ֮��С���˵�����ʱ����������ɴ���
	PowNum = OLED_Pow(10, FraLength);    //����ָ��С����λ����ȷ������
	FraNum = round(Number * PowNum);     //��С���˵�������ͬʱ�������룬������ʾ���
	IntNum += FraNum / PowNum;           //��������������˽�λ������Ҫ�ټӸ�����
	
	OLED_ShowNum(X + FontSize, Y, IntNum, IntLength, FontSize);                      //��ʾ��������
	OLED_ShowChar(X + FontSize * (IntLength + 1), Y, '.', FontSize);                 //��ʾС����
	OLED_ShowNum(X + FontSize * (IntLength + 2), Y, FraNum, FraLength, FontSize);    //��ʾС������
}

/**
  * @brief  OLED��ʾͼ����
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ��ͼ�����Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ��ͼ�����Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Width ָ��ͼ��Ŀ��ȣ���Χ��0 ~ 128
  * @param  Height ָ��ͼ��ĸ߶ȣ���Χ��0 ~ 64
  * @param  Image ָ��Ҫ��ʾ��ͼ��
  * @retval ��
  */
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
	uint8_t i = 0, j = 0;
	int16_t Page, Shift;
	
	OLED_ClearArea(X, Y, Width, Height);    //��ͼ�������������
	
	for(j = 0; j < (Height - 1) / 8 + 1; j++)    //����ָ��ͼ���漰�����ҳ
	{
		for(i = 0; i < Width; i++)    //����ָ��ͼ���漰�������
		{
			if(X + i >= 0 && X + i <= 127)    //������Ļ�����ݲ���ʾ
			{
				Page = Y / 8;
				Shift = Y % 8;
				if(Y < 0)    //���������ڼ���ҳ��ַ����λʱ��Ҫ��һ��ƫ��
				{
					Page -= 1;
					Shift += 8;
				}
				if(Page + j >= 0 && Page + j <= 7)    //������Ļ�����ݲ���ʾ
				{
					//��ʾͼ���ڵ�ǰҳ������
					OLED_DisplayBuf[Page + j][X + i] |= Image[j * Width + i] << (Shift);
				}
				if(Page + j + 1 >= 0 && Page + j + 1 <= 7)    //������Ļ�����ݲ���ʾ
				{
					//��ʾͼ���ڵ�ǰҳ������
					OLED_DisplayBuf[Page + j + 1][X + i] |= Image[j * Width + i] >> (8 - Shift);
				}
			}
		}
	}
}

/**
  * @brief  OLEDʹ��printf������ӡ��ʽ���ַ���������֧��ASCII������Ļ��д�룩
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  *         ��ʾ�������ַ���Ҫ��OLED_Data.c���OLED_CF16x16���鶨��
  *         δ�ҵ�ָ�������ַ�ʱ������ʾĬ��ͼ�Σ�һ�������ڲ�һ���ʺţ�
  *         �������СΪOLED_8X16ʱ�������ַ���16*16����������ʾ
  *         �������СΪOLED_6X8ʱ�������ַ���6*8������ʾ'?'
  * @param  X ָ����ʽ���ַ������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ����ʽ���ַ������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  FontSize ָ�������С
  *         ��Χ��OLED_8X16		��8���أ���16����
  *               OLED_6X8		��6���أ���8����
  * @param  format ָ��Ҫ��ʾ�ĸ�ʽ���ַ�������Χ��ASCII��ɼ��ַ��������ַ���ɵ��ַ���
  * @param  ... ��ʽ���ַ��������б�
  * @retval ��
  */
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
	char String[256];    //�����ַ�����
	va_list arg;         //����ɱ�����б��������͵ı���arg
	
	va_start(arg, format);                      //��format��ʼ�����ղ����б���arg����
	vsprintf(String, format, arg);              //ʹ��vsprintf��ӡ��ʽ���ַ����Ͳ����б����ַ�������
	va_end(arg);                                //��������arg
	OLED_ShowString(X, Y, String, FontSize);    //OLED��ʾ�ַ����飨�ַ�����
}

/**
  * @brief  OLED��ָ��λ�û��㺯��
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ����ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ����������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @retval ��
  */
void OLED_DrawPoint(int16_t X, int16_t Y)
{
	if(X >= 0 && X <= 127 && Y >=0 && Y <= 63)		//������Ļ�����ݲ���ʾ
	{
		OLED_DisplayBuf[Y / 8][X] |= 0x01 << (Y % 8);
	}
}

/**
  * @brief  OLED��ȡָ��λ�õ�ֵ����
  * @param  X ָ����ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ����������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63 
  * @retval ָ��λ���Ƿ��ڵ���״̬��1������0Ϩ��
  */
uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
	if(X >= 0 && X <= 127 && Y >=0 && Y <= 63)		//������Ļ�����ݲ���ȡ
	{
		if(OLED_DisplayBuf[Y / 8][X] & 0x01 <<(Y % 8))
		{
			return 1;
		}
	}
	return 0;
}

/**
  * @brief  OLED���ߺ�����ʹ��Bresenham�㷨��ֱ�ߣ����Ա����ʱ�ĸ������㣬Ч�ʸ���
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X0 ָ��һ���˵�ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y0 ָ��һ���˵�������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  X1 ָ����һ���˵�ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y1 ָ����һ���˵�������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @retval ��
  */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
	int16_t x, y, dx, dy, d, incrE, incrNE, temp;
	int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
    uint8_t yflag = 0, xyflag = 0;
	
	if(y0 == y1)    //���ߵ�������
	{
		//0��X�������1�ŵ�X���꣬�򽻻������X����
		if(x0 > x1)  {temp = x0; x0 = x1; x1 = temp;}
		for(x = x0; x <= x1; x++)
		{
			OLED_DrawPoint(x, y0);
		}
	}
	else if(x0 == x1)    //���ߵ�������
	{
		//0��Y�������1�ŵ�Y���꣬�򽻻������Y����
		if(y0 > y1)  {temp = y0; y0 = y1; y1 = temp;}
		for(y = y0; y <= y1; y++)
		{
			OLED_DrawPoint(x0, y);
		}
	}
	else    //б�ߴ���������Bresenham��ֱ���㷨
	{    
		//0��X�������1�ŵ�X���꣬�򽻻��������꣬��Ӱ�컭�ߣ������߷����޶�Ϊ��һ��������
		if(x0 > x1)
		{
			temp = x0; x0 = x1; x1 = temp;
			temp = y0; y0 = y1; y1 = temp;
		}
		
		//0��Y�������1�ŵ�Y���꣬��Y����ȡ����Ӱ�컭�ߣ������߷����޶�Ϊ��һ����
		if(y0 > y1)
		{
			y0 = -y0;
			y1 = -y1;
			yflag = 1;    //��yflag��־λ����ס��ǰ�任���ں���ʵ�ʻ���ʱ���ٽ����껻����
		}
		
		//����б�ʴ���1���򽻻�X�����Y���꣬Ӱ�컭�ߣ������߷����޶�Ϊ��һ����0 ~ 45�ȷ�Χ
		if(y1 - y0 > x1 - x0)
		{
			temp = x0; x0 = y0; y0 = temp;
			temp = x1; x1 = y1; y1 = temp;
			xyflag = 1;    //��xyflag��־λ����ס��ǰ�任���ں���ʵ�ʻ���ʱ���ٽ����껻����
		}
		
		//Bresenham��ֱ���㷨���㷨Ҫ���߷������Ϊ��һ����0 ~ 45�ȷ�Χ
		dx = x1 - x0;
		dy = y1 - y0;
		incrE = 2 * dy;
        incrNE = 2 * (dy - dx);
        d = 2 * dy - dx;
        x = x0; y = y0;
		
		//����ʼ�㣬ͬʱ�жϱ�־λ�������껻����
		if(yflag && xyflag)  {OLED_DrawPoint(y, -x);}
		else if(yflag)       {OLED_DrawPoint(x, -y);}
		else if(xyflag)      {OLED_DrawPoint(y, x);}
		else                 {OLED_DrawPoint(x, y);}
		
		while(x < x1)    //����x��ÿ����
		{
			x++;
			if(d < 0)    //��һ�����ڵ�ǰ�㶫��
			{
				d += incrE;
			}
			else    //��һ�����ڵ�ǰ�㶫����
			{
				y++;
				d += incrNE;
			}
			
			//ÿ��һ���㣬ͬʱ�жϱ�־λ�������껻����
			if(yflag && xyflag)  {OLED_DrawPoint(y, -x);}
			else if(yflag)       {OLED_DrawPoint(x, -y);}
			else if(xyflag)      {OLED_DrawPoint(y, x);}
			else                 {OLED_DrawPoint(x, y);}
		}
	}
}

/**
  * @brief  OLED�����κ���
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ���������Ͻǵĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ���������Ͻǵ������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Width ָ�����εĿ��ȣ���Χ��0 ~ 128
  * @param  Height ָ�����εĸ߶ȣ���Χ��0 ~ 64
  * @param  IsFilled ָ�������Ƿ����
  *         ��Χ��OLED_UNFILLED		�����
  *               OLED_FILLED		���
  * @retval ��
  */
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
	int16_t i, j;
	
	OLED_DrawLine(X, Y, X + Width - 1, Y);
	OLED_DrawLine(X, Y + Height - 1, X + Width - 1, Y + Height - 1);
	OLED_DrawLine(X, Y, X, Y + Height - 1);
	OLED_DrawLine(X + Width - 1, Y, X + Width - 1, Y + Height - 1);

	if(IsFilled)    //ָ�����������//ָ���������
	{
		for(i = X + 1; i < X + Width - 1; i++)
		{
			for (j = Y + 1; j < Y + Height - 1; j++)
			{
				OLED_DrawPoint(i, j);
			}
		}
	}
}

/**
  * @brief  OLED�������κ���
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X0 ָ����һ���˵�ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y0 ָ����һ���˵�������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  X1 ָ���ڶ����˵�ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y1 ָ���ڶ����˵�������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  X2 ָ���������˵�ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y2 ָ���������˵�������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  IsFilled ָ�������Ƿ����
  *         ��Χ��OLED_UNFILLED		�����
  *               OLED_FILLED		���
  * @retval ��
  */
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
	int16_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
	int16_t i, j;
	int16_t vx[] = {X0, X1, X2};
	int16_t vy[] = {Y0, Y1, Y2};
	
	OLED_DrawLine(X0, Y0, X1, Y1);
	OLED_DrawLine(X0, Y0, X2, Y2);
	OLED_DrawLine(X1, Y1, X2, Y2);

	if(IsFilled)    //ָ�����������
	{
		//�ҵ�����������С��X��Y����
		if(X1 < minx)  {minx = X1;}
		if(X2 < minx)  {minx = X2;}
		if(Y1 < miny)  {miny = Y1;}
		if(Y2 < miny)  {miny = Y2;}
		
		//�ҵ�������������X��Y����
		if(X1 > maxx)  {maxx = X1;}
		if(X2 > maxx)  {maxx = X2;}
		if(Y1 > maxy)  {maxy = Y1;}
		if(Y2 > maxy)  {maxy = Y2;}
		
		//��С�������֮��ľ���Ϊ������Ҫ�������򣬱��������������еĵ�
		for(i = minx; i <= maxx; i++)
		{
			for(j = miny; j <= maxy; j++)
			{
				//��������������ڲ����򻭵㣬������ڣ���������
				if(OLED_pnpoly(3, vx, vy, i, j))  {OLED_DrawPoint(i, j);}
			}
		}
	}
}

/**
  * @brief  OLED��Բ������ʹ��Bresenham�㷨��Բ�����Ա��ⲿ�ֺ�ʱ�ĸ������㣬Ч�ʸ���
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ��Բ��Բ�ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ��Բ��Բ�������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Radius ָ��Բ�İ뾶����Χ��0 ~ 255
  * @param  IsFilled ָ��Բ�Ƿ����
  *         ��Χ��OLED_UNFILLED		�����
  *               OLED_FILLED		���
  * @retval ��
  */
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
	int16_t x, y, d;
	
	//Bresenham��Բ�㷨��ֻ����һ����45 ~ 90�ȷ�Χ�����ಿ�ֶԳ�
	d = 1 - Radius;
	x = 0;
	y = Radius;
	
	//��ÿ���˷�֮һԲ������ʼ��
	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X + x, Y - y);
	OLED_DrawPoint(X + y, Y + x);
	OLED_DrawPoint(X - y, Y + x);
	
	if(IsFilled)    //ָ��Բ���
	{
		OLED_DrawLine(X + x, Y - y + 1, X + x, Y + y - 1);
	}
	
	while(x < y)    //����������ÿ��x����
	{
		x++;
		if(d < 0)    //��һ�����ڵ�ǰ�㶫��
		{
			d += 2 * x + 1;
		}
		else    //��һ�����ڵ�ǰ�㶫�Ϸ�
		{
			y--;
			d += 2 * (x - y) + 1;
		}
		
		//��ÿ���˷�֮һԲ���ĵ�
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X + y, Y + x);
		OLED_DrawPoint(X + y, Y - x);
		OLED_DrawPoint(X - y, Y + x);
		OLED_DrawPoint(X - y, Y - x);
		
		if(IsFilled)    //ָ��Բ���
		{
			//���м䲿��
			OLED_DrawLine(X + x, Y - y + 1, X + x, Y + y - 1);
			OLED_DrawLine(X - x, Y - y + 1, X - x, Y + y - 1);
			
			//�����߲���
			OLED_DrawLine(X + y, Y - x + 1, X + y, Y + x - 1);
			OLED_DrawLine(X - y, Y - x + 1, X - y, Y + x - 1);
		}
	}
}

/**
  * @brief  OLED����Բ������ʹ��Bresenham�㷨����Բ�����Ա��ⲿ�ֺ�ʱ�ĸ������㣬Ч�ʸ���
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ����Բ��Բ�ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ����Բ��Բ�������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  A ָ����Բ�ĺ�����᳤�ȣ���Χ��0 ~ 255
  * @param  B ָ����Բ��������᳤�ȣ���Χ��0 ~ 255
  * @param  IsFilled ָ��Բ�Ƿ����
  *         ��Χ��OLED_UNFILLED		�����
  *               OLED_FILLED		���
  * @retval ��
  */
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
	int16_t x, y;
	int16_t a = A, b = B;
	float d1, d2;
	
	//Bresenham����Բ�㷨
	d1 = b * b + a * a * (-b + 0.5);
	x = 0;
	y = b;
	
	//����Բ����ʼ��
	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X + x, Y - y);
	
	if(IsFilled)    //ָ����Բ���
	{
		OLED_DrawLine(X + x, Y - y + 1, X + x, Y + y - 1);
	}
	
	//����Բ�м䲿��
	while(b * b * (x + 1) < a * a * (y - 0.5))
	{
		if(d1 < 0)    //��һ�����ڵ�ǰ�㶫��
		{
			d1 += b * b * (2 * x + 3);
		}
		else    //��һ�����ڵ�ǰ�㶫�Ϸ�
		{
			d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
			y --;
		}
		x++;
		
		//����Բ�м䲿��Բ��
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		
		if(IsFilled)    //ָ����Բ���
		{
			//���м䲿��
			OLED_DrawLine(X + x, Y - y + 1, X + x, Y + y - 1);
			OLED_DrawLine(X - x, Y - y + 1, X - x, Y + y - 1);
		}
	}
	
	//����Բ���ಿ��
	d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;
	while(y > 0)
	{
		if(d2 < 0)    //��һ�����ڵ�ǰ�㶫��
		{
			d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
			x++;
		}
		else    //��һ�����ڵ�ǰ�㶫�Ϸ�
		{
			d2 += a * a * (-2 * y + 3);
		}
		y--;
		
		//����Բ���ಿ��Բ��
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		
		if(IsFilled)    //ָ����Բ���
		{
			//�����ಿ��
			OLED_DrawLine(X + x, Y - y + 1, X + x, Y + y - 1);
			OLED_DrawLine(X - x, Y - y + 1, X - x, Y + y - 1);
		}
	}
}

/**
  * @brief  OLED��Բ��������ʹ��Bresenham��Բ�㷨
  *         ���ô˺�����Ҫ�������س�������Ļ�ϣ�������ø��º���
  * @param  X ָ��Բ����Բ�ĺ����꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 127
  * @param  Y ָ��Բ����Բ�������꣬��Χ��-32768 ~ 32767����Ļ����0 ~ 63
  * @param  Radius ָ��Բ���İ뾶����Χ��0 ~ 255
  * @param  StartAngle ָ��Բ������ʼ�Ƕȣ���Χ��-180 ~ 180
  *         ˮƽ����Ϊ0�ȣ�ˮƽ����Ϊ180�Ȼ�-180�ȣ��·�Ϊ�������Ϸ�Ϊ������˳ʱ����ת
  * @param  EndAngle ָ��Բ������ֹ�Ƕȣ���Χ��-180 ~ 180
  *         ˮƽ����Ϊ0�ȣ�ˮƽ����Ϊ180�Ȼ�-180�ȣ��·�Ϊ�������Ϸ�Ϊ������˳ʱ����ת
  * @param  IsFilled ָ��Բ�Ƿ����
  *         ��Χ��OLED_UNFILLED		�����
  *               OLED_FILLED		���
  * @retval ��
  */
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
	int16_t x, y, d, j;
	
	//�˺�������Bresenham��Բ�㷨
	d = 1 - Radius;
	x = 0;
	y = Radius;
	
	//�ڻ�Բ��ÿ����ʱ���ж�ָ�����Ƿ���ָ���Ƕ��ڣ��ڣ��򻭵㣬���ڣ���������
	if(OLED_IsInAngle(x, y, StartAngle, EndAngle))   {OLED_DrawPoint(X + x, Y + y);}
	if(OLED_IsInAngle(x, -y, StartAngle, EndAngle))  {OLED_DrawPoint(X + x, Y - y);}
	if(OLED_IsInAngle(y, x, StartAngle, EndAngle))   {OLED_DrawPoint(X + y, Y + x);}
	if(OLED_IsInAngle(-y, x, StartAngle, EndAngle))  {OLED_DrawPoint(X - y, Y + x);}
	
	if(IsFilled)    //ָ��Բ�����
	{
		for(j = -y + 1; j < y; j++)
		{
			//�����Բ��ÿ����ʱ���ж�ָ�����Ƿ���ָ���Ƕ��ڣ��ڣ��򻭵㣬���ڣ���������
			if(OLED_IsInAngle(x, j, StartAngle, EndAngle))   {OLED_DrawPoint(X + x, Y + j);}
		}
	}
	
	while(x < y)    //����������ÿ��x����
	{
		x++;
		if(d < 0)    //��һ�����ڵ�ǰ�㶫��
		{
			d += 2 * x + 1;
		}
		else    //��һ�����ڵ�ǰ�㶫�Ϸ�
		{
			y--;
			d += 2 * (x - y) + 1;
		}
		
		//�ڻ�Բ��ÿ����ʱ���ж�ָ�����Ƿ���ָ���Ƕ��ڣ��ڣ��򻭵㣬���ڣ���������
		if(OLED_IsInAngle(x, y, StartAngle, EndAngle))    {OLED_DrawPoint(X + x, Y + y);}
		if(OLED_IsInAngle(x, -y, StartAngle, EndAngle))   {OLED_DrawPoint(X + x, Y - y);}
		if(OLED_IsInAngle(-x, y, StartAngle, EndAngle))   {OLED_DrawPoint(X - x, Y + y);}
		if(OLED_IsInAngle(-x, -y, StartAngle, EndAngle))  {OLED_DrawPoint(X - x, Y - y);}
		if(OLED_IsInAngle(y, x, StartAngle, EndAngle))    {OLED_DrawPoint(X + y, Y + x);}
		if(OLED_IsInAngle(y, -x, StartAngle, EndAngle))   {OLED_DrawPoint(X + y, Y - x);}
		if(OLED_IsInAngle(-y, x, StartAngle, EndAngle))   {OLED_DrawPoint(X - y, Y + x);}
		if(OLED_IsInAngle(-y, -x, StartAngle, EndAngle))  {OLED_DrawPoint(X - y, Y - x);}
		
		if(IsFilled)    //ָ��Բ�����
		{
			//���м䲿��
			for(j = -y + 1; j < y; j++)
			{
				//�����Բ��ÿ����ʱ���ж�ָ�����Ƿ���ָ���Ƕ��ڣ��ڣ��򻭵㣬���ڣ���������
				if(OLED_IsInAngle(x, j, StartAngle, EndAngle))   {OLED_DrawPoint(X + x, Y + j);}
				if(OLED_IsInAngle(-x, j, StartAngle, EndAngle))  {OLED_DrawPoint(X - x, Y + j);}
			}
			
			//�����߲���
			for(j = -x + 1; j < x; j++)
			{
				//�����Բ��ÿ����ʱ���ж�ָ�����Ƿ���ָ���Ƕ��ڣ��ڣ��򻭵㣬���ڣ���������
				if(OLED_IsInAngle(y, j, StartAngle, EndAngle))   {OLED_DrawPoint(X + y, Y + j);}
				if(OLED_IsInAngle(-y, j, StartAngle, EndAngle))  {OLED_DrawPoint(X - y, Y + j);}
			}
		}
	}
}

/*********************���ܺ���*/
