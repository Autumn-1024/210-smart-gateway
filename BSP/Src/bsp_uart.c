/**
 ****************************************************************************************************
 * @file        bsp_uart.c
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-16
 * @brief       USART1 调试串口驱动 (PA9/PA10, printf重定向)
 ****************************************************************************************************
 */

#include "bsp_uart.h"

static UART_HandleTypeDef g_uart_handle;

/**
 * @brief       USART1初始化
 * @param       bound: 波特率
 * @retval      无
 */
void bsp_uart_init(uint32_t bound)
{
    GPIO_InitTypeDef gpio_init_struct;

    DEBUG_TX_GPIO_CLK_ENABLE();
    DEBUG_RX_GPIO_CLK_ENABLE();
    DEBUG_USART_CLK_ENABLE();

    /* TX - 复用推挽输出 */
    gpio_init_struct.Pin   = DEBUG_TX_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_TX_GPIO_PORT, &gpio_init_struct);

    /* RX - 浮空输入 */
    gpio_init_struct.Pin  = DEBUG_RX_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DEBUG_RX_GPIO_PORT, &gpio_init_struct);

    /* USART配置 */
    g_uart_handle.Instance          = DEBUG_USART;
    g_uart_handle.Init.BaudRate     = bound;
    g_uart_handle.Init.WordLength   = UART_WORDLENGTH_8B;
    g_uart_handle.Init.StopBits     = UART_STOPBITS_1;
    g_uart_handle.Init.Parity       = UART_PARITY_NONE;
    g_uart_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    g_uart_handle.Init.Mode         = UART_MODE_TX_RX;
    HAL_UART_Init(&g_uart_handle);
}

/**
 * @brief       重定向fputc到USART1 (支持printf)
 */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&g_uart_handle, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief       重定向fgetc到USART1 (支持scanf)
 */
int fgetc(FILE *f)
{
    uint8_t ch = 0;
    HAL_UART_Receive(&g_uart_handle, &ch, 1, HAL_MAX_DELAY);
    return (int)ch;
}
