/*****************************************************************
  Copyright (c) [2026], [开发者] All rights reserved
  File Name   : servo.c
  Description : PWM 舵机驱动（TIMER1 通道2 -> PB10，20ms 周期 / 50Hz）
  History     :
         2026-09-14   create
*****************************************************************/

/* ================================= 头文件包含区 ================================= */
#include "servo.h"   /* 里面定义了 PB10 / TIMER1 / 通道2 这些"引脚和外设"的宏 */

/* ================================= 私有宏定义区 ================================= */
/* ----------------------------------------------------------------------------
 * 定时器的三个核心参数，共同决定了输出波形的"周期"和"脉宽"。
 * 结论先放这，再看下面的推导：
 *   定时器时钟 = 84MHz
 *   预分频 = 83     →  84MHz ÷ (83+1) = 1MHz，也就是"计数器数 1 下 = 1 微秒"
 *   周期   = 19999  →  数 20000 下 = 20000 微秒 = 20ms（一个完整周期 = 50Hz）
 * ----------------------------------------------------------------------------
 */

/* 预分频：把 84MHz 的定时器时钟降到 1MHz。
 * 为什么要 1MHz？因为这样"计数器每数一下 = 1 微秒"，
 * 后面算脉宽时就能直接用"微秒"当单位，特别直观。 */
#define SERVO_TIMER_PRESCALER       83U

/* 周期：计数器从 0 数到 19999 后归零，一共 20000 个数。
 * 20000 个数 × 1 微秒/个 = 20000 微秒 = 20ms = 50Hz（舵机要求的标准刷新率）。 */
#define SERVO_TIMER_PERIOD          19999U

/* 脉宽范围（单位：微秒，因为 1 计数 = 1us，所以这个值直接就是微秒数）：
 *   500  = 0.5ms = 0°（舵机转到一个极限）
 *   1500 = 1.5ms = 90°（居中）
 *   2500 = 2.5ms = 180°（转到另一个极限）
 * 这是所有标准舵机通用的约定，SG90 / MG996R 都认这个范围。 */
#define SERVO_PULSE_MIN             500U
#define SERVO_PULSE_MAX             2500U

/* ================================= 私有类型定义区 ================================= */

/* ================================= 私有全局变量区 ================================= */

/* ================================= 私有函数声明区 ================================= */

/* ================================= 私有函数实现区 ================================= */

/* ================================= 导出函数实现区 ================================= */

