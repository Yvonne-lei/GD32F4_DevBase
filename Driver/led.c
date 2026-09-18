/*****************************************************************
  Copyright (c) [2026], [开发者] All rights reserved
  File Name   : led.c
  Description : 板载 LED 驱动（LED1 -> PB5，LED2 -> PB6）
  History     :
         2026-09-13   create
*****************************************************************/

/* ================================= 头文件包含区 ================================= */
#include "led.h"

/* ================================= 私有宏定义区 ================================= */

/* ================================= 私有类型定义区 ================================= */

/* ================================= 私有全局变量区 ================================= */

/* ================================= 私有函数声明区 ================================= */

/* ================================= 私有函数实现区 ================================= */

/* ================================= 导出函数实现区 ================================= */

/*!
    \brief      初始化 LED 的 GPIO
    \param[in]  none
    \param[out] none
    \retval     none
*/
void led_init(void)
{
    /* 使能 LED 所在 GPIO 端口的时钟 */
    rcu_periph_clock_enable(LED_GPIO_CLK);

    /* 配置为推挽输出 */
    gpio_mode_set(LED1_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  LED1_GPIO_PIN | LED2_GPIO_PIN);
    gpio_output_options_set(LED1_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            LED1_GPIO_PIN | LED2_GPIO_PIN);

    /* 上电默认熄灭 */
    led_off(LED1_GPIO_PORT, LED1_GPIO_PIN);
    led_off(LED2_GPIO_PORT, LED2_GPIO_PIN);
}

/*!
    \brief      点亮指定的 LED
    \param[in]  gpio_periph: GPIO 端口，如 GPIOB
    \param[in]  pin: 引脚，如 GPIO_PIN_5
    \param[out] none
    \retval     none
*/
void led_on(uint32_t gpio_periph, uint32_t pin)
{
#if (LED_ON_LEVEL == 1U)
    gpio_bit_set(gpio_periph, pin);
#else
    gpio_bit_reset(gpio_periph, pin);
#endif
}

/*!
    \brief      熄灭指定的 LED
    \param[in]  gpio_periph: GPIO 端口，如 GPIOB
    \param[in]  pin: 引脚，如 GPIO_PIN_5
    \param[out] none
    \retval     none
*/
void led_off(uint32_t gpio_periph, uint32_t pin)
{
#if (LED_ON_LEVEL == 1U)
    gpio_bit_reset(gpio_periph, pin);
#else
    gpio_bit_set(gpio_periph, pin);
#endif
}

/*!
    \brief      翻转指定的 LED
    \param[in]  gpio_periph: GPIO 端口，如 GPIOB
    \param[in]  pin: 引脚，如 GPIO_PIN_5
    \param[out] none
    \retval     none
*/
void led_toggle(uint32_t gpio_periph, uint32_t pin)
{
    gpio_bit_toggle(gpio_periph, pin);
}

/*****************************************************************  END OF FILE  ***/