/**
 ****************************************************************************************************
 * @file        bsp_rs485.h
 * @author      Autumn
 * @version     V1.1
 * @date        2026-06-16
 * @brief       RS485 驱动 (USART2, 自动方向)
 ****************************************************************************************************
 * @attention
 *
 * 平台: STM32F103C8T6
 * 接线: PA2(TX), PA3(RX)
 * RS485模块自动方向控制，无需额外DE/RE引脚
 *
 ****************************************************************************************************
 */

#ifndef __BSP_RS485_H
#define __BSP_RS485_H

#include "stm32f1xx_hal.h"

/******************************************************************************************/
/* RS485 硬件引脚 */

#define RS485_TX_GPIO_PORT          GPIOA
#define RS485_TX_GPIO_PIN           GPIO_PIN_2
#define RS485_TX_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define RS485_RX_GPIO_PORT          GPIOA
#define RS485_RX_GPIO_PIN           GPIO_PIN_3
#define RS485_RX_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define RS485_USART                 USART2
#define RS485_USART_IRQn            USART2_IRQn
#define RS485_USART_IRQHandler      USART2_IRQHandler
#define RS485_USART_CLK_ENABLE()    do{ __HAL_RCC_USART2_CLK_ENABLE(); }while(0)

/******************************************************************************************/
/* 配置 */

#define RS485_REC_LEN               64
#define RS485_EN_RX                 1

/******************************************************************************************/
/* 外部变量 */

extern uint8_t g_rs485_rx_buf[RS485_REC_LEN];
extern uint8_t g_rs485_rx_cnt;

/******************************************************************************************/
/* 函数声明 */

void bsp_rs485_init(uint32_t bound);
void bsp_rs485_send_data(uint8_t *buf, uint8_t len);
void bsp_rs485_receive_data(uint8_t *buf, uint8_t *len);

#endif
