/**
 ****************************************************************************************************
 * @file        bsp_oled.c
 * @author      Autumn
 * @version     V2.0
 * @date        2026-06-16
 * @brief       OLED SSD1306 驱动 (软件IIC模式)
 ****************************************************************************************************
 * @attention
 *
 * 平台: STM32F103C8T6
 * 接线: PB6(SCL), PB7(SDA)
 * SSD1306 IIC地址: 0x78
 *
 ****************************************************************************************************
 */

#include "bsp_oled.h"
#include "bsp_oledfont.h"
#include <stdio.h>

/* OLED显存, 每字节存8个像素, 128列 x 8页 = 128x64 */
static uint8_t g_oled_gram[128][8];

/******************************************************************************************/
/* 软件IIC底层 */

/**
 * @brief       IIC延时 (约4us, ~100kHz)
 * @param       无
 * @retval      无
 */
static void oled_iic_delay(void)
{
    uint8_t i = 10;
    while (i--);
}

/**
 * @brief       IIC起始信号
 * @param       无
 * @retval      无
 */
static void oled_iic_start(void)
{
    OLED_SDA(1);
    OLED_SCL(1);
    oled_iic_delay();
    OLED_SDA(0);
    oled_iic_delay();
    OLED_SCL(0);
    oled_iic_delay();
}

/**
 * @brief       IIC停止信号
 * @param       无
 * @retval      无
 */
static void oled_iic_stop(void)
{
    OLED_SDA(0);
    OLED_SCL(1);
    oled_iic_delay();
    OLED_SDA(1);
    oled_iic_delay();
}

/**
 * @brief       IIC等待ACK (盲ACK, 与参考驱动一致)
 * @param       无
 * @retval      0
 */
static uint8_t oled_iic_wait_ack(void)
{
    OLED_SCL(1);
    oled_iic_delay();
    OLED_SCL(0);
    oled_iic_delay();
    return 0;
}

/**
 * @brief       IIC发送一个字节
 * @param       data: 要发送的字节
 * @retval      无
 */
static void oled_iic_write_byte(uint8_t data)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        OLED_SDA((data & 0x80) >> 7);
        data <<= 1;
        oled_iic_delay();
        OLED_SCL(1);
        oled_iic_delay();
        OLED_SCL(0);
        oled_iic_delay();
    }

    oled_iic_wait_ack();
}

/**
 * @brief       向OLED写一个字节 (带控制字节)
 * @param       data: 数据
 * @param       cmd : 0=命令, 1=数据
 * @retval      无
 */
static void oled_wr_byte(uint8_t data, uint8_t cmd)
{
    oled_iic_start();
    oled_iic_write_byte(OLED_IIC_ADDR);    /* 发送设备地址 */
    oled_iic_write_byte(cmd ? 0x40 : 0x00); /* 控制字节: 0x00=命令, 0x40=数据 */
    oled_iic_write_byte(data);
    oled_iic_stop();
}

/******************************************************************************************/
/* OLED功能函数 */

/**
 * @brief       刷新显存到OLED
 * @param       无
 * @retval      无
 */
void oled_refresh_gram(void)
{
    uint8_t i, n;

    for (i = 0; i < 8; i++)
    {
        oled_wr_byte(0xb0 + i, OLED_CMD);
        oled_wr_byte(0x02, OLED_CMD);   /* 列低地址 +2 偏移 */
        oled_wr_byte(0x10, OLED_CMD);

        for (n = 0; n < 128; n++)
        {
            oled_wr_byte(g_oled_gram[n][i], OLED_DATA);
        }
    }
}

/**
 * @brief       开启OLED显示
 * @param       无
 * @retval      无
 */
void oled_display_on(void)
{
    oled_wr_byte(0X8D, OLED_CMD);
    oled_wr_byte(0X14, OLED_CMD);
    oled_wr_byte(0XAF, OLED_CMD);
}

/**
 * @brief       关闭OLED显示
 * @param       无
 * @retval      无
 */
void oled_display_off(void)
{
    oled_wr_byte(0X8D, OLED_CMD);
    oled_wr_byte(0X10, OLED_CMD);
    oled_wr_byte(0XAE, OLED_CMD);
}

/**
 * @brief       清屏
 * @param       无
 * @retval      无
 */
void oled_clear(void)
{
    uint8_t i, n;

    for (i = 0; i < 8; i++)
    {
        for (n = 0; n < 128; n++)
        {
            g_oled_gram[n][i] = 0X00;
        }
    }

    oled_refresh_gram();
}

/**
 * @brief       画点
 * @param       x   : 0~127
 * @param       y   : 0~63
 * @param       dot : 1=亮, 0=灭
 * @retval      无
 */
void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot)
{
    uint8_t pos, bx, temp = 0;

    if (x > 127 || y > 63) return;

    pos = y / 8;
    bx = y % 8;
    temp = 1 << bx;

    if (dot)
        g_oled_gram[x][pos] |= temp;
    else
        g_oled_gram[x][pos] &= ~temp;
}

/**
 * @brief       区域填充
 * @param       x1,y1 : 起始坐标
 * @param       x2,y2 : 结束坐标
 * @param       dot   : 1=亮, 0=灭
 * @retval      无
 */
