#include "stm32f10x.h"                  // Device header
#include <math.h>
#include "MyI2C.h"
#include "Delay.h"
#include "MPU6050_Reg.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "Serial.h"

/*��������*/
#define MPU6050_SCL        GPIO_Pin_10   //PB10
#define MPU6050_SDA        GPIO_Pin_11   //PB11

#define MPU6050_ADDRESS        0xD0             //д��ַ
#define DEFAULT_MPU_HZ         (100)            //��������ٶ�
#define q30                    1073741824.0f    //q30��ʽ��longתfloatʱ�ĳ���
#define MPU_INT_EN_REG			0X38	//�ж�ʹ�ܼĴ���
#define MPU_INTBP_CFG_REG		0X37	//�ж�/��·���üĴ���

//�����Ƿ�������
static signed char gyro_orientation[9] = { 1,  0,  0,
                                           0,  1,  0,
                                           0,  0,  1};
float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
unsigned long sensor_timestamp;
short gyro[3], accel[3], sensors;
unsigned char more;
long quat[4];

/**
  * @brief  MPU6050ָ����ַдһ���ֽں���
  * @param  RegAddress �Ĵ�����ַ
  * @param  Data Ҫд����ֽ�����
  * @retval ��
  */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    MyI2C_Start();
    MyI2C_SendByte(MPU6050_ADDRESS | 0x00);
    MyI2C_ReceiveAck();
    MyI2C_SendByte(RegAddress);
    MyI2C_ReceiveAck();
    MyI2C_SendByte(Data);
    MyI2C_ReceiveAck();
    MyI2C_Stop();   
}

/**
  * @brief  MPU6050ָ����ַ��һ���ֽں���
  * @param  RegAddress �Ĵ�����ַ
  * @retval ���ض������ֽ�����
  */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
    uint8_t Data;

    MyI2C_Start();
    MyI2C_SendByte(MPU6050_ADDRESS | 0x00);
    MyI2C_ReceiveAck();
    MyI2C_SendByte(RegAddress);
    MyI2C_ReceiveAck();
    MyI2C_Start();
    MyI2C_SendByte(MPU6050_ADDRESS | 0x01);
    MyI2C_ReceiveAck();
    Data = MyI2C_ReceiveByte();
    MyI2C_SendAck(1);
    MyI2C_Stop(); 
    return Data;
}

/**
  * @brief  MPU6050ָ����ַ����д�ֽں���
  * @param  Addr ������ַ
  * @param  Reg  �Ĵ�����ַ
  * @param  Len  Ҫд������ݳ���
  * @param  Buf  д�����ݵĴ洢��
  * @retval ����0��ʾ������������ʾʧ��
  */
uint8_t MPU6050_Write_Len(uint8_t Addr, uint8_t Reg, uint8_t Len, uint8_t *Buf)
{
    MyI2C_Start();
	MyI2C_SendByte((Addr << 1) | 0x00);    //����������ַ+д����
	if(MyI2C_ReceiveAck())    //�ȴ�Ӧ��
	{
		MyI2C_Stop();
		return 1;
	}
    MyI2C_SendByte(Reg);	  //д�Ĵ�����ַ
    if(MyI2C_ReceiveAck())    //�ȴ�Ӧ��
	{
		MyI2C_Stop();
		return 1;
	}
	while(Len--)
	{
		MyI2C_SendByte(*Buf++);    //��������
		if(MyI2C_ReceiveAck())     //�ȴ�ACK
		{
			MyI2C_Stop();
			return 1;
		}
	}
    MyI2C_Stop();
	return 0;
} 

/**
  * @brief  MPU6050ָ����ַ�������ֽں���
  * @param  Addr ������ַ
  * @param  Reg  �Ĵ�����ַ
  * @param  Len  Ҫ��ȡ�����ݳ���
  * @param  Buf  ��ȡ���ݵĴ洢��
  * @retval ����0��ʾ������������ʾʧ��
  */
