#ifndef __KEY_H
#define __KEY_H

/*按键编码定义*/
#define KEY1         1
#define KEY2         2
#define KEY3         3
#define KEY4         4
#define KEY5         5
#define KEY6         6
#define KEY7         7
#define KEY8         8
#define KEY9         9
#define KEY10        10
#define KEY11        11
#define KEY12        12
#define KEY13        13
#define KEY14        14
#define KEY15        15
#define KEY16        16

/*按键事件编码定义*/
#define KEY_CLICK		0x01
#define KEY_DOUBLE		0x02
#define KEY_LONG		0x04

void Key_Init(void);
uint8_t Key_Check(uint8_t n, uint8_t Event);
void Key_Clear(void);
void Key_Tick(void);

#endif
