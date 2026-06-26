/**
 ****************************************************************************************************
 * @file        bsp_led.h
 * @author      STM32F103 LED Driver
 * @version     V1.0
 * @date        2024
 * @brief       LED驱动头文件
 ****************************************************************************************************
 * @attention
 *
 * 平台: STM32F103C8T6
 *
 ****************************************************************************************************
 */

#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "stm32f1xx_hal.h"

/******************************************************************************************/
/* LED引脚定义 */

#define LED_GPIO_PORT                  GPIOC                                  /* LED端口 */
#define LED_GPIO_PIN                   GPIO_PIN_13                            /* LED引脚号 */

#define LED_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* GPIOC时钟使能 */

/******************************************************************************************/
/* LED控制宏定义 */

#define LED(x)   do{ x ? \
                      HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET) : \
                      HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_RESET); \
                  }while(0)      /* LED控制，0点亮，1关闭 */

#define LED_TOGGLE()   do{ HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN); }while(0)   /* LED翻转 */

/******************************************************************************************/
/* 函数声明 */

void bsp_led_init(void);                                                                  /* LED初始化函数 */

#endif
