#include "stm32f10x.h"                  // Device header
#include <math.h>
#include "MyI2C.h"
#include "MPU6050_Reg.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "Serial.h"

/*引脚配置*/
#define MPU6050_SCL        GPIO_Pin_10   //PB10
#define MPU6050_SDA        GPIO_Pin_11   //PB11

#define MPU6050_ADDRESS        0xD0             //写地址
#define DEFAULT_MPU_HZ         (100)            //定义输出速度
#define q30                    1073741824.0f    //q30格式，long转float时的除数
#define MPU_INT_EN_REG			0X38	//中断使能寄存器
#define MPU_INTBP_CFG_REG		0X37	//中断/旁路设置寄存器

//陀螺仪方向设置
static signed char gyro_orientation[9] = { 1,  0,  0,
                                           0,  1,  0,
                                           0,  0,  1};
float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
unsigned long sensor_timestamp;
short gyro[3], accel[3], sensors;
unsigned char more;
long quat[4];

/**
  * @brief  MPU6050指定地址写一个字节函数
  * @param  RegAddress 寄存器地址
  * @param  Data 要写入的字节数据
  * @retval 无
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
  * @brief  MPU6050指定地址读一个字节函数
  * @param  RegAddress 寄存器地址
  * @retval 返回读出的字节数据
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
  * @brief  MPU6050指定地址连续写字节函数
  * @param  Addr 器件地址
  * @param  Reg  寄存器地址
  * @param  Len  要写入的数据长度
  * @param  Buf  写入数据的存储区
  * @retval 返回0表示正常，其他表示失败
  */
uint8_t MPU6050_Write_Len(uint8_t Addr, uint8_t Reg, uint8_t Len, uint8_t *Buf)
{
    MyI2C_Start();
	MyI2C_SendByte((Addr << 1) | 0x00);    //发送器件地址+写命令
	if(MyI2C_ReceiveAck())    //等待应答
	{
		MyI2C_Stop();
		return 1;
	}
    MyI2C_SendByte(Reg);	  //写寄存器地址
    if(MyI2C_ReceiveAck())    //等待应答
	{
		MyI2C_Stop();
		return 1;
	}
	while(Len--)
	{
		MyI2C_SendByte(*Buf++);    //发送数据
		if(MyI2C_ReceiveAck())     //等待ACK
		{
			MyI2C_Stop();
			return 1;
		}
	}
    MyI2C_Stop();
	return 0;
} 

/**
  * @brief  MPU6050指定地址连续读字节函数
  * @param  Addr 器件地址
  * @param  Reg  寄存器地址
  * @param  Len  要读取的数据长度
  * @param  Buf  读取数据的存储区
  * @retval 返回0表示正常，其他表示失败
  */
uint8_t MPU6050_Read_Len(uint8_t Addr, uint8_t Reg, uint8_t Len, uint8_t *Buf)
{
	MyI2C_Start();
	MyI2C_SendByte((Addr << 1) | 0x00);    //发送器件地址+写命令
	if(MyI2C_ReceiveAck())
	{
		MyI2C_Stop();
		return 1;
	}
	MyI2C_SendByte(Reg);      //写寄存器地址
	if(MyI2C_ReceiveAck())    //等待应答
	{
		MyI2C_Stop();
		return 1;
	}
	MyI2C_Start();
	MyI2C_SendByte((Addr << 1) | 0x01);    //发送器件地址+读命令
	if(MyI2C_ReceiveAck())    //等待应答
	{
		MyI2C_Stop();
		return 1;
	}
	while(Len--)
	{
		*Buf++ = MyI2C_ReceiveByte();    //读数据
		if(Len)  {MyI2C_SendAck(0);}     //发送ACK
		else     {MyI2C_SendAck(1);}     //发送nACK	
	}
	MyI2C_Stop();
	return 0;
}

/**
  * @brief  MPU6050初始化函数
  * @param  无
  * @retval 无
  */
void MPU6050_Init(void)
{
    MyI2C_Init();
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);      //不复位，解除睡眠，不循环，温度传感器不失能，选择X轴陀螺仪时钟
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);      //不循环，不待机
    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);      //10分频，采样率1kHz / (1 + 9) = 100Hz
    MPU6050_WriteReg(MPU6050_CONFIG, 0x06);          //无外部同步，数字滤波模式6
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);     //陀螺仪不自测，量程±2000°/s
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);    //加速度计不自测，量程±16g，不使用高通滤波器
		MPU6050_WriteReg(MPU_INT_EN_REG, 0X01);
    MPU6050_WriteReg(MPU_INTBP_CFG_REG, 0X80);	//INT引脚0X80低电平触发
}

void MPU6050_EXTI_Init(void)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE); //外部中断，需要使能AFIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); //使能PB端口时钟
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //上拉输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource5);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line5;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02; //抢占优先级2， 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01; //子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
}


