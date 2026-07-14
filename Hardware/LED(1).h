#ifndef __LED_H
#define __LED_H

/*LED编码定义*/
#define LED1         0x0001
#define LED2         0x0002
#define LED3         0x0004
#define LED4         0x0008
#define LED5         0x0010
#define LED6         0x0020
#define LED7         0x0040
#define LED8         0x0080
#define LED9         0x0100
#define LED10        0x0200
#define LED11        0x0400
#define LED12        0x0800
#define LED13        0x1000
#define LED14        0x2000
#define LED15        0x4000
#define LED16        0x8000

/*模式定义*/
#define LED_OFFMode              0
#define LED_ONMode               1
#define LED_SlowFlashMode        2
#define LED_FastFlashMode        3
#define LED_DotFlashMode         4

void LED_Init(void);
void LED_ON(uint16_t Code);
void LED_OFF(uint16_t Code);
void LED_Turn(uint16_t Code);
void LED_SetMode(uint16_t Code, uint8_t Mode);
void LED_Tick(void);

#endif
