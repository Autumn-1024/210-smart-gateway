/**
 ****************************************************************************************************
 * @file        bsp_rs485.c
 * @author      Autumn
 * @version     V1.1
 * @date        2026-06-16
 * @brief       RS485 驱动 (USART2, PA2/PA3, 自动方向)
 ****************************************************************************************************
 */

#include "bsp_rs485.h"

static UART_HandleTypeDef g_rs485_handle;

#if RS485_EN_RX
uint8_t g_rs485_rx_buf[RS485_REC_LEN];
uint8_t g_rs485_rx_cnt = 0;

/**
 * @brief       USART2中断服务函数
 */
void RS485_USART_IRQHandler(void)
{
    uint8_t res;

    if (__HAL_UART_GET_FLAG(&g_rs485_handle, UART_FLAG_RXNE) != RESET)
    {
        HAL_UART_Receive(&g_rs485_handle, &res, 1, 1000);

        if (g_rs485_rx_cnt < RS485_REC_LEN)
        {
            g_rs485_rx_buf[g_rs485_rx_cnt] = res;
            g_rs485_rx_cnt++;
        }
    }
}
#endif

/**
 * @brief       RS485初始化
 * @param       bound: 波特率
 * @retval      无
 */
void bsp_rs485_init(uint32_t bound)
{
    GPIO_InitTypeDef gpio_init_struct;

    RS485_TX_GPIO_CLK_ENABLE();
    RS485_RX_GPIO_CLK_ENABLE();
    RS485_USART_CLK_ENABLE();

    /* TX - 复用推挽输出 */
    gpio_init_struct.Pin   = RS485_TX_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_TX_GPIO_PORT, &gpio_init_struct);

    /* RX - 浮空输入 */
    gpio_init_struct.Pin  = RS485_RX_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(RS485_RX_GPIO_PORT, &gpio_init_struct);

    /* USART配置 */
    g_rs485_handle.Instance          = RS485_USART;
    g_rs485_handle.Init.BaudRate     = bound;
    g_rs485_handle.Init.WordLength   = UART_WORDLENGTH_8B;
    g_rs485_handle.Init.StopBits     = UART_STOPBITS_1;
    g_rs485_handle.Init.Parity       = UART_PARITY_NONE;
    g_rs485_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    g_rs485_handle.Init.Mode         = UART_MODE_TX_RX;
    HAL_UART_Init(&g_rs485_handle);

    __HAL_UART_DISABLE_IT(&g_rs485_handle, UART_IT_TC);

#if RS485_EN_RX
    __HAL_UART_ENABLE_IT(&g_rs485_handle, UART_IT_RXNE);
    HAL_NVIC_EnableIRQ(RS485_USART_IRQn);
    HAL_NVIC_SetPriority(RS485_USART_IRQn, 3, 3);
#endif
}

/**
 * @brief       RS485发送数据
 * @param       buf: 数据缓冲区
 * @param       len: 数据长度
 * @retval      无
 */
void bsp_rs485_send_data(uint8_t *buf, uint8_t len)
{
    HAL_UART_Transmit(&g_rs485_handle, buf, len, 1000);
    g_rs485_rx_cnt = 0;
}

/**
 * @brief       RS485接收数据(轮询)
 * @param       buf: 接收缓冲区
 * @param       len: 接收长度指针
 * @retval      无
 */
void bsp_rs485_receive_data(uint8_t *buf, uint8_t *len)
{
    uint8_t rxlen = g_rs485_rx_cnt;
    uint8_t i;

    *len = 0;
    HAL_Delay(10);

    if (rxlen == g_rs485_rx_cnt && rxlen)
    {
        for (i = 0; i < rxlen; i++)
        {
            buf[i] = g_rs485_rx_buf[i];
        }

        *len = g_rs485_rx_cnt;
        g_rs485_rx_cnt = 0;
    }
}
