/*****************************************************************
  Copyright (c) [2026], [开发者] All rights reserved
  File Name   : usart.c
  Description : USART0 串口驱动（PA9 TX / PA10 RX，中断接收）
  History     :
         2026-09-14   create
*****************************************************************/

/* ================================= 头文件包含区 ================================= */
#include "usart.h"   /* 里面定义了 PA9/PA10、USART0 等宏和函数声明 */

/* ================================= 私有宏定义区 ================================= */

/* ================================= 私有类型定义区 ================================= */

/* ================================= 私有全局变量区 ================================= */
/* 这三个变量是"接收缓冲区"，被中断函数和主循环共用：
 *   s_rx_buf        收到的字节存这里（比如 '4'、'5'）
 *   s_rx_len        已经存了几个字节
 *   s_rx_line_ready = 1 表示"已经收到完整一行（碰到回车了）"，等主循环来取
 * 注意：它们带 volatile，因为既被中断改、又被主循环读，防止编译器优化出错。 */
static volatile uint8_t s_rx_buf[USART1_RX_BUF_SIZE];  /* 接收缓冲区 */
static volatile uint8_t s_rx_len = 0;                  /* 已接收的字节数 */
static volatile uint8_t s_rx_line_ready = 0;           /* 一行就绪标志 */

/* ================================= 私有函数声明区 ================================= */

/* ================================= 私有函数实现区 ================================= */

/* ================================= 导出函数实现区 ================================= */

/*!
    \brief      初始化 USART0（8N1，中断接收）
    \param[in]  baud: 波特率，如 115200
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    又是熟悉的"外设驱动 5 步"：开时钟 → 配引脚 → 配串口 → 开中断 → 使能。
    8N1 的含义：8 位数据、无校验位、1 位停止位，这是最常用的串口格式。
*/
void usart1_init(uint32_t baud)
{
    /* ---- ① 开时钟：GPIOA（引脚）和 USART0（串口） ----------------------- */
    rcu_periph_clock_enable(USART1_GPIO_CLK);
    rcu_periph_clock_enable(USART1_CLK);

    /* ---- ② 配置 TX(PA9) 为"复用推挽输出" --------------------------------
     * TX 是发送脚，要往外推信号，所以设成复用+推挽输出。 */
    gpio_af_set(USART1_GPIO_PORT, USART1_GPIO_AF, USART1_TX_PIN);   /* 复用 AF7 */
    gpio_mode_set(USART1_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART1_TX_PIN);
    gpio_output_options_set(USART1_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN);

    /* ---- 配置 RX(PA10) 为"复用输入" -------------------------------------
     * RX 是接收脚，只负责收，不需要输出能力，所以只设模式和复用。 */
    gpio_af_set(USART1_GPIO_PORT, USART1_GPIO_AF, USART1_RX_PIN);   /* 复用 AF7 */
    gpio_mode_set(USART1_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART1_RX_PIN);

    /* ---- ③ 配置串口本体 -------------------------------------------------
     * 依次设置：波特率、无校验、8 位数据、1 位停止位、开接收、开发送。 */
    usart_deinit(USART1_PERIPH);                                  /* 恢复默认 */
    usart_baudrate_set(USART1_PERIPH, baud);                      /* 波特率 115200 */
    usart_parity_config(USART1_PERIPH, USART_PM_NONE);            /* 无校验位 */
    usart_word_length_set(USART1_PERIPH, USART_WL_8BIT);          /* 8 位数据 */
    usart_stop_bit_set(USART1_PERIPH, USART_STB_1BIT);            /* 1 位停止位 */
    usart_receive_config(USART1_PERIPH, USART_RECEIVE_ENABLE);    /* 打开接收 */
    usart_transmit_config(USART1_PERIPH, USART_TRANSMIT_ENABLE);  /* 打开发送 */

    /* ---- ④ 开启"接收中断" ----------------------------------------------
     * 意思是：每当收到一个字节，就触发中断，跳进 usart1_irq_handler。
     * nvic_irq_enable 后两个参数：优先级 1、子优先级 0。
     * 优先级比 SysTick（0）低一点，保证延时计数不受影响。 */
    usart_interrupt_enable(USART1_PERIPH, USART_INT_RBNE);  /* 开"接收非空"中断 */
    nvic_irq_enable(USART1_IRQ, 1, 0);                       /* 在 NVIC 里使能这个中断号 */

    /* ---- ⑤ 使能串口总开关 ---------------------------------------------- */
    usart_enable(USART1_PERIPH);
}

