/**
 ****************************************************************************************************
 * @file        bsp_key.c
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-16
 * @brief       按键驱动
 ****************************************************************************************************
 */

#include "bsp_key.h"

/**
 * @brief       按键初始化
 * @param       无
 * @retval      无
 */
void bsp_key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    KEY0_GPIO_CLK_ENABLE();
    KEY1_GPIO_CLK_ENABLE();
    KEY2_GPIO_CLK_ENABLE();
    KEY3_GPIO_CLK_ENABLE();

    /* KEY0/1/2/3: 上拉输入，低电平有效 */
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio_init_struct.Pin = KEY0_GPIO_PIN;
    HAL_GPIO_Init(KEY0_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = KEY1_GPIO_PIN;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = KEY2_GPIO_PIN;
    HAL_GPIO_Init(KEY2_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = KEY3_GPIO_PIN;
    HAL_GPIO_Init(KEY3_GPIO_PORT, &gpio_init_struct);
}

/**
 * @brief       按键扫描
 * @param       mode: 0=不支持连按, 1=支持连按
 * @retval      KEY0_PRES / KEY1_PRES / KEY2_PRES / WKUP_PRES / 0(无按键)
 */
uint8_t bsp_key_scan(uint8_t mode)
{
    static uint8_t key_up = 1;

    if (mode)
    {
        key_up = 1;
    }

    if (key_up && (KEY0 == 0 || KEY1 == 0 || KEY2 == 0 || KEY3 == 0))
    {
        HAL_Delay(10);
        key_up = 0;

        if (KEY0 == 0)       return KEY0_PRES;
        else if (KEY1 == 0)  return KEY1_PRES;
        else if (KEY2 == 0)  return KEY2_PRES;
        else if (KEY3 == 0)  return KEY3_PRES;
    }
    else if (KEY0 == 1 && KEY1 == 1 && KEY2 == 1 && KEY3 == 1)
    {
        key_up = 1;
    }

    return 0;
}