uint8_t MPU6050_Read_Len(uint8_t Addr, uint8_t Reg, uint8_t Len, uint8_t *Buf)
{
	MyI2C_Start();
	MyI2C_SendByte((Addr << 1) | 0x00);    //����������ַ+д����
	if(MyI2C_ReceiveAck())
	{
		MyI2C_Stop();
		return 1;
	}
	MyI2C_SendByte(Reg);      //д�Ĵ�����ַ
	if(MyI2C_ReceiveAck())    //�ȴ�Ӧ��
	{
		MyI2C_Stop();
		return 1;
	}
	MyI2C_Start();
	MyI2C_SendByte((Addr << 1) | 0x01);    //����������ַ+������
	if(MyI2C_ReceiveAck())    //�ȴ�Ӧ��
	{
		MyI2C_Stop();
		return 1;
	}
	while(Len--)
	{
		*Buf++ = MyI2C_ReceiveByte();    //������
		if(Len)  {MyI2C_SendAck(0);}     //����ACK
		else     {MyI2C_SendAck(1);}     //����nACK	
	}
	MyI2C_Stop();
	return 0;
}

/**
  * @brief  MPU6050��ʼ������
  * @param  ��
  * @retval ��
  */
void MPU6050_Init(void)
{
    MyI2C_Init();
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);      //����λ�����˯�ߣ���ѭ�����¶ȴ�������ʧ�ܣ�ѡ��X��������ʱ��
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);      //��ѭ����������
    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);      //10��Ƶ��������1kHz / (1 + 9) = 100Hz
    MPU6050_WriteReg(MPU6050_CONFIG, 0x06);          //���ⲿͬ���������˲�ģʽ6
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);     //�����ǲ��Բ⣬���̡�2000��/s
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);    //���ٶȼƲ��Բ⣬���̡�16g����ʹ�ø�ͨ�˲���
		MPU6050_WriteReg(MPU_INT_EN_REG, 0X01);
    MPU6050_WriteReg(MPU_INTBP_CFG_REG, 0X80);	//INT����0X80�͵�ƽ����
}

void MPU6050_EXTI_Init(void)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE); //�ⲿ�жϣ���Ҫʹ��AFIOʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); //ʹ��PB�˿�ʱ��
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //��������
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource5);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line5;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //�½��ش���
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02; //��ռ���ȼ�2�� 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01; //�����ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
}


/**
  * @brief  MPU6050��ȡID�ź���
  * @param  ��
  * @retval ����ID��
  */
uint8_t MPU6050_GetID(void)
{
    return MPU6050_ReadReg(MPU6050_WHO_AM_I);
}

/**
  * @brief  探测 MPU 是否在总线上应答，并读 WHO_AM_I
  * @param  whoami  输出 WHO_AM_I 寄存器值（读失败时为 0x00）
  * @retval 0=地址 0x68 应答；1=地址 0x69 应答；0xFF=均无应答
  * @note   eMPL 默认用 0x68（AD0=GND）。AD0 接 VCC 时为 0x69。
  */
uint8_t MPU6050_Probe(uint8_t *whoami)
{
    uint8_t dummy = 0;
    uint8_t id = 0;

    MyI2C_Init();
    Delay_ms(50);   /* 上电稳定 */

    /* 先试 7 位地址 0x68（Write_Len 内部会左移成 0xD0） */
    if (MPU6050_Read_Len(0x68, MPU6050_WHO_AM_I, 1, &id) == 0)
    {
        if (whoami) *whoami = id;
        return 0;
    }

    /* 再试 0x69（AD0 拉高） */
    if (MPU6050_Read_Len(0x69, MPU6050_WHO_AM_I, 1, &id) == 0)
    {
        if (whoami) *whoami = id;
        return 1;
    }

    if (whoami) *whoami = 0x00;
    (void)dummy;
    return 0xFF;
}

/**
  * @brief  MPU6050��ȡ���ݺ���
  * @param  AccX ���ٶȼ�X������
  * @param  AccY ���ٶȼ�Y������
  * @param  AccZ ���ٶȼ�Z������
  * @param  GyroX ������X������
  * @param  GyroY ������Y������
  * @param  GyroZ ������Z������
  * @retval ��
  */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
    uint8_t DataH, DataL;
    
    DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);
    *AccX = (DataH << 8) | DataL;
    DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);
    *AccY = (DataH << 8) | DataL;
    DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);
    *AccZ = (DataH << 8) | DataL;
    DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);
    *GyroX = (DataH << 8) | DataL;
    DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);
    *GyroY = (DataH << 8) | DataL;
    DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);
    *GyroZ = (DataH << 8) | DataL;  
}

