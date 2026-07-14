#ifndef __MPU6050_H
#define __MPU6050_H

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_ReadReg(uint8_t RegAddress);
uint8_t MPU6050_Write_Len(uint8_t Addr, uint8_t Reg, uint8_t Len, uint8_t *Buf);
uint8_t MPU6050_Read_Len(uint8_t Addr,uint8_t Reg,uint8_t Len,uint8_t *Buf);
void MPU6050_Init(void);
uint8_t MPU6050_GetID(void);
/** @brief 探测 I2C；返回 0=0x68 在线，1=0x69 在线，0xFF=无设备；whoami 为 WHO_AM_I */
uint8_t MPU6050_Probe(uint8_t *whoami);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);
/**
  * @brief  初始化 MPU6050 并加载 DMP
  * @retval 0=成功，非0=失败（不再内部死循环，由 main 决定如何处理）
  */
uint8_t MPU6050_DMPInit(void);
uint8_t MPU6050_ReadDMP(float *Pitch, float *Roll, float *Yaw);
void MPU6050_EXTI_Init(void);

#endif
