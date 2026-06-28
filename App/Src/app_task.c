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
    extern volatile uint32_t g_uart3_isr_cnt;
    static uint32_t last_isr_cnt = 0;

    /* 初始化菜单 */
    app_menu_init();

    /* 初始化 USART3 (中控触摸屏 RS485, PB10/PB11) */
    bsp_uart3_init(115200);

    printf("\r\n========== Curtain Gateway Started ==========\r\n");
    printf("[SYS] USART1=Debug/ESP01S  USART2=RS485-Curtain  USART3=Touchpad\r\n\r\n");

    while (1)
    {
        /* 扫描按键 */
        key = bsp_key_scan(0);

        /* 菜单处理 */
        app_menu_process(key);

        /* ========== RS485 窗帘接收 ========== */
        bsp_rs485_receive_data(rx_buf, &rx_len);
        if (rx_len)
        {
            if (rx_len > 16) rx_len = 16;
            printf("[RS485 RX %d] ", rx_len);
            for (i = 0; i < rx_len; i++) printf("%02X ", rx_buf[i]);
            printf("\r\n");
        }

        /* Web服务器轮询 */
        app_web_process();

        /* ========== 触摸屏 UART3 接收 ========== */
        {
            uint8_t cmd_buf[16];
            uint16_t cmd_len = bsp_uart3_get_data(cmd_buf, sizeof(cmd_buf));
            if (cmd_len > 0)
            {
                printf("[UART3 RX %d] ", cmd_len);
                for (i = 0; i < cmd_len; i++) printf("%02X ", cmd_buf[i]);
                printf("\r\n");
            }
            if (cmd_len >= 5)
            {
                uint8_t dev_addr = cmd_buf[0];
                uint8_t ctrl     = cmd_buf[4];
                uint8_t is_on    = (ctrl == 0xFF);
                uint16_t i;

                printf("[Touchpad] addr=0x%02X ctrl=0x%02X %s\r\n",
                       dev_addr, ctrl, is_on ? "ON" : "OFF");

                switch (dev_addr)
                {
                    case 0x01: { static const uint8_t f[] = {0x55,0x02,0x01,0x03,0x01,0xB9,0x44};
                                static const uint8_t g[] = {0x55,0x02,0x01,0x03,0x02,0xF9,0x45};
                                uint8_t *p = (uint8_t*)(is_on ? f : g);
                                printf("[RS485 TX 7] "); for(i=0;i<7;i++) printf("%02X ",p[i]); printf(" (Curtain1 %s)\r\n", is_on?"ON":"OFF");
                                bsp_rs485_send_data(p, 7); } break;
                    case 0x02: { static const uint8_t f[] = {0x55,0x02,0x02,0x03,0x01,0x49,0x44};
                                static const uint8_t g[] = {0x55,0x02,0x02,0x03,0x02,0x09,0x45};
                                uint8_t *p = (uint8_t*)(is_on ? f : g);
                                printf("[RS485 TX 7] "); for(i=0;i<7;i++) printf("%02X ",p[i]); printf(" (Curtain2 %s)\r\n", is_on?"ON":"OFF");
                                bsp_rs485_send_data(p, 7); } break;
                    case 0x03: { static const uint8_t f[] = {0x55,0x02,0x03,0x03,0x01,0x18,0x84};
                                static const uint8_t g[] = {0x55,0x02,0x03,0x03,0x02,0x58,0x85};
                                uint8_t *p = (uint8_t*)(is_on ? f : g);
                                printf("[RS485 TX 7] "); for(i=0;i<7;i++) printf("%02X ",p[i]); printf(" (Curtain3 %s)\r\n", is_on?"ON":"OFF");
                                bsp_rs485_send_data(p, 7); } break;
                    case 0x04: { static const uint8_t f[] = {0x55,0x02,0x04,0x03,0x01,0xA9,0x45};
                                static const uint8_t g[] = {0x55,0x02,0x04,0x03,0x02,0xE9,0x44};
                                uint8_t *p = (uint8_t*)(is_on ? f : g);
                                printf("[RS485 TX 7] "); for(i=0;i<7;i++) printf("%02X ",p[i]); printf(" (Curtain4 %s)\r\n", is_on?"ON":"OFF");
                                bsp_rs485_send_data(p, 7); } break;

                    case 0x05: { uint8_t r = bsp_esp01s_http_post("10.0.50.110", 80, is_on ? "{\"ir1\":2}" : "{\"ir2\":2}");
                              printf("[AC1] %s %s\r\n", is_on?"ON":"OFF", r?"OK":"FAIL"); } break;

                    case 0x06: { uint8_t r = bsp_esp01s_http_post("10.0.50.107", 80, is_on ? "{\"ir1\":2}" : "{\"ir2\":2}");
                              printf("[AC2] %s %s\r\n", is_on?"ON":"OFF", r?"OK":"FAIL"); } break;

                    case 0x07: { uint8_t r = bsp_esp01s_http_post("10.0.50.105", 80, is_on ? "{\"power1\":1}" : "{\"power1\":0}");
                              printf("[Light1] %s %s\r\n", is_on?"ON":"OFF", r?"OK":"FAIL"); } break;
                    case 0x08: { uint8_t r = bsp_esp01s_http_post("10.0.50.105", 80, is_on ? "{\"power2\":1}" : "{\"power2\":0}");
                              printf("[Light2] %s %s\r\n", is_on?"ON":"OFF", r?"OK":"FAIL"); } break;
                    case 0x09: { uint8_t r = bsp_esp01s_http_post("10.0.50.105", 80, is_on ? "{\"power3\":1}" : "{\"power3\":0}");
                              printf("[Light3] %s %s\r\n", is_on?"ON":"OFF", r?"OK":"FAIL"); } break;
                    case 0x0A: { uint8_t r = bsp_esp01s_http_post("10.0.50.106", 80, is_on ? "{\"power1\":1}" : "{\"power1\":0}");
                              printf("[Light4] %s %s\r\n", is_on?"ON":"OFF", r?"OK":"FAIL"); } break;
                    case 0x0B: { uint8_t r = bsp_esp01s_http_post("10.0.50.106", 80, is_on ? "{\"power2\":1}" : "{\"power2\":0}");
                              printf("[Light5] %s %s\r\n", is_on?"ON":"OFF", r?"OK":"FAIL"); } break;
                    case 0x0C: { uint8_t r = bsp_esp01s_http_post("10.0.50.106", 80, is_on ? "{\"power3\":1}" : "{\"power3\":0}");
                              printf("[Light6] %s %s\r\n", is_on?"ON":"OFF", r?"OK":"FAIL"); } break;

                    case 0x0D:
                        { uint8_t r1 = bsp_esp01s_http_post("10.0.50.105", 80, is_on ? "{\"power0\":1}" : "{\"power0\":0}");
                          uint8_t r2 = bsp_esp01s_http_post("10.0.50.106", 80, is_on ? "{\"power0\":1}" : "{\"power0\":0}");
                          printf("[AllLights] %s 105:%s 106:%s\r\n", is_on?"ON":"OFF", r1?"OK":"FAIL", r2?"OK":"FAIL"); } break;

                    default:
                        printf("[Touchpad] Unknown addr: 0x%02X\r\n", dev_addr);
                        break;
                }
            }
        }

        /* ========== UART3 ISR activity check ========== */
        if (g_uart3_isr_cnt != last_isr_cnt)
        {
            printf("[UART3] ISR count: %lu\r\n", g_uart3_isr_cnt);
            last_isr_cnt = g_uart3_isr_cnt;
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