/*!
    \brief      初始化舵机 PWM（50Hz，1MHz 计数）
    \param[in]  none
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    初始化流程共 5 步：
       ① 开时钟   ② 配 GPIO 引脚   ③ 配定时器   ④ 配 PWM 通道   ⑤ 使能定时器
*/
void servo_init(void)
{
    /* 这两个结构体是厂商库规定的"配置参数包"，
       先把所有参数塞进结构体，再一次性传给初始化函数。 */
    timer_parameter_struct timer_initpara;    /* 定时器的基础配置（分频、周期等） */
    timer_oc_parameter_struct timer_ocpara;   /* 输出比较/PWM 的配置（极性、使能等） */

    /* ---- ① 开时钟 --------------------------------------------------------
     * 单片机为了省电，所有外设默认都是"断电"状态，用之前必须先给它们通电。
     * 这里要开两个时钟：GPIOB（引脚所在的端口）和 TIMER1（定时器本身）。
     * --------------------------------------------------------------------- */
    rcu_periph_clock_enable(SERVO_GPIO_CLK);    /* 给 GPIOB 端口通电 */
    rcu_periph_clock_enable(SERVO_TIMER_CLK);   /* 给 TIMER1 定时器通电 */

    /* ---- ② 配置 PB10 引脚为"复用功能" ------------------------------------
     * PB10 默认是普通 IO 口，要让它变成"定时器通道2的输出脚"，需要三步：
     *   gpio_mode_set           → 设成"复用功能"模式（AF = Alternate Function）
     *   gpio_output_options_set → 推挽输出 + 50MHz 速度（推挽才能有力驱动波形）
     *   gpio_af_set             → 指定复用哪个功能：AF1 = TIMER1 的通道
     * --------------------------------------------------------------------- */
    gpio_mode_set(SERVO_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SERVO_GPIO_PIN);
    gpio_output_options_set(SERVO_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SERVO_GPIO_PIN);
    gpio_af_set(SERVO_GPIO_PORT, SERVO_GPIO_AF, SERVO_GPIO_PIN);

    /* ---- ③ 配置定时器的"计数方式" ----------------------------------------
     * timer_deinit：先把定时器恢复成默认状态（防止残留上次的配置）。
     * timer_init：设好下面几个关键参数：
     *   prescaler        = 83      → 84MHz ÷ 84 = 1MHz（数 1 下 = 1 微秒）
     *   period           = 19999   → 数 20000 下 = 20ms 一个周期
     *   counterdirection = UP      → 从 0 往上数
     *   alignedmode      = EDGE    → 普通计数模式（不用中心对齐）
     * --------------------------------------------------------------------- */
    timer_deinit(SERVO_TIMER);
    timer_initpara.prescaler         = SERVO_TIMER_PRESCALER;   /* 分频系数 83 */
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;      /* 边沿对齐计数 */
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;        /* 向上计数 */
    timer_initpara.period            = SERVO_TIMER_PERIOD;      /* 周期 19999（=20ms） */
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;        /* 时钟不分频 */
    timer_initpara.repetitioncounter = 0;                       /* 不重复计数 */
    timer_init(SERVO_TIMER, &timer_initpara);

    /* ---- ④ 配置通道2为 PWM 输出 ------------------------------------------
     * outputstate  = ENABLE  → 打开这个通道的输出
     * ocpolarity   = HIGH    → 输出极性（高电平有效）
     * outputnstate = DISABLE → 关掉互补输出（TIMER1 是通用定时器，用不到）
     * --------------------------------------------------------------------- */
    timer_ocpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;   /* 高电平有效 */
    timer_ocpara.outputstate  = TIMER_CCX_ENABLE;         /* 使能通道输出 */
    timer_ocpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;  /* 互补输出极性（备用） */
    timer_ocpara.outputnstate = TIMER_CCXN_DISABLE;       /* 关闭互补输出 */
    timer_ocpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;  /* 空闲时输出低电平 */
    timer_ocpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW; /* 互补空闲时低电平 */
    timer_channel_output_config(SERVO_TIMER, SERVO_TIMER_CH, &timer_ocpara);

    /* 设成 PWM 模式 0：计数器小于"比较值"时输出高，大于等于时输出低。
     * 这样"比较值"就等于"高电平持续的时间（微秒）"。 */
    timer_channel_output_mode_config(SERVO_TIMER, SERVO_TIMER_CH, TIMER_OC_MODE_PWM0);

    /* 关掉"影子寄存器"：让改比较值时立即生效，而不是等下一个周期才更新。
     * 对舵机来说，我们希望"一发指令马上动"，所以关掉它。 */
    timer_channel_output_shadow_config(SERVO_TIMER, SERVO_TIMER_CH, TIMER_OC_SHADOW_DISABLE);

    /* 先给一个初始比较值 1500（= 1.5ms = 90°），防止上电瞬间输出乱跳。 */
    timer_channel_output_pulse_value_config(SERVO_TIMER, SERVO_TIMER_CH, 1500U);

    /* 使能"自动重载影子"：让周期值 19999 稳定生效（标准做法）。 */
    timer_auto_reload_shadow_enable(SERVO_TIMER);

    /* ---- ⑤ 打开定时器总开关，开始计数输出波形 ---------------------------- */
    timer_enable(SERVO_TIMER);

    /* 上电默认转到 90°（居中），方便你知道舵机在工作 */
    servo_set_angle(90U);
}

/*!
    \brief      设置舵机角度
    \param[in]  angle: 目标角度（0 ~ 180），超出会自动钳到 180
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    这是你串口发数字后最终会调到的函数，核心就一个公式：
        脉宽 = 500 + (角度 × 2000) / 180
    例如：角度 45  →  脉宽 = 500 + 45×2000/180 = 1000 微秒 = 1.0ms
*/
void servo_set_angle(uint32_t angle)
{
    uint32_t pulse;   /* 算出来的脉宽（微秒） */

    /* 角度钳位：防止输入超过 180° 导致算出越界的脉宽 */
    if(angle > SERVO_ANGLE_MAX) {
        angle = SERVO_ANGLE_MAX;
    }

    /* 关键公式：把 0~180° 线性映射到 0.5ms~2.5ms（即 500~2500 微秒）
     *   角度 0   → 500 + 0        = 500  （0.5ms）
     *   角度 90  → 500 + 1000     = 1500 （1.5ms）
     *   角度 180 → 500 + 2000     = 2500 （2.5ms）
     */
    pulse = SERVO_PULSE_MIN + (angle * (SERVO_PULSE_MAX - SERVO_PULSE_MIN)) / SERVO_ANGLE_MAX;

    /* 把算好的脉宽写进"通道2 比较寄存器"。
     * 写进去后，定时器下一个周期就会按新的高电平时间输出，
     * 舵机检测到脉宽变化，就把轴转到对应角度。 */
    timer_channel_output_pulse_value_config(SERVO_TIMER, SERVO_TIMER_CH, pulse);
}

/*****************************************************************  END OF FILE  ***/
