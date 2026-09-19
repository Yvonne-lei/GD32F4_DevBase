/*****************************************************************
  Copyright (c) [2026], [开发者] All rights reserved
  File Name   : timer.c
  Description : 定时器 PWM 驱动（LED 呼吸灯：PB5->TIMER2，PB6->TIMER3）
  History     :
         2026-09-19   create
*****************************************************************/

/* ================================= 头文件包含区 ================================= */
#include "timer.h"   /* 里面定义了 PB5/PB6、TIMER2/TIMER3、通道等宏 */

/* ================================= 私有宏定义区 ================================= */

/* ================================= 私有类型定义区 ================================= */

/* ================================= 私有全局变量区 ================================= */

/* ================================= 私有函数声明区 ================================= */
/* 初始化"一个定时器的一个 PWM 通道"。
 * 两个 LED 的初始化流程完全一样，只有引脚/定时器/通道不同，
 * 所以抽成一个函数避免代码重复。 */
static void pwm_channel_init(uint32_t port,  uint32_t port_clk,
                             uint32_t pin,   uint32_t af,
                             uint32_t timer, uint32_t timer_clk,
                             uint32_t ch);

/* ================================= 私有函数实现区 ================================= */

/*!
    \brief      初始化一个 PWM 通道（LED1/LED2 通用）
    \param[in]  port/port_clk : GPIO 端口及其时钟
    \param[in]  pin/af        : 引脚号及复用功能编号
    \param[in]  timer/timer_clk: 定时器及其时钟
    \param[in]  ch            : 定时器通道
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    还是熟悉的"外设 5 步"：开时钟 → 配引脚 → 配定时器 → 配 PWM 通道 → 使能。
    和舵机 servo_init() 一模一样，只是引脚、定时器、通道这些参数不同。
*/
static void pwm_channel_init(uint32_t port,  uint32_t port_clk,
                             uint32_t pin,   uint32_t af,
                             uint32_t timer, uint32_t timer_clk,
                             uint32_t ch)
{
    timer_parameter_struct timer_initpara;    /* 定时器基础配置 */
    timer_oc_parameter_struct timer_ocpara;   /* PWM 通道配置 */

    /* ---- ① 开时钟：GPIO 端口 + 定时器 ------------------------------- */
    rcu_periph_clock_enable(port_clk);
    rcu_periph_clock_enable(timer_clk);

    /* ---- ② 配引脚为"复用推挽输出"（和舵机 PB10 同样的套路） -------- */
    gpio_mode_set(port, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
    gpio_output_options_set(port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pin);
    gpio_af_set(port, af, pin);

    /* ---- ③ 配定时器：84MHz/84 = 1MHz，周期 999 = 1kHz ---------------- */
    timer_deinit(timer);
    timer_initpara.prescaler         = TIMER_PWM_PRESCALER;   /* 83 */
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = TIMER_PWM_PERIOD;      /* 999 */
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(timer, &timer_initpara);

    /* ---- ④ 配通道为 PWM 模式 0 -------------------------------------
     * PWM 模式 0：计数器小于"比较值"时输出高，大于等于时输出低。
     * 所以"比较值"就决定了高电平占比 = 亮度。
     * 关掉影子寄存器，让改比较值时立即生效（呼吸灯要随时改亮度）。 */
    timer_ocpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(timer, ch, &timer_ocpara);
    timer_channel_output_mode_config(timer, ch, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(timer, ch, TIMER_OC_SHADOW_DISABLE);

    /* 初始亮度 0（上电时 LED 灭），防止上电瞬间乱闪 */
    timer_channel_output_pulse_value_config(timer, ch, 0U);

    timer_auto_reload_shadow_enable(timer);

    /* ---- ⑤ 使能定时器，开始输出 PWM -------------------------------- */
    timer_enable(timer);
}

/* ================================= 导出函数实现区 ================================= */

/*!
    \brief      初始化两个 LED 的 PWM（呼吸灯用）
    \param[in]  none
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    分别调用 pwm_channel_init 配好两个灯，上电默认都是熄灭状态。
*/
void timer_pwm_init(void)
{
    /* LED1：PB5 -> TIMER2_CH1 */
    pwm_channel_init(TIMER_LED1_PORT, TIMER_LED1_CLK,
                     TIMER_LED1_PIN,  TIMER_LED1_AF,
                     TIMER_LED1_TIMER, TIMER_LED1_TIMER_CLK,
                     TIMER_LED1_CH);

    /* LED2：PB6 -> TIMER3_CH0 */
    pwm_channel_init(TIMER_LED2_PORT, TIMER_LED2_CLK,
                     TIMER_LED2_PIN,  TIMER_LED2_AF,
                     TIMER_LED2_TIMER, TIMER_LED2_TIMER_CLK,
                     TIMER_LED2_CH);
}

/*!
    \brief      设置某个 LED 的亮度
    \param[in]  led     : 选择哪个灯（TIMER_LED1 或 TIMER_LED2）
    \param[in]  percent : 亮度百分比 0~100，超出自动钳到 100
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    核心就是把 0~100% 映射成比较值 0~999：
        比较值 = percent × 999 / 100
    例如：50% → 499，100% → 999（占空比 100%，最亮），0% → 0（灭）。
    写进比较寄存器后，定时器下一个周期就会按新的占空比输出。
*/
void timer_pwm_set(uint8_t led, uint8_t percent)
{
    uint32_t pulse;

    /* 亮度钳位，防止超过 100% */
    if(percent > 100U) {
        percent = 100U;
    }

    /* 百分比 → 比较值（0~999） */
    pulse = (uint32_t)percent * TIMER_PWM_PERIOD / 100U;

    /* 根据灯编号，把比较值写进对应定时器的对应通道 */
    if(TIMER_LED1 == led) {
        timer_channel_output_pulse_value_config(TIMER_LED1_TIMER, TIMER_LED1_CH, pulse);
    } else {
        timer_channel_output_pulse_value_config(TIMER_LED2_TIMER, TIMER_LED2_CH, pulse);
    }
}

/*****************************************************************  END OF FILE  ***/
