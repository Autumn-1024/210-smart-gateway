/**
 ****************************************************************************************************
 * @file        bsp_adc.h
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-30
 * @brief       ADC 光照传感器驱动 (PB1, ADC1_CH9)
 ****************************************************************************************************
 */

#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "stm32f1xx_hal.h"

#define LIGHT_ADC                   ADC1
#define LIGHT_ADC_CHANNEL           ADC_CHANNEL_9
#define LIGHT_ADC_CLK_ENABLE()      do{ __HAL_RCC_ADC1_CLK_ENABLE(); }while(0)

#define LIGHT_GPIO_PORT             GPIOB
#define LIGHT_GPIO_PIN              GPIO_PIN_1
#define LIGHT_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

void bsp_adc_init(void);
uint16_t bsp_adc_read(void);
uint16_t bsp_adc_read_avg(uint8_t times);

#endif
