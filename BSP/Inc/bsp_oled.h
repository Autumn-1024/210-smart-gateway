/**
 ****************************************************************************************************
 * @file        bsp_oled.h
 * @author      Autumn
 * @version     V2.0
 * @date        2026-06-16
 * @brief       OLED SSD1306 驱动 (IIC模式)
 ****************************************************************************************************
 * @attention
 *
 * 平台: STM32F103C8T6
 * 接线:
 *   PB6 -> OLED_SCL
 *   PB7 -> OLED_SDA
 *
 ****************************************************************************************************
 */

#ifndef __BSP_OLED_H
#define __BSP_OLED_H

#include "stm32f1xx_hal.h"

/******************************************************************************************/
/* OLED IIC引脚定义 */

#define OLED_SCL_PORT           GPIOB
#define OLED_SCL_PIN            GPIO_PIN_6
#define OLED_SCL_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define OLED_SDA_PORT           GPIOB
#define OLED_SDA_PIN            GPIO_PIN_7
#define OLED_SDA_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

/******************************************************************************************/
/* IIC控制宏 */

#define OLED_SCL(x)   do{ x ? \
                          HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PIN_RESET); \
                    }while(0)

#define OLED_SDA(x)   do{ x ? \
                          HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PIN_RESET); \
                    }while(0)

/* SSD1306 IIC地址 (7位地址0x3C, 左移1位=0x78) */
#define OLED_IIC_ADDR   0x78

/* 命令/数据标志 */
#define OLED_CMD      0
#define OLED_DATA     1

/******************************************************************************************/
/* 函数声明 */

void oled_init(void);
void oled_clear(void);
void oled_display_on(void);
void oled_display_off(void);
void oled_refresh_gram(void);
void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot);
void oled_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot);
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode);
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void oled_show_string(uint8_t x, uint8_t y, const char *p, uint8_t size);

#endif