/*!
    \brief      发送单个字节
    \param[in]  ch: 待发送字节
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    发送的套路：把数据写进"发送寄存器"，然后等硬件把它发完。
    那个 while 循环就是在"等发送完"（TBE = 发送缓冲空）。
*/
void usart1_send_char(uint8_t ch)
{
    usart_data_transmit(USART1_PERIPH, (uint16_t)ch);   /* 写入发送寄存器 */

    /* 等待"发送缓冲空"标志，直到上一个字节真正发出去再返回 */
    while(RESET == usart_flag_get(USART1_PERIPH, USART_FLAG_TBE)) {
    }
}

/*!
    \brief      发送字符串
    \param[in]  str: 以 '\0' 结尾的字符串
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    其实就是把字符串里每个字符逐个调 send_char 发出去，直到碰到结尾 '\0'。
*/
void usart1_send_string(const char *str)
{
    while('\0' != *str) {
        usart1_send_char((uint8_t)(*str++));   /* 发一个字符，指针后移 */
    }
}

/*!
    \brief      轮询获取一行指令
    \param[in]  buf: 接收缓冲区（把收到的一行拷到这里）
    \param[in]  maxlen: 缓冲区大小
    \param[out] none
    \retval      1 = 收到完整一行，0 = 暂无
    ------------------------------------------------------------
    主循环反复调用这个函数"问"：有没有攒好一行？
    有 → 把内容拷给调用者，清空状态，返回 1；
    没有 → 直接返回 0。
*/
uint8_t usart1_get_line(char *buf, uint8_t maxlen)
{
    uint8_t i;

    /* 没有攒好一行，直接返回 0 */
    if(0U == s_rx_line_ready) {
        return 0U;
    }

    /* 有完整一行：把 s_rx_buf 里的字符逐个拷到调用者的 buf，最多拷 maxlen-1 个 */
    for(i = 0U; (i < s_rx_len) && (i < (maxlen - 1U)); i++) {
        buf[i] = (char)s_rx_buf[i];
    }
    buf[i] = '\0';   /* 末尾补 '\0'，形成标准 C 字符串 */

    /* 清空状态，表示"这一行已经被取走了"，等待下一次输入 */
    s_rx_line_ready = 0U;
    s_rx_len = 0U;

    return 1U;
}

/*!
    \brief      接收中断处理（由 USART0_IRQHandler 调用）
    \param[in]  none
    \param[out] none
    \retval     none
    ------------------------------------------------------------
    这是整个串口接收的"心脏"。每收到一个字节，硬件就触发中断，
    跳进这里执行。逻辑分三种情况：
       · 收到 '\r' 或 '\n'（回车/换行）→ 一行结束，置 ready 标志
       · 收到 '0'~'9'（数字）       → 存进缓冲区
       · 收到其他字符（空格、小数点）→ 当作非法，丢弃当前累计重新开始
*/
void usart1_irq_handler(void)
{
    uint8_t ch;

    /* 确认是"接收非空"中断（有字节到了），才往下处理 */
    if(RESET != usart_interrupt_flag_get(USART1_PERIPH, USART_INT_FLAG_RBNE)) {
        ch = (uint8_t)usart_data_receive(USART1_PERIPH);   /* 读出这个字节（同时清标志） */

        if(('\r' == ch) || ('\n' == ch)) {
            /* 情况①：收到回车或换行 = 一行结束 */
            if(s_rx_len > 0U) {          /* 只要之前存过至少一个数字 */
                s_rx_line_ready = 1U;    /* 就标记"一行就绪"，等主循环取走 */
            }
        } else if((ch >= '0') && (ch <= '9')) {
            /* 情况②：收到数字，存进缓冲区 */
            if(s_rx_line_ready) {        /* 如果上一行还没被取走，先清掉重新开始 */
                s_rx_len = 0U;
                s_rx_line_ready = 0U;
            }
            if(s_rx_len < (USART1_RX_BUF_SIZE - 1U)) {   /* 防止溢出 */
                s_rx_buf[s_rx_len++] = ch;               /* 存入，长度 +1 */
            }
        } else {
            /* 情况③：非法字符（空格、小数点等），丢弃当前累计 */
            s_rx_len = 0U;
            s_rx_line_ready = 0U;
        }
    }
}

/*****************************************************************  END OF FILE  ***/
