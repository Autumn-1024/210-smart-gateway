/**
 ****************************************************************************************************
 * @file        bsp_adc.c
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-30
 * @brief       ADC 光照传感器驱动 (PB1, ADC1_CH9)
 ****************************************************************************************************
 * @attention
 *
 * 关键: ADC预分频必须设为DIV6 (72MHz/6=12MHz), 否则ADC时钟超14MHz会读到0
 * 参考: https://controllerstech.com/stm32-adc-single-channel-polling/
 *
 ****************************************************************************************************
 */

#include "bsp_adc.h"

static ADC_HandleTypeDef s_adc_handle;

/**
 * @brief       ADC 初始化
 */
void bsp_adc_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    LIGHT_GPIO_CLK_ENABLE();
    LIGHT_ADC_CLK_ENABLE();

    /* ADC 预分频: PCLK2(72MHz) / 6 = 12MHz (必须 <= 14MHz) */
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);

    /* 配置 PB1 为模拟输入 */
    gpio_init.Pin   = LIGHT_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(LIGHT_GPIO_PORT, &gpio_init);

    /* 配置 ADC */
    s_adc_handle.Instance               = LIGHT_ADC;
    s_adc_handle.Init.ScanConvMode      = DISABLE;
    s_adc_handle.Init.ContinuousConvMode = DISABLE;
    s_adc_handle.Init.DiscontinuousConvMode = DISABLE;
    s_adc_handle.Init.ExternalTrigConv  = ADC_SOFTWARE_START;
    s_adc_handle.Init.DataAlign         = ADC_DATAALIGN_RIGHT;
    s_adc_handle.Init.NbrOfConversion   = 1;
    HAL_ADC_Init(&s_adc_handle);

    /* 校准 ADC */
    HAL_ADCEx_Calibration_Start(&s_adc_handle);
    HAL_Delay(1);
}

/**
 * @brief       单次读取 ADC 值
 * @retval      12位 ADC 值 (0~4095)
 */
uint16_t bsp_adc_read(void)
{
    ADC_ChannelConfTypeDef config = {0};

    config.Channel      = LIGHT_ADC_CHANNEL;
    config.Rank         = ADC_REGULAR_RANK_1;
    config.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&s_adc_handle, &config);

    HAL_ADC_Start(&s_adc_handle);
    HAL_ADC_PollForConversion(&s_adc_handle, 10);

    return (uint16_t)HAL_ADC_GetValue(&s_adc_handle);
}

/**
 * @brief       多次采样取平均
 * @param       times: 采样次数
 * @retval      平均 ADC 值
 */
uint16_t bsp_adc_read_avg(uint8_t times)
{
    uint32_t sum = 0;
    uint8_t i;

    for (i = 0; i < times; i++)
    {
        sum += bsp_adc_read();
    }

    return (uint16_t)(sum / times);
}