/**
  * @brief  MPU6050获取ID号函数
  * @param  无
  * @retval 返回ID号
  */
uint8_t MPU6050_GetID(void)
{
    return MPU6050_ReadReg(MPU6050_WHO_AM_I);
}

/**
  * @brief  MPU6050获取数据函数
  * @param  AccX 加速度计X轴数据
  * @param  AccY 加速度计Y轴数据
  * @param  AccZ 加速度计Z轴数据
  * @param  GyroX 陀螺仪X轴数据
  * @param  GyroY 陀螺仪Y轴数据
  * @param  GyroZ 陀螺仪Z轴数据
  * @retval 无
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
  * @brief  DMP初始化函数
  * @param  无
  * @retval 无
  */
void MPU6050_DMPInit(void)
{
	uint8_t res = 0;
    
	MyI2C_Init();
    res = mpu_init();
    if(!res)
    {
        Serial_Printf("mpu initialization complete ......\r\n");
        
		//设置所需要的传感器
        res = mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
        if(!res)
            Serial_Printf("mpu_set_sensor complete ......\r\n");
        else
            Serial_Printf("mpu_set_sensor come across error ......\r\n");
		
        //设置FIFO
        res = mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
        if(!res)
            Serial_Printf("mpu_configure_fifo complete ......\r\n");
        else
            Serial_Printf("mpu_configure_fifo come across error ......\r\n");
        
		//设置采样率
        res = mpu_set_sample_rate(DEFAULT_MPU_HZ);
        if(!res)
            Serial_Printf("mpu_set_sample_rate complete ......\r\n");
        else
            Serial_Printf("mpu_set_sample_rate come across error ......\r\n");
        
		//加载DMP固件
        res = dmp_load_motion_driver_firmware();
        if(!res)
            Serial_Printf("dmp_load_motion_driver_firmware complete ......\r\n");
        else
            Serial_Printf("dmp_load_motion_driver_firmware come across error ......\r\n");
        
		//设置陀螺仪方向
        res = dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));
        if(!res)
            Serial_Printf("dmp_set_orientation complete ......\r\n");
        else
            Serial_Printf("dmp_set_orientation come across error ......\r\n");
        
		//设置DMP功能
        res = dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |	              
              DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
              DMP_FEATURE_GYRO_CAL);
        if(!res)
            Serial_Printf("dmp_enable_feature complete ......\r\n");
        else
            Serial_Printf("dmp_enable_feature come across error ......\r\n");
        
		//设置DMP输出速率(最大不超过200Hz)
        res = dmp_set_fifo_rate(DEFAULT_MPU_HZ);
        if(!res)
            Serial_Printf("dmp_set_fifo_rate complete ......\r\n");
        else
            Serial_Printf("dmp_set_fifo_rate come across error ......\r\n");
        
		//自检
        res = run_self_test();
        if(!res)
            Serial_Printf("mpu_run_self_test complete ......\r\n");
        else
            Serial_Printf("mpu_run_self_test come across error ......\r\n");
        
		//使能DMP
        res = mpu_set_dmp_state(1);
        if(!res)
            Serial_Printf("mpu_set_dmp_state complete ......\r\n");
        else
            Serial_Printf("mpu_set_dmp_state come across error ......\r\n");
    }
    else
    {
        Serial_Printf("mpu initialization come across error ......\r\n");
        while(1);
    }
}

/**
  * @brief  获取DMP处理后数据函数
  * @param  Pitch 俯仰角，精度:0.1°，范围:-90.0° ~ +90.0°
  * @param  Roll  横滚角，精度:0.1°，范围:-180.0°~ +180.0°
  * @param  yaw   偏航角，精度:0.1°，范围:-180.0°~ +180.0°
  * @retval 返回0表示正常，其他表示失败
  */
uint8_t MPU6050_ReadDMP(float *Pitch, float *Roll, float *Yaw)
{	
	if(dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors, &more))  return 1;	
	if(sensors & INV_WXYZ_QUAT)
	{    
        q0 = quat[0] / q30;    //q30格式转换为浮点数
        q1 = quat[1] / q30;
        q2 = quat[2] / q30;
        q3 = quat[3] / q30;
        //计算得到俯仰角、横滚角和偏航角
        *Pitch = asin(-2 * q1 * q3 + 2 * q0 * q2) * 57.3; 	
        *Roll  = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2 * q2 + 1) * 57.3;
        *Yaw   = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2 * q2 - 2 * q3 * q3 + 1) * 57.3;
    }else  return 2;
    return 0;
}

//void EXTI9_5_IRQHandler(void)
//{
//	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5) == 0) //低电平触发
//	{
//		EXTI->PR=1<<5; //清楚中断标志位
//		
//	}
//}