/**
  * @brief  DMP��ʼ������
  * @param  ��
  * @retval ��
  */
uint8_t MPU6050_DMPInit(void)
{
	uint8_t res = 0;
	uint8_t step = 0;
    
	MyI2C_Init();
	Delay_ms(100);   /* 模块上电稳定后再访问 */

    res = mpu_init();
    if (res)
    {
        Serial_Printf("mpu_init fail code=%d\r\n", (int)res);
        return 1;   /* 步骤1失败：I2C 或芯片兼容 */
    }
    Serial_Printf("mpu initialization complete ......\r\n");

    step = 2;
    res = mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    if (res) { Serial_Printf("mpu_set_sensor error\r\n"); return step; }
    Serial_Printf("mpu_set_sensor complete ......\r\n");

    step = 3;
    res = mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    if (res) { Serial_Printf("mpu_configure_fifo error\r\n"); return step; }
    Serial_Printf("mpu_configure_fifo complete ......\r\n");

    step = 4;
    res = mpu_set_sample_rate(DEFAULT_MPU_HZ);
    if (res) { Serial_Printf("mpu_set_sample_rate error\r\n"); return step; }
    Serial_Printf("mpu_set_sample_rate complete ......\r\n");

    step = 5;
    res = dmp_load_motion_driver_firmware();
    if (res) { Serial_Printf("dmp_load_firmware error\r\n"); return step; }
    Serial_Printf("dmp_load_motion_driver_firmware complete ......\r\n");

    step = 6;
    res = dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));
    if (res) { Serial_Printf("dmp_set_orientation error\r\n"); return step; }
    Serial_Printf("dmp_set_orientation complete ......\r\n");

    step = 7;
    res = dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
          DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
          DMP_FEATURE_GYRO_CAL);
    if (res) { Serial_Printf("dmp_enable_feature error\r\n"); return step; }
    Serial_Printf("dmp_enable_feature complete ......\r\n");

    step = 8;
    res = dmp_set_fifo_rate(DEFAULT_MPU_HZ);
    if (res) { Serial_Printf("dmp_set_fifo_rate error\r\n"); return step; }
    Serial_Printf("dmp_set_fifo_rate complete ......\r\n");

    /* 自检：失败也不 return（函数内已放宽） */
    (void)run_self_test();
    Serial_Printf("mpu_run_self_test done ......\r\n");

    step = 9;
    res = mpu_set_dmp_state(1);
    if (res) { Serial_Printf("mpu_set_dmp_state error\r\n"); return step; }
    Serial_Printf("mpu_set_dmp_state complete ......\r\n");

    return 0;
}

/**
  * @brief  ��ȡDMP���������ݺ���
  * @param  Pitch �����ǣ�����:0.1�㣬��Χ:-90.0�� ~ +90.0��
  * @param  Roll  ����ǣ�����:0.1�㣬��Χ:-180.0��~ +180.0��
  * @param  yaw   ƫ���ǣ�����:0.1�㣬��Χ:-180.0��~ +180.0��
  * @retval ����0��ʾ������������ʾʧ��
  */
uint8_t MPU6050_ReadDMP(float *Pitch, float *Roll, float *Yaw)
{	
	if(dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors, &more))  return 1;	
	if(sensors & INV_WXYZ_QUAT)
	{    
        q0 = quat[0] / q30;    //q30��ʽת��Ϊ������
        q1 = quat[1] / q30;
        q2 = quat[2] / q30;
        q3 = quat[3] / q30;
        //����õ������ǡ�����Ǻ�ƫ����
        *Pitch = asin(-2 * q1 * q3 + 2 * q0 * q2) * 57.3; 	
        *Roll  = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2 * q2 + 1) * 57.3;
        *Yaw   = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2 * q2 - 2 * q3 * q3 + 1) * 57.3;
    }else  return 2;
    return 0;
}

//void EXTI9_5_IRQHandler(void)
//{
//	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5) == 0) //�͵�ƽ����
//	{
//		EXTI->PR=1<<5; //����жϱ�־λ
//		
//	}
//}

