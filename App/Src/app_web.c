/**
 ****************************************************************************************************
 * @file        app_web.c
 * @author      Autumn
 * @version     V1.0
 * @date        2026-06-17
 * @brief       Web控制页面 + HTTP请求处理
 ****************************************************************************************************
 * @attention
 *
 * 控制4个窗帘: 0x0201, 0x0202, 0x0203, 0x0204
 * API: /c1/open, /c1/close, /c2/open, /c2/close, ...
 *
 ****************************************************************************************************
 */

#include "app_web.h"
#include "bsp_esp01s.h"
#include "bsp_rs485.h"
#include "bsp_oled.h"
#include <stdio.h>
#include <string.h>

/******************************************************************************************/
/* WiFi配置 */

#define WIFI_SSID       "210"
#define WIFI_PASS       "12345678"
#define WEB_PORT        80

#define IR_MODULE_IP    "10.0.50.110"
#define IR_MODULE_PORT  80

/******************************************************************************************/
/* 预计算窗帘控制帧 (地址+开/关) */

static const uint8_t curtain_frames[][7] = {
    /* C1 0x0201 */ {0x55,0x02,0x01,0x03,0x01,0xB9,0x44}, /* open  */
    /* C1 0x0201 */ {0x55,0x02,0x01,0x03,0x02,0xF9,0x45}, /* close */
    /* C2 0x0202 */ {0x55,0x02,0x02,0x03,0x01,0x49,0x44}, /* open  */
    /* C2 0x0202 */ {0x55,0x02,0x02,0x03,0x02,0x09,0x45}, /* close */
    /* C3 0x0203 */ {0x55,0x02,0x03,0x03,0x01,0x18,0x84}, /* open  */
    /* C3 0x0203 */ {0x55,0x02,0x03,0x03,0x02,0x58,0x85}, /* close */
    /* C4 0x0204 */ {0x55,0x02,0x04,0x03,0x01,0xA9,0x45}, /* open  */
    /* C4 0x0204 */ {0x55,0x02,0x04,0x03,0x02,0xE9,0x44}, /* close */
};
#define FRAME_OPEN(c)   (curtain_frames[(c)*2])
#define FRAME_CLOSE(c)  (curtain_frames[(c)*2+1])

/******************************************************************************************/
/* HTML页面 (存储在Flash中) */

