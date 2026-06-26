/**
 ****************************************************************************************************
 * @file        bsp_uart.h
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-16
 * @brief       USART1 调试串口驱动
 ****************************************************************************************************
 * @attention
 *
 * 平台: STM32F103C8T6
 * 接线: PA9(TX), PA10(RX)
 * 功能: printf重定向
 *
 ****************************************************************************************************
 */

#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "stm32f1xx_hal.h"
#include <stdio.h>

#define DEBUG_USART                 USART1
#define DEBUG_USART_IRQn            USART1_IRQn
#define DEBUG_USART_IRQHandler      USART1_IRQHandler
#define DEBUG_USART_CLK_ENABLE()    do{ __HAL_RCC_USART1_CLK_ENABLE(); }while(0)

#define DEBUG_TX_GPIO_PORT          GPIOA
#define DEBUG_TX_GPIO_PIN           GPIO_PIN_9
#define DEBUG_TX_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define DEBUG_RX_GPIO_PORT          GPIOA
#define DEBUG_RX_GPIO_PIN           GPIO_PIN_10
#define DEBUG_RX_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

void bsp_uart_init(uint32_t bound);

#endif
