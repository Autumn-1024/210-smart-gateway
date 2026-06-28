/**
 ****************************************************************************************************
 * @file        bsp_uart3.h
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-28
 * @brief       USART3 驱动 — 接收中控触摸屏 RS485 信号 (PB10/PB11)
 ****************************************************************************************************
 */

#ifndef __BSP_UART3_H
#define __BSP_UART3_H

#include "stm32f1xx_hal.h"

#define UART3_RX_BUF_SIZE   64

void bsp_uart3_init(uint32_t bound);
uint16_t bsp_uart3_get_data(uint8_t *buf, uint16_t buf_size);

#endif
