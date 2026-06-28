/**
 ****************************************************************************************************
 * @file        bsp_uart3.c
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-28
 * @brief       USART3 驱动 — 接收中控触摸屏 RS485 信号 (PB10/PB11)
 ****************************************************************************************************
 * @attention
 *
 * 引脚: PB10(TX), PB11(RX)
 * 波特率: 115200
 * 协议: 8字节帧, 字节[0]=地址, 字节[4]=控制位(0x00关/0xFF开)
 *
 ****************************************************************************************************
 */

#include "bsp_uart3.h"
#include <string.h>

/******************************************************************************************/
/* 私有变量 */

static UART_HandleTypeDef s_uart3_handle;
static uint8_t s_rx_buf[UART3_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/******************************************************************************************/
/* 底层 — 直接在中断中读取，不依赖 HAL 回调 */

void USART3_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&s_uart3_handle, UART_FLAG_RXNE) != RESET)
    {
        uint8_t ch = (uint8_t)(s_uart3_handle.Instance->DR & 0xFF);
        uint16_t next = (s_rx_head + 1) % UART3_RX_BUF_SIZE;
        if (next != s_rx_tail)
        {
            s_rx_buf[s_rx_head] = ch;
            s_rx_head = next;
        }
    }
    __HAL_UART_CLEAR_PEFLAG(&s_uart3_handle);
}

/******************************************************************************************/
/* 公开接口 */

/**
 * @brief       USART3 初始化 (PB10=TX, PB11=RX)
 */
void bsp_uart3_init(uint32_t bound)
{
    GPIO_InitTypeDef gpio_init;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    /* TX - PB10 复用推挽 */
    gpio_init.Pin   = GPIO_PIN_10;
    gpio_init.Mode  = GPIO_MODE_AF_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    /* RX - PB11 浮空输入 */
    gpio_init.Pin  = GPIO_PIN_11;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    /* USART 配置 */
    s_uart3_handle.Instance          = USART3;
    s_uart3_handle.Init.BaudRate     = bound;
    s_uart3_handle.Init.WordLength   = UART_WORDLENGTH_8B;
    s_uart3_handle.Init.StopBits     = UART_STOPBITS_1;
    s_uart3_handle.Init.Parity       = UART_PARITY_NONE;
    s_uart3_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    s_uart3_handle.Init.Mode         = UART_MODE_TX_RX;
    HAL_UART_Init(&s_uart3_handle);

    /* 开启接收中断 (直接寄存器方式，不依赖 HAL 回调) */
    __HAL_UART_ENABLE_IT(&s_uart3_handle, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART3_IRQn, 3, 1);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

/**
 * @brief       读取接收缓冲区数据
 * @param       buf: 输出缓冲区
 * @param       buf_size: 缓冲区大小
 * @retval      读取到的字节数
 */
uint16_t bsp_uart3_get_data(uint8_t *buf, uint16_t buf_size)
{
    uint16_t cnt = 0;
    while (s_rx_tail != s_rx_head && cnt < buf_size)
    {
        buf[cnt++] = s_rx_buf[s_rx_tail];
        s_rx_tail = (s_rx_tail + 1) % UART3_RX_BUF_SIZE;
    }
    return cnt;
}