void oled_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot)
{
    uint8_t x, y;

    for (x = x1; x <= x2; x++)
    {
        for (y = y1; y <= y2; y++)
        {
            oled_draw_point(x, y, dot);
        }
    }

    oled_refresh_gram();
}

/**
 * @brief       显示一个字符
 * @param       x,y  : 坐标
 * @param       chr  : 字符
 * @param       size : 字体大小 12/16/24
 * @param       mode : 0=覆盖, 1=叠加
 * @retval      无
 */
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode)
{
    uint8_t temp, t, t1;
    uint8_t y0 = y;
    uint8_t *pfont = 0;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);

    chr = chr - ' ';

    if (size == 12)
        pfont = (uint8_t *)oled_asc2_1206[chr];
    else if (size == 16)
        pfont = (uint8_t *)oled_asc2_1608[chr];
    else if (size == 24)
        pfont = (uint8_t *)oled_asc2_2412[chr];
    else
        return;

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];

        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
                oled_draw_point(x, y, mode);
            else
                oled_draw_point(x, y, !mode);

            temp <<= 1;
            y++;

            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**
 * @brief       幂函数
 */
static uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n--)
        result *= m;

    return result;
}

/**
 * @brief       显示数字
 * @param       x,y : 坐标
 * @param       num : 数值
 * @param       len : 位数
 * @param       size: 字体大小 12/16/24
 * @retval      无
 */
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;

        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                oled_show_char(x + (size / 2) * t, y, ' ', size, 1);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }

        oled_show_char(x + (size / 2) * t, y, temp + '0', size, 1);
    }
}

/**
 * @brief       显示字符串
 * @param       x,y : 坐标
 * @param       *p  : 字符串
 * @param       size: 字体大小 12/16/24
 * @retval      无
 */
void oled_show_string(uint8_t x, uint8_t y, const char *p, uint8_t size)
{
    while ((*p <= '~') && (*p >= ' '))
    {
        if (x > (128 - (size / 2)))
        {
            x = 0;
            y += size;
        }

        if (y > (64 - size))
        {
            y = x = 0;
            oled_clear();
        }

        oled_show_char(x, y, *p, size, 1);
        x += size / 2;
        p++;
    }
}

/**
 * @brief       OLED初始化 (SSD1306, 软件IIC)
 * @param       无
 * @retval      无
 */
void oled_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    /* 使能时钟 */
    OLED_SCL_CLK_ENABLE();
    OLED_SDA_CLK_ENABLE();

    /* 配置GPIO为推挽输出 (与SSD1306参考驱动一致) */
    gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull  = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio_init_struct.Pin = OLED_SCL_PIN;
    HAL_GPIO_Init(OLED_SCL_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = OLED_SDA_PIN;
    HAL_GPIO_Init(OLED_SDA_PORT, &gpio_init_struct);

    OLED_SCL(1);
    OLED_SDA(1);

    HAL_Delay(200);

    /* SSD1306 初始化序列 */
    oled_wr_byte(0xAE, OLED_CMD);   /* 关闭显示 */
    oled_wr_byte(0xD5, OLED_CMD);   /* 设置时钟分频 */
    oled_wr_byte(80, OLED_CMD);
    oled_wr_byte(0xA8, OLED_CMD);   /* 设置多路复用率 */
    oled_wr_byte(0X3F, OLED_CMD);   /* 1/64 */
    oled_wr_byte(0xD3, OLED_CMD);   /* 设置显示偏移 */
    oled_wr_byte(0X00, OLED_CMD);
    oled_wr_byte(0x40, OLED_CMD);   /* 设置起始行 */
    oled_wr_byte(0x8D, OLED_CMD);   /* 电荷泵 */
    oled_wr_byte(0x14, OLED_CMD);   /* 使能 */
    oled_wr_byte(0x20, OLED_CMD);   /* 寻址模式 */
    oled_wr_byte(0x02, OLED_CMD);   /* 页寻址 */
    oled_wr_byte(0xA1, OLED_CMD);   /* 段重映射 */
    oled_wr_byte(0xC8, OLED_CMD);   /* COM扫描方向 */
    oled_wr_byte(0xDA, OLED_CMD);   /* COM硬件配置 */
    oled_wr_byte(0x12, OLED_CMD);
    oled_wr_byte(0x81, OLED_CMD);   /* 对比度 */
    oled_wr_byte(0xEF, OLED_CMD);
    oled_wr_byte(0xD9, OLED_CMD);   /* 预充电周期 */
    oled_wr_byte(0xf1, OLED_CMD);
    oled_wr_byte(0xDB, OLED_CMD);   /* VCOMH电压 */
    oled_wr_byte(0x30, OLED_CMD);
    oled_wr_byte(0xA4, OLED_CMD);   /* 全局显示 */
    oled_wr_byte(0xA6, OLED_CMD);   /* 正常显示 */
    oled_wr_byte(0xAF, OLED_CMD);   /* 开启显示 */

    oled_clear();

    printf("[OLED] Init done (PB6=SCL, PB7=SDA, Addr=0x78)\r\n");
}
