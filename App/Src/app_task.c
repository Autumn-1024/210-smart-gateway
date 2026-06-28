/**
 ****************************************************************************************************
 * @file        app_task.c
 * @author      Autumn
 * @version     V2.0
 * @date        2026-06-16
 * @brief       应用层 - 菜单驱动主循环
 ****************************************************************************************************
 * @attention
 *
 * 按键映射 (菜单模式):
 *   KEY0(PA4)  -> 返回
 *   KEY1(PA5)  -> 下移 / 减少
 *   KEY2(PA6)  -> 上移 / 增加
 *   KEY_UP(PA7)-> 确认 / 执行
 *
 ****************************************************************************************************
 */

#include "app_task.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_oled.h"
#include "bsp_rs485.h"
#include "bsp_curtain.h"
#include "bsp_esp01s.h"
#include "bsp_uart3.h"
#include "app_menu.h"
#include "app_web.h"
#include <stdio.h>

/**
 * @brief       应用入口
 * @param       无
 * @retval      无
 */
void app_start(void)
{
    uint8_t key;
    uint8_t i;
    uint8_t rx_buf[16];
    uint8_t rx_len;
    uint8_t t = 0;

    /* 初始化菜单 */
    app_menu_init();

    /* 初始化 USART3 (中控触摸屏 RS485, PB10/PB11) */
    bsp_uart3_init(115200);

    while (1)
    {
        /* 扫描按键 */
        key = bsp_key_scan(0);

        /* 菜单处理 */
        app_menu_process(key);

        /* 接收RS485数据 */
        bsp_rs485_receive_data(rx_buf, &rx_len);

        if (rx_len)
        {
            if (rx_len > 16) rx_len = 16;

            printf("[RX] RS485 RECV: ");
            for (i = 0; i < rx_len; i++)
            {
                printf("%02X ", rx_buf[i]);
            }
            printf("\r\n");
        }

        /* Web服务器轮询 */
        app_web_process();

        /* 中控触摸屏 RS485 命令处理 */
        {
            uint8_t cmd_buf[16];
            uint16_t cmd_len = bsp_uart3_get_data(cmd_buf, sizeof(cmd_buf));
            if (cmd_len >= 5)
            {
                uint8_t dev_addr = cmd_buf[0];  /* 字节[0]=设备地址 */
                uint8_t ctrl     = cmd_buf[4];  /* 字节[4]=控制位 */
                uint8_t is_on    = (ctrl == 0xFF); /* 0xFF=开, 0x00=关 */
                uint16_t i;

                printf("[触摸屏] addr=0x%02X ctrl=0x%02X\r\n", dev_addr, ctrl);
                printf("[触摸屏] RAW: ");
                for (i = 0; i < cmd_len; i++) printf("%02X ", cmd_buf[i]);
                printf("\r\n");

                switch (dev_addr)
                {
                    /* 窗帘 0x01~0x04 (杜亚RS485协议) */
                    case 0x01: { static const uint8_t f[] = {0x55,0x02,0x01,0x03,0x01,0xB9,0x44};
                                static const uint8_t g[] = {0x55,0x02,0x01,0x03,0x02,0xF9,0x45};
                                bsp_rs485_send_data((uint8_t*)(is_on ? f : g), 7); } break;
                    case 0x02: { static const uint8_t f[] = {0x55,0x02,0x02,0x03,0x01,0x49,0x44};
                                static const uint8_t g[] = {0x55,0x02,0x02,0x03,0x02,0x09,0x45};
                                bsp_rs485_send_data((uint8_t*)(is_on ? f : g), 7); } break;
                    case 0x03: { static const uint8_t f[] = {0x55,0x02,0x03,0x03,0x01,0x18,0x84};
                                static const uint8_t g[] = {0x55,0x02,0x03,0x03,0x02,0x58,0x85};
                                bsp_rs485_send_data((uint8_t*)(is_on ? f : g), 7); } break;
                    case 0x04: { static const uint8_t f[] = {0x55,0x02,0x04,0x03,0x01,0xA9,0x45};
                                static const uint8_t g[] = {0x55,0x02,0x04,0x03,0x02,0xE9,0x44};
                                bsp_rs485_send_data((uint8_t*)(is_on ? f : g), 7); } break;

                    /* 空调1 */
                    case 0x05:
                        if (is_on) {
                            bsp_esp01s_http_post("10.0.50.110", 80, "{\"ir1\":2}");
                        } else {
                            bsp_esp01s_http_post("10.0.50.110", 80, "{\"ir2\":2}");
                        }
                        break;

                    /* 空调2 */
                    case 0x06:
                        if (is_on) {
                            bsp_esp01s_http_post("10.0.50.107", 80, "{\"ir1\":2}");
                        } else {
                            bsp_esp01s_http_post("10.0.50.107", 80, "{\"ir2\":2}");
                        }
                        break;

                    /* 灯1~6 */
                    case 0x07: bsp_esp01s_http_post("10.0.50.105", 80, is_on ? "{\"power1\":1}" : "{\"power1\":0}"); break;
                    case 0x08: bsp_esp01s_http_post("10.0.50.105", 80, is_on ? "{\"power2\":1}" : "{\"power2\":0}"); break;
                    case 0x09: bsp_esp01s_http_post("10.0.50.105", 80, is_on ? "{\"power3\":1}" : "{\"power3\":0}"); break;
                    case 0x0A: bsp_esp01s_http_post("10.0.50.106", 80, is_on ? "{\"power1\":1}" : "{\"power1\":0}"); break;
                    case 0x0B: bsp_esp01s_http_post("10.0.50.106", 80, is_on ? "{\"power2\":1}" : "{\"power2\":0}"); break;
                    case 0x0C: bsp_esp01s_http_post("10.0.50.106", 80, is_on ? "{\"power3\":1}" : "{\"power3\":0}"); break;

                    /* 全部灯 0x0D */
                    case 0x0D:
                        bsp_esp01s_http_post("10.0.50.105", 80, is_on ? "{\"power0\":1}" : "{\"power0\":0}");
                        bsp_esp01s_http_post("10.0.50.106", 80, is_on ? "{\"power0\":1}" : "{\"power0\":0}");
                        break;

                    default:
                        printf("[触摸屏] 未知地址: 0x%02X\r\n", dev_addr);
                        break;
                }
            }
        }

        t++;
        HAL_Delay(10);

        if (t == 50)
        {
            LED_TOGGLE();
            t = 0;
        }
    }
}