static const char html_page[] =
"<!DOCTYPE html><html><head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Smart Gateway</title>"
"<style>"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{font-family:system-ui,sans-serif;background:#f0f2f5;padding:12px}"
"h1{text-align:center;color:#333;font-size:20px;margin-bottom:12px}"
".g{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:10px;max-width:800px;margin:0 auto}"
".c{background:#fff;border-radius:10px;padding:12px;box-shadow:0 1px 4px rgba(0,0,0,.08)}"
".c h3{font-size:12px;color:#888;text-align:center;margin-bottom:8px}"
".b{display:block;width:100%;padding:10px 0;margin:3px 0;border:none;border-radius:6px;font-size:13px;font-weight:600;color:#fff;cursor:pointer;transition:opacity .15s}"
".b:active{opacity:.6}"
".b.d{opacity:.5;pointer-events:none}"
".o{background:#4CAF50}"
".x{background:#f44336}"
".a{background:#2196F3}"
"#log{max-width:800px;margin:12px auto 0;background:#1a1a2e;border-radius:8px;padding:10px;max-height:180px;overflow-y:auto;font-family:monospace;font-size:11px;color:#0f0;line-height:1.6}"
"#log .e{color:#f55}"
"#log .t{color:#888}"
"</style></head><body>"
"<h1>Smart Gateway</h1>"
"<div class=\"g\">"
"<div class=\"c\"><h3>Curtain 1</h3>"
"<button class=\"b o\" onclick=\"s(this,'/c1/open')\">OPEN</button>"
"<button class=\"b x\" onclick=\"s(this,'/c1/close')\">CLOSE</button></div>"
"<div class=\"c\"><h3>Curtain 2</h3>"
"<button class=\"b o\" onclick=\"s(this,'/c2/open')\">OPEN</button>"
"<button class=\"b x\" onclick=\"s(this,'/c2/close')\">CLOSE</button></div>"
"<div class=\"c\"><h3>Curtain 3</h3>"
"<button class=\"b o\" onclick=\"s(this,'/c3/open')\">OPEN</button>"
"<button class=\"b x\" onclick=\"s(this,'/c3/close')\">CLOSE</button></div>"
"<div class=\"c\"><h3>Curtain 4</h3>"
"<button class=\"b o\" onclick=\"s(this,'/c4/open')\">OPEN</button>"
"<button class=\"b x\" onclick=\"s(this,'/c4/close')\">CLOSE</button></div>"
"<div class=\"c\"><h3>AC Control</h3>"
"<button class=\"b o\" onclick=\"s(this,'/ir/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/ir/2')\">OFF</button>"
"<button class=\"b a\" onclick=\"s(this,'/ir/3')\">AUTO</button></div>"
"</div>"
"<div id=\"log\"></div>"
"<script>"
"var lg=document.getElementById('log');"
"function a(m,c){var d=document.createElement('div');d.className=c||'';var t=new Date();d.innerHTML='<span class=t>'+('0'+t.getHours()).slice(-2)+':'+('0'+t.getMinutes()).slice(-2)+':'+('0'+t.getSeconds()).slice(-2)+'</span> '+m;lg.appendChild(d);lg.scrollTop=lg.scrollHeight;if(lg.children.length>50)lg.removeChild(lg.firstChild)}"
"function s(b,p){b.classList.add('d');a('>> '+p);fetch(p).then(function(r){return r.text()}).then(function(t){a('<< '+t);b.classList.remove('d')}).catch(function(e){a('ERR '+e,'e');b.classList.remove('d')})}"
"a('Ready','t')"
"</script></body></html>";

static const char http_200_header[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n\r\n";

static const char http_200_text[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/plain\r\n"
"Connection: close\r\n\r\n";

static const char http_404[] =
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/plain\r\n"
"Connection: close\r\n\r\nNot Found";

/******************************************************************************************/
/* 状态 */

static uint8_t s_web_initialized = 0;
static uint8_t s_wifi_connected = 0;
static uint8_t s_hold_display = 0;  /* 1=保持当前OLED显示，不被菜单覆盖 */

/******************************************************************************************/
/* HTTP请求处理回调 */

static void on_http_request(uint8_t link_id, const char *method, const char *path)
{
    char msg[32];

    printf("[WEB] %d %s %s\r\n", link_id, method, path);

    /* 窗帘控制: /c1/open /c1/close /c2/open ... */
    if (path[0] == '/' && path[1] == 'c' && path[3] == '/')
    {
        uint8_t idx = path[2] - '1';  /* 0~3 */
        const char *action = path + 4;

        if (idx < 4)
        {
            const uint8_t *frame = NULL;

            if (action[0] == 'o' && action[1] == 'p')  /* open */
            {
                frame = FRAME_OPEN(idx);
                snprintf(msg, sizeof(msg), "C%d OPEN OK", idx + 1);
            }
            else if (action[0] == 'c')  /* close */
            {
                frame = FRAME_CLOSE(idx);
                snprintf(msg, sizeof(msg), "C%d CLOSE OK", idx + 1);
            }

            if (frame)
            {
                printf("[WEB] -> C%d %s\r\n", idx + 1, action);
                bsp_rs485_send_data((uint8_t *)frame, 7);
                bsp_esp01s_send_response(link_id, http_200_text, msg);
                return;
            }
        }
    }

    /* IR control: /ir/1 /ir/2 /ir/3 */
    if (path[0] == '/' && path[1] == 'i' && path[2] == 'r' && path[3] == '/')
    {
        uint8_t ir_id = path[4] - '0';
        if (ir_id >= 1 && ir_id <= 3)
        {
            char json[24];
            snprintf(json, sizeof(json), "{\"ir%d\": 2}", ir_id);
            printf("[WEB] -> IR%d send\r\n", ir_id);

            if (bsp_esp01s_http_post(IR_MODULE_IP, IR_MODULE_PORT, json))
                snprintf(msg, sizeof(msg), "IR%d OK", ir_id);
            else
                snprintf(msg, sizeof(msg), "IR%d FAIL", ir_id);

            bsp_esp01s_send_response(link_id, http_200_text, msg);
            return;
        }
    }

    /* 首页 */

    if (path[0] == '/' && path[1] == '\0')
    {
        bsp_esp01s_send_response(link_id, http_200_header, html_page);
        return;
    }

    /* 404 */
    bsp_esp01s_send_response(link_id, http_404, NULL);
}

/******************************************************************************************/
/* 公开接口 */

/**
 * @brief       Web控制初始化
 */
void app_web_init(void)
{
    uint8_t i;

    /* 在OLED上显示初始化状态 */
    oled_clear();
    oled_show_string(0, 0, "WiFi Init...", 12);
    oled_refresh_gram();

    printf("[WEB] Initializing ESP01S...\r\n");
    bsp_esp01s_init();

    /* 连接WiFi */
    printf("[WEB] Connecting to %s...\r\n", WIFI_SSID);
    oled_clear();
    oled_show_string(0, 0, "Connecting WiFi", 12);
    oled_show_string(0, 14, WIFI_SSID, 12);
    oled_refresh_gram();

    for (i = 0; i < 3; i++)
    {
        if (bsp_esp01s_connect_wifi(WIFI_SSID, WIFI_PASS))
        {
            s_wifi_connected = 1;
            printf("[WEB] WiFi connected!\r\n");
            break;
        }
        printf("[WEB] WiFi retry %d/3...\r\n", i + 1);
    }

    if (!s_wifi_connected)
    {
        printf("[WEB] WiFi failed!\r\n");
        oled_clear();
        oled_show_string(0, 24, "WiFi Failed!", 12);
        oled_refresh_gram();
        return;
    }

    /* 获取IP地址 */
    char ip_addr[20] = {0};
    bsp_esp01s_get_ip(ip_addr, sizeof(ip_addr));
    printf("[WEB] IP: %s\r\n", ip_addr);

    /* 启动Web服务器 */
    oled_clear();
    oled_show_string(0, 0, "Starting server", 12);
    oled_refresh_gram();

    if (bsp_esp01s_start_server(WEB_PORT))
    {
        s_web_initialized = 1;

        /* 注册HTTP回调 */
        bsp_esp01s_set_http_callback(on_http_request);

        /* 显示智能网关 + IP */
        oled_clear();
        oled_show_string(0, 0, "Smart Gateway", 12);
        oled_show_string(0, 14, ip_addr, 12);
        oled_refresh_gram();

        s_hold_display = 1;  /* 保持显示，不被菜单覆盖 */
    }
    else
    {
        printf("[WEB] Server failed!\r\n");
        oled_clear();
        oled_show_string(0, 24, "Server Failed!", 12);
        oled_refresh_gram();
    }
}

/**
 * @brief       Web服务轮询 (主循环调用)
 */
void app_web_process(void)
{
    if (s_web_initialized)
    {
        bsp_esp01s_poll();
    }
}

/**
 * @brief       检查并清除OLED保持标志
 * @retval      1=需要保持显示(调用后自动清除), 0=正常
 */
uint8_t app_web_check_hold(void)
{
    if (s_hold_display)
    {
        s_hold_display = 0;
        return 1;
    }
    return 0;
}
