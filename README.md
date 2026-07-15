# MPU6050 + DMP 姿态角 OLED 显示（防卡死版）

基于 **STM32F103C8** 与 **MPU6050**，使用 InvenSense **eMPL / DMP** 解算 Pitch / Roll / Yaw，在 OLED 上实时显示。工程特别处理了 **中断内 I2C + SysTick 延时导致整机卡死** 的经典问题。

---

## 问题与改法

| 错误做法 | 后果 |
|----------|------|
| 在 TIM 中断里调用 `MPU6050_ReadDMP` → 软件 I2C → `Delay_us` 改写 SysTick | 主循环延时标志错乱 → **卡死 / OLED 假死** |

**正确做法：**

- 中断里 **只置标志位**，禁止 I2C / 延时  
- 主循环中读 DMP + 刷屏  

---

## 功能概述

- 软件 I2C 读 MPU6050 / DMP  
- 姿态角 OLED 显示（±xxx.x°，不用 `%f`）  
- 可选串口输出（`Serial`）  
- TIM 仅作周期节拍，不在 ISR 内访问总线  

---

## 硬件

| 模块 | 接口 | 说明 |
|------|------|------|
| MPU6050 | 软件 I2C（见 `MyI2C`） | SDA/SCL 接对应 GPIO，上拉 |
| OLED | 见 `OLED.c` | |
| 主控 | STM32F103C8 | |

---

## 目录结构

```
├── User/main.c
├── Hardware/
│   ├── MPU6050.* / MPU6050_Reg.h
│   ├── MyI2C.*          # 软件 I2C
│   ├── eMPL/            # DMP 官方库相关
│   ├── OLED.* / Serial.*
│   └── Key / LED
├── System/ Library/ Start/
└── Project.uvprojx
```

> 仓库中若存在 `*(1).*` 备份文件，以不带 `(1)` 的工程与源文件为准。

---

## 使用

1. Keil 打开 `Project.uvprojx`，确认包含路径含 eMPL  
2. 接好 MPU6050 与 OLED，编译下载  
3. 上电显示 Attitude，三轴角度随姿态变化  

初始化失败时检查 I2C 引脚、地址、供电与 AD0 电平。

---

## 相关仓库

- [jy901s](https://github.com/kaka12331/jy901s) — 串口姿态模块  
- [lingke-6axis-gyro](https://github.com/kaka12331/lingke-6axis-gyro) — 串口 6 轴解析  

用于学习与备赛交流。
