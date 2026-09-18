/*****************************************************************
  Copyright (c) [2026], [开发者] All rights reserved
  File Name   : main.c
  Description : 程序入口，串口收角度指令控舵机，LED 以 250ms 交替闪烁

*****************************************************************/

/* ================================= 头文件包含区 ================================= */
#include "main.h"      /* 本文件对应的头文件（定义 LED 闪烁间隔等） */
#include "systick.h"   /* 延时函数 delay_1ms() */
#include "led.h"       /* LED 驱动：led_init / led_on / led_off */
#include "usart.h"     /* 串口驱动：usart1_init / usart1_get_line / 发送 */
#include "servo.h"     /* 舵机驱动：servo_init / servo_set_angle */

/* ================================= 私有宏定义区 ================================= */
#define RX_LINE_MAX     8U    /* 接收一行指令的最大长度（"180" 也就 3 个字符） */

/* ================================= 私有类型定义区 ================================= */

/* ================================= 私有全局变量区 ================================= */
static char s_rx_line[RX_LINE_MAX];   /* 存放从串口取出的一行字符串，如 "45" */

/* ================================= 私有函数声明区 ================================= */
static void handle_serial_command(void);   /* 处理串口指令的函数（见下方实现） */

/* ================================= 私有函数实现区 ================================= */

/*!
    \brief      处理串口舵机指令
    \param[in]  none
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    这是"串口 → 舵机"的核心逻辑，每次被调用时：
      ① 先问串口有没有收到完整一行，没有就直接返回
      ② 有就把字符串解析成数字（比如 "45" → 45）
      ③ 调 servo_set_angle(45) 让舵机转
      ④ 回显 "servo -> 45 deg" 给你看
*/
static void handle_serial_command(void)
{
    uint8_t i;
    uint32_t angle = 0U;

    /* ① 取一行：没有完整指令就直接返回，不往下走 */
    if(0U == usart1_get_line(s_rx_line, RX_LINE_MAX)) {
        return;
    }

    /* ② 把 ASCII 字符串解析成整数。
     *    例如 s_rx_line = "45"：
     *      第 1 个字符 '4'：angle = 0×10 + 4 = 4
     *      第 2 个字符 '5'：angle = 4×10 + 5 = 45
     *    其中 s_rx_line[i] - '0' 是把"字符数字"变成"真正数字"（ASCII 相减）。 */
    for(i = 0U; s_rx_line[i] != '\0'; i++) {
        angle = angle * 10U + (uint32_t)(s_rx_line[i] - '0');
    }

    /* ③ 钳位并设置舵机：角度超过 180 就按 180 处理 */
    if(angle > 180U) {
        angle = 180U;
    }
    servo_set_angle(angle);   /* 让舵机转到对应角度 */

    /* ④ 回显确认：在串口助手上打印 "servo -> 45 deg"，让你知道指令生效了 */
    usart1_send_string("servo -> ");
    usart1_send_string(s_rx_line);
    usart1_send_string(" deg\r\n");
}

/* ================================= 导出函数实现区 ================================= */

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    程序的入口。执行顺序：
       ① systick_config()  配置 1ms 心跳，给 delay_1ms 用
       ② led_init()        初始化 LED（PB5/PB6）
       ③ servo_init()      初始化舵机 PWM，并先转到 90°（居中）
       ④ usart1_init(115200) 初始化串口
       ⑤ while(1) 死循环：反复"处理串口指令"+"两个 LED 交替闪"
*/
int main(void)
{
    /* ① 配置 SysTick（系统滴答定时器），提供 delay_1ms() 延时 */
    systick_config();

    /* ② 初始化 PB5 / PB6 上的两个 LED */
    led_init();

    /* ③ 初始化舵机 PWM（TIMER1 CH2 -> PB10），上电默认转到 90° */
    servo_init();

    /* ④ 初始化 USART0（PA9/PA10，115200，板载 CH340C 串口） */
    usart1_init(115200U);

    /* ⑤ 主循环：永远不停地转 */
    while(1) {
        /* 处理串口舵机指令（有指令就执行，没有就跳过） */
        handle_serial_command();

        /* LED1(PB5) 亮，LED2(PB6) 灭 */
        led_on(LED1_GPIO_PORT, LED1_GPIO_PIN);
        led_off(LED2_GPIO_PORT, LED2_GPIO_PIN);
        delay_1ms(LED_BLINK_INTERVAL_MS);   /* 延时 250ms */

        /* 再查一次串口指令（让响应更及时） */
        handle_serial_command();

        /* LED1(PB5) 灭，LED2(PB6) 亮 —— 交替闪烁 */
        led_off(LED1_GPIO_PORT, LED1_GPIO_PIN);
        led_on(LED2_GPIO_PORT, LED2_GPIO_PIN);
        delay_1ms(LED_BLINK_INTERVAL_MS);   /* 延时 250ms */
    }
}

/*****************************************************************  END OF FILE  ***/
