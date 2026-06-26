/**
 ****************************************************************************************************
 * @file        bsp_key.h
 * @author      Autumn
 * @version     V1.2
 * @date        2026-06-16
 * @brief       按键驱动
 ****************************************************************************************************
 * @attention
 *
 * 平台: STM32F103C8T6
 * 按键: KEY0=PA4, KEY1=PA5, KEY2=PA6, KEY_UP=PA7
 * 有效电平: KEY0/1/2 低电平有效, KEY_UP 高电平有效
 *
 ****************************************************************************************************
 */

#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "stm32f1xx_hal.h"

/******************************************************************************************/
/* 按键引脚定义 */

#define KEY0_GPIO_PORT          GPIOA
#define KEY0_GPIO_PIN           GPIO_PIN_4
#define KEY0_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define KEY1_GPIO_PORT          GPIOA
#define KEY1_GPIO_PIN           GPIO_PIN_5
#define KEY1_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define KEY2_GPIO_PORT          GPIOA
#define KEY2_GPIO_PIN           GPIO_PIN_6
#define KEY2_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define KEY3_GPIO_PORT          GPIOA
#define KEY3_GPIO_PIN           GPIO_PIN_7
#define KEY3_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

/******************************************************************************************/
/* 按键读取 */

#define KEY0        HAL_GPIO_ReadPin(KEY0_GPIO_PORT, KEY0_GPIO_PIN)
#define KEY1        HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN)
#define KEY2        HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN)
#define KEY3        HAL_GPIO_ReadPin(KEY3_GPIO_PORT, KEY3_GPIO_PIN)

/* 返回值定义 */
#define KEY0_PRES   1
#define KEY1_PRES   2
#define KEY2_PRES   3
#define KEY3_PRES   4

/******************************************************************************************/
/* 函数声明 */

void bsp_key_init(void);
uint8_t bsp_key_scan(uint8_t mode);

#endif
