/*****************************************************************
  Copyright (c) [2026], [开发者] All rights reserved
  File Name   : timer.h
  Description : 定时器 PWM 驱动（LED 呼吸灯：PB5 / PB6）
  History     :
         2026-09-19   create
*****************************************************************/

/* ================================= 头文件保护宏 ================================= */
#ifndef __TIMER_H
#define __TIMER_H

/* ================================= 依赖头文件包含区 ================================= */
#include "gd32f4xx.h"   /* 芯片厂商的库，里面有 GPIOB、TIMER2/TIMER3 等定义 */

/* ================================= 导出宏定义区 ================================= */
/* ----------------------------------------------------------------------------
 * LED1 引脚：PB5 = TIMER2_CH1，复用 AF2
 *   依据 GD32F407 引脚定义表：PB5 的复用功能是 TIMER2_CH1。
 *   GD32 的通道从 0 开始编号，手册写的 CH1 对应库里的 TIMER_CH_1。
 * ----------------------------------------------------------------------------
 */
#define TIMER_LED1_PORT         GPIOB          /* 端口 B */
#define TIMER_LED1_PIN          GPIO_PIN_5     /* PB5 */
#define TIMER_LED1_CLK          RCU_GPIOB      /* GPIOB 时钟（用前要开） */
#define TIMER_LED1_AF           GPIO_AF_2      /* 复用功能 AF2 = TIMER2 */
#define TIMER_LED1_TIMER        TIMER2         /* 用定时器 2 */
#define TIMER_LED1_TIMER_CLK    RCU_TIMER2     /* TIMER2 时钟 */
#define TIMER_LED1_CH           TIMER_CH_1     /* 通道 1（手册里的 CH1） */

/* ----------------------------------------------------------------------------
 * LED2 引脚：PB6 = TIMER3_CH0，复用 AF2
 *   依据 GD32F407 引脚定义表：PB6 的复用功能是 TIMER3_CH0。
 * ----------------------------------------------------------------------------
 */
#define TIMER_LED2_PORT         GPIOB          /* 端口 B */
#define TIMER_LED2_PIN          GPIO_PIN_6     /* PB6 */
#define TIMER_LED2_CLK          RCU_GPIOB      /* GPIOB 时钟 */
#define TIMER_LED2_AF           GPIO_AF_2      /* 复用功能 AF2 = TIMER3 */
#define TIMER_LED2_TIMER        TIMER3         /* 用定时器 3 */
#define TIMER_LED2_TIMER_CLK    RCU_TIMER3     /* TIMER3 时钟 */
#define TIMER_LED2_CH           TIMER_CH_0     /* 通道 0（手册里的 CH0） */

/* ----------------------------------------------------------------------------
 * PWM 波形参数（LED 呼吸灯用）：
 *   定时器时钟 = 84MHz（APB1 42MHz ×2，和舵机的 TIMER1 一样）
 *   预分频 83  → 84MHz/84 = 1MHz
 *   周期   999 → 数 1000 下 = 1kHz（LED 用 1kHz 足够，人眼完全看不出闪烁）
 * 亮度 0~100% 就对应比较值 0~999。
 * ----------------------------------------------------------------------------
 */
#define TIMER_PWM_PRESCALER     83U
#define TIMER_PWM_PERIOD        999U

/* LED 编号（传给 timer_pwm_set 选择控制哪个灯） */
#define TIMER_LED1              0U
#define TIMER_LED2              1U

/* ================================= 导出类型定义区 ================================= */

/* ================================= 导出常量定义区 ================================= */

/* ================================= 导出全局变量声明区 ================================= */

/* ================================= 导出函数声明区 ================================= */
/* 初始化两个 LED 的 PWM（PB5->TIMER2，PB6->TIMER3）。
 * 程序启动时在 main() 里调用一次即可。 */
void timer_pwm_init(void);

/* 设置某个 LED 的亮度。
 *   led     : 选哪个灯（TIMER_LED1 或 TIMER_LED2）
 *   percent : 亮度 0~100，超出自动钳到 100 */
void timer_pwm_set(uint8_t led, uint8_t percent);

#endif /* __TIMER_H */

/*****************************************************************  END OF FILE  ***/
