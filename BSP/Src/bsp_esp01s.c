/**
 ****************************************************************************************************
 * @file        bsp_esp01s.c
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-17
 * @brief       ESP01S WiFi模块驱动 (USART1, PA9/PA10, AT指令)
 ****************************************************************************************************
 * @attention
 *
 * 接线: PA9(TX) -> ESP01S RX, PA10(RX) -> ESP01S TX
 * 波特�? 115200
 *
 ****************************************************************************************************
 */

#include "bsp_esp01s.h"
#include "bsp_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************************/
/* 私有变量 */

static UART_HandleTypeDef s_esp_uart;
static uint8_t s_rx_byte;                      /* 单字节接收缓�?*/
static uint8_t s_rx_buf[ESP01S_RX_BUF_SIZE];   /* 接收环形缓冲�?*/
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

static http_request_cb_t s_http_cb = NULL;

/******************************************************************************************/
/* 底层串口 */

/**
 * @brief       USART1初始�?(ESP01S专用)
 */
static void esp_uart_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* TX - PA9 复用推挽输出 */
    gpio_init_struct.Pin   = GPIO_PIN_9;
    gpio_init_struct.Mode  = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    /* RX - PA10 浮空输入 */
    gpio_init_struct.Pin  = GPIO_PIN_10;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    /* USART配置 */
    s_esp_uart.Instance          = ESP01S_USART;
    s_esp_uart.Init.BaudRate     = ESP01S_BAUDRATE;
    s_esp_uart.Init.WordLength   = UART_WORDLENGTH_8B;
    s_esp_uart.Init.StopBits     = UART_STOPBITS_1;
    s_esp_uart.Init.Parity       = UART_PARITY_NONE;
    s_esp_uart.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    s_esp_uart.Init.Mode         = UART_MODE_TX_RX;
    HAL_UART_Init(&s_esp_uart);

    /* 开启接收中�?*/
    HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_UART_Receive_IT(&s_esp_uart, &s_rx_byte, 1);
}

/**
 * @brief       USART1中断服务函数
 */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&s_esp_uart);
}

/**
 * @brief       UART接收完成回调
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == ESP01S_USART)
    {
        uint16_t next = (s_rx_head + 1) % ESP01S_RX_BUF_SIZE;
        if (next != s_rx_tail)
        {
            s_rx_buf[s_rx_head] = s_rx_byte;
            s_rx_head = next;
        }
        HAL_UART_Receive_IT(&s_esp_uart, &s_rx_byte, 1);
    }
}

/**
 * @brief       发送AT指令
 */
static void esp_send_cmd(const char *cmd)
{
    HAL_UART_Transmit(&s_esp_uart, (uint8_t *)cmd, strlen(cmd), 2000);
}

/**
 * @brief       发送原始数�? */
static void esp_send_data(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&s_esp_uart, (uint8_t *)data, len, 2000);
}

/**
 * @brief       读取缓冲区中的一个字�? * @retval      -1=无数�? >=0=字节�? */
static int16_t esp_read_byte(void)
{
    if (s_rx_head == s_rx_tail) return -1;
    uint8_t ch = s_rx_buf[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) % ESP01S_RX_BUF_SIZE;
    return ch;
}

/**
 * @brief       清空接收缓冲�? */
static void esp_flush_rx(void)
{
    s_rx_head = s_rx_tail = 0;
}

/**
 * @brief       等待指定字符�?(带超�?
 * @param       target: 目标字符�? * @param       timeout_ms: 超时时间
 * @retval      1=找到, 0=超时
 */
static uint8_t esp_wait_for(const char *target, uint16_t timeout_ms)
{
    uint16_t pos = 0;
    uint16_t elapsed = 0;
    uint16_t target_len = strlen(target);

    while (elapsed < timeout_ms)
    {
        int16_t ch = esp_read_byte();
        if (ch >= 0)
        {
            if (ch == target[pos])
            {
                pos++;
                if (pos >= target_len) return 1;
            }
            else
            {
                pos = (ch == target[0]) ? 1 : 0;
            }
        }
        else
        {
            HAL_Delay(1);
            elapsed++;
        }
    }
    return 0;
}

/**
 * @brief       等待指定字符串之一 (带超�?
 * @param       t1, t2: 目标字符�? * @param       timeout_ms: 超时时间
 * @retval      1=找到t1, 2=找到t2, 0=超时
 */
