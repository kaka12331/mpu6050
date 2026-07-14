/**
  ******************************************************************************
  * @file    main.c
  * @brief   MPU6050 三轴角度 OLED 显示（防卡死版）
  * @note
  *   【卡死原因】
  *   TIM2 中断里调用 MPU6050_ReadDMP → 软件 I2C → Delay_us() 会改写 SysTick。
  *   主循环里 OLED/Delay_ms 也用 SysTick。中断打断延时时 SysTick 被改乱，
  *   主循环 Delay_us 永远等不到标志位 → 整机卡死，OLED 假死。
  *
  *   【改法】
  *   中断里禁止做 I2C / Delay；只在主循环读 DMP + 刷屏。
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "MPU6050.h"
#include "Serial.h"
#include "Timer.h"

float Pitch, Roll, Yaw;
static uint8_t g_mpu_ok = 0;

/* 仅由 TIM2 置 1，主循环清 0；中断内禁止任何 I2C/延时 */
volatile uint8_t g_time_flag = 0;

/**
  * @brief  显示 ±xxx.x 角度（1 位小数，不用 %f）
  */
static void OLED_ShowAngle1(int16_t X, int16_t Y, float angle)
{
	int32_t v;
	int32_t ip;
	int32_t fp;

	if (angle >= 0.0f)
		v = (int32_t)(angle * 10.0f + 0.5f);
	else
		v = (int32_t)(angle * 10.0f - 0.5f);

	if (v >= 0)
	{
		OLED_ShowChar(X, Y, '+', OLED_8X16);
	}
	else
	{
		OLED_ShowChar(X, Y, '-', OLED_8X16);
		v = -v;
	}

	ip = v / 10;
	fp = v % 10;

	OLED_ShowNum(X + 8, Y, (uint32_t)ip, 3, OLED_8X16);
	OLED_ShowChar(X + 8 + 24, Y, '.', OLED_8X16);
	OLED_ShowNum(X + 8 + 32, Y, (uint32_t)fp, 1, OLED_8X16);
}

static void UI_DrawStatic(void)
{
	OLED_Clear();
	OLED_ShowString(0, 0,  "Attitude", OLED_8X16);
	OLED_ShowString(0, 16, "Pitch:", OLED_8X16);
	OLED_ShowString(0, 32, "Roll :", OLED_8X16);
	OLED_ShowString(0, 48, "Yaw  :", OLED_8X16);
	OLED_ShowString(104, 20, "deg", OLED_6X8);
	OLED_ShowString(104, 36, "deg", OLED_6X8);
	OLED_ShowString(104, 52, "deg", OLED_6X8);
	OLED_Update();
}

int main(void)
{
	uint8_t who = 0;
	uint8_t probe;
	uint8_t dmp_err;
	uint8_t oled_div = 0;   /* 分频：不必每次读传感器都刷屏 */

	OLED_Init();
	OLED_ShowString(0, 0, "OLED OK", OLED_8X16);
	OLED_Update();
	Delay_ms(150);

	Serial_Init();

	OLED_Clear();
	OLED_ShowString(0, 0, "Probe MPU", OLED_8X16);
	OLED_Update();

	probe = MPU6050_Probe(&who);
	OLED_ShowString(0, 16, "ID:", OLED_8X16);
	OLED_ShowHexNum(24, 16, who, 2, OLED_8X16);

	if (probe == 0xFF)
	{
		OLED_ShowString(0, 32, "NO ACK!", OLED_8X16);
		OLED_ShowString(0, 48, "PB10/11?", OLED_8X16);
		OLED_Update();
		g_mpu_ok = 0;
	}
	else if (probe == 1)
	{
		OLED_ShowString(0, 32, "Addr 0x69", OLED_8X16);
		OLED_ShowString(0, 48, "AD0->GND!", OLED_8X16);
		OLED_Update();
		g_mpu_ok = 0;
	}
	else
	{
		OLED_ShowString(0, 32, "I2C OK", OLED_8X16);
		OLED_Update();
		Delay_ms(200);

		OLED_Clear();
		OLED_ShowString(0, 0, "DMP init..", OLED_8X16);
		OLED_Update();

		dmp_err = MPU6050_DMPInit();
		if (dmp_err == 0)
		{
			g_mpu_ok = 1;
			Timer_Init();     /* 仅作节拍，中断不读传感器 */
			UI_DrawStatic();
		}
		else
		{
			g_mpu_ok = 0;
			OLED_ShowString(0, 16, "DMP FAIL", OLED_8X16);
			OLED_ShowString(0, 32, "step:", OLED_8X16);
			OLED_ShowNum(48, 32, dmp_err, 2, OLED_8X16);
			OLED_ShowString(0, 48, "ID:", OLED_8X16);
			OLED_ShowHexNum(24, 48, who, 2, OLED_8X16);
			OLED_Update();
		}
	}

	while (1)
	{
		if (!g_mpu_ok)
		{
			Delay_ms(200);
			continue;
		}

		/* 等 TIM2 节拍（约 10ms），中断只置位，不碰 I2C */
		if (g_time_flag == 0)
		{
			continue;
		}
		g_time_flag = 0;

		/* 主循环里读 DMP（可用 Delay_us，不会被同套 SysTick 打断搞死） */
		MPU6050_ReadDMP(&Pitch, &Roll, &Yaw);

		/* 约每 5 个节拍刷一次屏 ≈ 50ms，减轻 OLED 软件 I2C 负担 */
		oled_div++;
		if (oled_div < 5)
		{
			continue;
		}
		oled_div = 0;

		OLED_ShowAngle1(48, 16, Pitch);
		OLED_ShowAngle1(48, 32, Roll);
		OLED_ShowAngle1(48, 48, Yaw);
		OLED_UpdateArea(48, 16, 56, 48);
	}
}

/**
  * @brief  TIM2 中断：只置标志！禁止 I2C / Delay / 浮点重计算
  */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		g_time_flag = 1;
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
