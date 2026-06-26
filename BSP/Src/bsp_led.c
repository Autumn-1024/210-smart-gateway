/**
 ****************************************************************************************************
 * @file        bsp_led.c
 * @author      STM32F103 LED Driver
 * @version     V1.0
 * @date        2024
 * @brief       LED驱动源文件
 ****************************************************************************************************
 * @attention
 *
 * 平台: STM32F103C8T6
 *
 ****************************************************************************************************
 */

#include "bsp_led.h"

/**
 * @brief       初始化LED
 * @param       无
 * @retval      无
 * @note        配置LED引脚为推挽输出模式
 */
void bsp_led_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    /* 使能GPIOC时钟 */
    LED_GPIO_CLK_ENABLE();

    /* 配置LED引脚 */
    gpio_init_struct.Pin = LED_GPIO_PIN;                    /* LED引脚 */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* 推挽输出 */
    gpio_init_struct.Pull = GPIO_NOPULL;                     /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;           /* 低速 */
    HAL_GPIO_Init(LED_GPIO_PORT, &gpio_init_struct);        /* 初始化LED引脚 */

    /* 关闭LED */
    LED(1);
}