static uint8_t esp_wait_for_either(const char *t1, const char *t2, uint16_t timeout_ms)
{
    uint16_t p1 = 0, p2 = 0;
    uint16_t elapsed = 0;
    uint16_t l1 = strlen(t1), l2 = strlen(t2);

    while (elapsed < timeout_ms)
    {
        int16_t ch = esp_read_byte();
        if (ch >= 0)
        {
            if (ch == t1[p1]) { p1++; if (p1 >= l1) return 1; }
            else p1 = (ch == t1[0]) ? 1 : 0;

            if (ch == t2[p2]) { p2++; if (p2 >= l2) return 2; }
            else p2 = (ch == t2[0]) ? 1 : 0;
        }
        else
        {
            HAL_Delay(1);
            elapsed++;
        }
    }
    return 0;
}

/**
 * @brief       发送AT指令并等待OK
 * @param       cmd: AT指令
 * @param       timeout_ms: 超时时间
 * @retval      1=成功, 0=失败
 */
static uint8_t esp_send_at(const char *cmd, uint16_t timeout_ms)
{
    esp_flush_rx();
    esp_send_cmd(cmd);
    return esp_wait_for("OK", timeout_ms);
}

/******************************************************************************************/
/* 公开接口 */

/**
 * @brief       ESP01S初始�? */
void bsp_esp01s_init(void)
{
    esp_uart_init();
    HAL_Delay(2000);  /* 等待ESP01S启动 */

    /* 多次测试AT通信 */
    uint8_t i;
    for (i = 0; i < 5; i++)
    {
        if (esp_send_at("AT\r\n", 2000)) break;
        HAL_Delay(500);
    }
    HAL_Delay(200);

    /* 关闭回显 */
    esp_send_at("ATE0\r\n", 2000);
    HAL_Delay(200);

    /* 查询版本 (调试) */
    esp_send_at("AT+GMR\r\n", 3000);
    HAL_Delay(200);
}

/**
 * @brief       连接WiFi
 * @param       ssid: WiFi名称
 * @param       password: WiFi密码
 * @retval      1=成功, 0=失败
 */
uint8_t bsp_esp01s_connect_wifi(const char *ssid, const char *password)
{
    char cmd[128];

    /* 先设置多连接模式 (必须在连WiFi之前!) */
    esp_send_at("AT+CIPMUX=1\r\n", 3000);
    HAL_Delay(500);

    /* 设置Station模式 */
    esp_send_at("AT+CWMODE=1\r\n", 3000);
    HAL_Delay(500);

    /* 断开之前的连�?*/
    esp_send_at("AT+CWQAP\r\n", 3000);
    HAL_Delay(500);

    /* 连接WiFi */
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    esp_flush_rx();
    esp_send_cmd(cmd);

    /* 等待连接结果 (最�?0�? */
    uint8_t result = esp_wait_for_either("OK", "FAIL", 20000);
    if (result != 1)
        return 0;

    /* 等待获取IP */
    HAL_Delay(3000);

    return 1;
}

/**
 * @brief       启动TCP服务�? * @param       port: 端口�? * @retval      1=成功, 0=失败
 */
uint8_t bsp_esp01s_start_server(uint16_t port)
{
    char cmd[64];

    /* CIPMUX已在connect_wifi中设�?*/

    /* 启动TCP服务�?*/
    snprintf(cmd, sizeof(cmd), "AT+CIPSERVER=1,%d\r\n", port);
    esp_send_at(cmd, 5000);
    HAL_Delay(500);

    /* 设置服务器超�?*/
    esp_send_at("AT+CIPSTO=120\r\n", 3000);
    HAL_Delay(500);

    return 1;
}

/**
 * @brief       获取本机IP地址
 * @param       ip_buf: IP字符串缓冲区
 * @param       buf_size: 缓冲区大�? * @retval      1=成功, 0=失败
 */
