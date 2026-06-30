/**
 ****************************************************************************************************
 * @file        main.c
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-16
 * @brief       主程序 - 只做初始化
 ****************************************************************************************************
 * @attention
 *
 * 平台: STM32F103C8T6
 * 功能: OLED + 4按键 + RS485 窗帘控制
 * 参考: D:\MiCloud\项目\智能窗帘控制\F429系列RS485
 *
 ****************************************************************************************************
 */

#include "stm32f1xx_hal.h"
#include "bsp_uart.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_oled.h"
#include "bsp_rs485.h"
#include "bsp_curtain.h"
#include "bsp_esp01s.h"
#include "bsp_adc.h"
#include "app_task.h"
#include "app_web.h"

/**
 * @brief       系统时钟配置 (HSE 8MHz -> 72MHz)
 * @param       无
 * @retval      无
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        while(1);
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        while(1);
    }
}

/**
 * @brief       主函数 - 只做初始化
 * @param       无
 * @retval      无
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    bsp_uart_init(115200);
    bsp_led_init();
    bsp_key_init();
    oled_init();
    bsp_rs485_init(9600);
    bsp_curtain_init();
    bsp_adc_init();

    app_web_init();

    app_start();    /* 交给应用层 */

    while(1) {}     /* 不会到这里 */
}
