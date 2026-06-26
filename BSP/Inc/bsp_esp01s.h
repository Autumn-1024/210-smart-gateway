/**
 ****************************************************************************************************
 * @file        bsp_esp01s.h
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-17
 * @brief       ESP01S WiFi模块驱动 (USART1, PA9/PA10, AT指令)
 ****************************************************************************************************
 */

#ifndef __BSP_ESP01S_H
#define __BSP_ESP01S_H

#include "stm32f1xx_hal.h"

/******************************************************************************************/
/* 配置 */

#define ESP01S_USART            USART1
#define ESP01S_BAUDRATE         115200
#define ESP01S_RX_BUF_SIZE      512
#define ESP01S_TX_BUF_SIZE      256

/******************************************************************************************/
/* 函数声明 */

void bsp_esp01s_init(void);
uint8_t bsp_esp01s_connect_wifi(const char *ssid, const char *password);
uint8_t bsp_esp01s_start_server(uint16_t port);
void bsp_esp01s_poll(void);

/******************************************************************************************/
/* HTTP回调 (由app_web实现) */

typedef void (*http_request_cb_t)(uint8_t link_id, const char *method, const char *path);
void bsp_esp01s_set_http_callback(http_request_cb_t cb);

/******************************************************************************************/
/* 获取IP地址 */

uint8_t bsp_esp01s_get_ip(char *ip_buf, uint8_t buf_size);

/******************************************************************************************/
/* HTTP响应发送 */

void bsp_esp01s_send_response(uint8_t link_id, const char *header, const char *body);

#endif