uint8_t bsp_esp01s_get_ip(char *ip_buf, uint8_t buf_size)
{
    uint16_t pos = 0;
    uint16_t elapsed = 0;
    uint8_t found = 0;
    char line[64];

    esp_flush_rx();
    esp_send_cmd("AT+CIFSR\r\n");

    /* 等待响应, 解析 +CIFSR:STAIP,"xxx.xxx.xxx.xxx" */
    while (elapsed < 5000)
    {
        int16_t ch = esp_read_byte();
        if (ch >= 0)
        {
            if (ch == '\n')
            {
                line[pos] = '\0';
                if (strncmp(line, "+CIFSR:STAIP,\"", 14) == 0)
                {
                    char *start = line + 14;
                    char *end = strchr(start, '\"');
                    if (end && (end - start) < buf_size)
                    {
                        strncpy(ip_buf, start, end - start);
                        ip_buf[end - start] = '\0';
                        found = 1;
                    }
                }
                pos = 0;
            }
            else if (ch != '\r')
            {
                if (pos < sizeof(line) - 1)
                    line[pos++] = (char)ch;
            }
        }
        else
        {
            HAL_Delay(1);
            elapsed++;
        }
    }
    return found;
}

/**
 * @brief       设置HTTP回调
 */
void bsp_esp01s_set_http_callback(http_request_cb_t cb)
{
    s_http_cb = cb;
}

/**
 * @brief       发送HTTP响应
 * @param       link_id: 连接ID
 * @param       header: HTTP�? * @param       body: HTTP�?(可以为NULL)
 *
 * MEMORY.md: CIPSEND要等>提示�? 发完响应清缓冲区
 */
void bsp_esp01s_send_response(uint8_t link_id, const char *header, const char *body)
{
    char cmd[64];
    uint16_t header_len = strlen(header);
    uint16_t body_len = body ? strlen(body) : 0;
    uint16_t total_len = header_len + body_len;
    uint16_t sent = 0;
    uint16_t chunk;
    uint8_t send_ok;

    while (sent < total_len)
    {
        chunk = total_len - sent;
        if (chunk > 2048) chunk = 2048;

        snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d\r\n", link_id, chunk);
        esp_flush_rx();
        esp_send_cmd(cmd);

        if (!esp_wait_for(">", 2000))
        {
            esp_send_cmd("+\r\n");
            return;
        }

        if (sent < header_len)
        {
            uint16_t hdr_part = header_len - sent;
            if (hdr_part > chunk) hdr_part = chunk;
            esp_send_data((const uint8_t *)header + sent, hdr_part);
            if (hdr_part < chunk)
                esp_send_data((const uint8_t *)body, chunk - hdr_part);
        }
        else
        {
            esp_send_data((const uint8_t *)body + (sent - header_len), chunk);
        }

        sent += chunk;

        send_ok = esp_wait_for("SEND OK", 3000);
        if (!send_ok)
        {
            printf("[ESP] SEND FAIL %d/%d\r\n", sent, total_len);
            break;
        }
    }

    HAL_Delay(100);
    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d\r\n", link_id);
    esp_send_cmd(cmd);
    HAL_Delay(100);
    esp_flush_rx();
}

/**
 * @brief       HTTP client POST (send to other devices)
 * @param       host: target IP address
 * @param       port: target port
 * @param       json: JSON data string
 * @retval      1=success, 0=fail
 */
uint8_t bsp_esp01s_http_post(const char *host, uint16_t port, const char *json)
{
    char cmd[128];
    char http_req[256];
    uint16_t req_len;
    uint8_t result = 0;
    uint8_t cid = 4;  /* use link_id 4 for client connections */

    /* build HTTP POST request */
    req_len = snprintf(http_req, sizeof(http_req),
        "POST /v3 HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        host, (int)strlen(json), json);

    printf("[HTTP] POST %s:%d (%d bytes)\r\n", host, port, req_len);

    /* close any previous connection on this link_id */
    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d\r\n", cid);
    esp_flush_rx();
    esp_send_cmd(cmd);
    HAL_Delay(200);

    /* TCP connect with specific link_id */
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=%d,\"TCP\",\"%s\",%d\r\n", cid, host, port);
    esp_flush_rx();
    esp_send_cmd(cmd);

    if (!esp_wait_for("OK", 5000))
    {
        printf("[HTTP] TCP connect FAIL to %s:%d\r\n", host, port);
        printf("[HTTP] Check if device is online\r\n");
        return 0;
    }

    /* send AT+CIPSEND with link_id */
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d\r\n", cid, req_len);
    esp_flush_rx();
    esp_send_cmd(cmd);

    if (!esp_wait_for(">", 3000))
    {
        printf("[HTTP] CIPSEND timeout\r\n");
        esp_send_cmd("AT+CIPCLOSE\r\n");
        HAL_Delay(200);
        return 0;
    }

    /* send HTTP request */
    esp_send_data((const uint8_t *)http_req, req_len);

    if (esp_wait_for("SEND OK", 5000))
    {
        printf("[HTTP] Send OK\r\n");
        esp_wait_for("+IPD", 3000);
        result = 1;
    }
    else
    {
        printf("[HTTP] Send FAIL\r\n");
    }

    /* close connection */
    HAL_Delay(100);
    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d\r\n", cid);
    esp_send_cmd(cmd);
    HAL_Delay(200);
    esp_flush_rx();

    return result;
}

/**
 * @brief       轮询处理ESP01S数据 (主循环调�?
 *
 * ESP01S +IPD 格式: +IPD,link_id,data_len:data...
 * 冒号后面直接是HTTP数据, 没有换行�? *
 */
void bsp_esp01s_poll(void)
{
    static char data_buf[512];
    static uint16_t data_pos = 0;
    static uint8_t ipd_link = 0;
    static uint16_t ipd_len = 0;
    static uint16_t ipd_count = 0;
    

    /* +IPD, 前缀匹配 */
    static const char ipd_prefix[] = "+IPD,";
    static uint8_t prefix_pos = 0;
    static char tmp[8];
    static uint8_t tmp_pos = 0;
    static uint8_t parse_state = 0; /* 0=搜索前缀, 1=读link, 2=读len, 3=读data */

    int16_t ch;

    while ((ch = esp_read_byte()) >= 0)
    {
        switch (parse_state)
        {
        case 0: /* 搜索 +IPD, 前缀 */
            if (ch == ipd_prefix[prefix_pos])
            {
                prefix_pos++;
                if (prefix_pos >= 5) /* 找到 "+IPD," */
                {
                    parse_state = 1;
                    tmp_pos = 0;
                    prefix_pos = 0;
                }
            }
            else
            {
                prefix_pos = (ch == '+') ? 1 : 0;
            }
            break;

        case 1: /* 读取 link_id (直到 ',') */
            if (ch == ',')
            {
                tmp[tmp_pos] = '\0';
                ipd_link = atoi(tmp);
                parse_state = 2;
                tmp_pos = 0;
            }
            else if (tmp_pos < sizeof(tmp) - 1)
            {
                tmp[tmp_pos++] = (char)ch;
            }
            break;

        case 2: /* 读取 data_len (直到 ':') */
            if (ch == ':')
            {
                tmp[tmp_pos] = '\0';
                ipd_len = atoi(tmp);
                ipd_count = 0;
                data_pos = 0;
                parse_state = 3;
            }
            else if (tmp_pos < sizeof(tmp) - 1)
            {
                tmp[tmp_pos++] = (char)ch;
            }
            break;

        case 3: /* 读取 data (精确 ipd_len 字节) */
            if (data_pos < sizeof(data_buf) - 1)
            {
                data_buf[data_pos++] = (char)ch;
            }
            ipd_count++;

            if (ipd_count >= ipd_len)
            {
                data_buf[data_pos] = '\0';
                parse_state = 0;

                /* 解析HTTP请求 */
                if (s_http_cb)
                {
                    char method[8] = {0};
                    char path[64] = {0};
                    char *p;
                    char *sp;

                    if (strncmp(data_buf, "GET ", 4) == 0)
                    {
                        strcpy(method, "GET");
                        p = data_buf + 4;
                        sp = strchr(p, ' ');
                        if (sp && (sp - p) < (int)sizeof(path))
                        {
                            memcpy(path, p, sp - p);
                            path[sp - p] = '\0';
                        }
                    }
                    else if (strncmp(data_buf, "POST ", 5) == 0)
                    {
                        strcpy(method, "POST");
                        p = data_buf + 5;
                        sp = strchr(p, ' ');
                        if (sp && (sp - p) < (int)sizeof(path))
                        {
                            memcpy(path, p, sp - p);
                            path[sp - p] = '\0';
                        }
                    }

                    if (method[0] && path[0])
                    {
                        s_http_cb(ipd_link, method, path);
                    }
                }
            }
            break;

        default:
            parse_state = 0;
            prefix_pos = 0;
            break;
        }
    }
}
