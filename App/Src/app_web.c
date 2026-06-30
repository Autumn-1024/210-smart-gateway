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

#define IR2_MODULE_IP   "10.0.50.107"
#define IR2_MODULE_PORT 80

#define LIGHT1_IP       "10.0.50.104"
#define LIGHT2_IP       "10.0.50.106"
#define LIGHT_PORT      80

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
"<title>智能网关</title>"
"<style>"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{font-family:system-ui,-apple-system,sans-serif;background:linear-gradient(135deg,#0f0c29,#302b63,#24243e);min-height:100vh;padding:20px;color:#eee}"
"h1{text-align:center;font-size:24px;font-weight:700;margin-bottom:4px;background:linear-gradient(90deg,#a78bfa,#60a5fa,#34d399);-webkit-background-clip:text;-webkit-text-fill-color:transparent}"
".sub{text-align:center;font-size:12px;color:rgba(255,255,255,.3);margin-bottom:20px;letter-spacing:2px}"
".section{max-width:1000px;margin:0 auto 16px}"
".section h2{font-size:13px;color:rgba(255,255,255,.4);margin-bottom:8px;padding-left:4px;letter-spacing:1px}"
".g{display:grid;gap:10px}"
".gc{grid-template-columns:repeat(4,1fr)}"
".ga{grid-template-columns:repeat(2,1fr)}"
".gl{grid-template-columns:repeat(6,1fr)}"
".c{background:rgba(255,255,255,.05);backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,.07);border-radius:12px;padding:14px 10px}"
".c h3{font-size:11px;color:rgba(255,255,255,.45);text-align:center;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px}"
".b{display:block;width:100%;padding:10px 0;margin:3px 0;border:none;border-radius:8px;font-size:12px;font-weight:600;color:#fff;cursor:pointer;transition:transform .1s,box-shadow .2s}"
".b:active{transform:scale(.95)}"
".b.d{opacity:.4;pointer-events:none}"
".o{background:linear-gradient(135deg,#22c55e,#16a34a);box-shadow:0 2px 8px rgba(34,197,94,.2)}"
".o:hover{box-shadow:0 4px 14px rgba(34,197,94,.35)}"
".x{background:linear-gradient(135deg,#ef4444,#dc2626);box-shadow:0 2px 8px rgba(239,68,68,.2)}"
".x:hover{box-shadow:0 4px 14px rgba(239,68,68,.35)}"
".a{background:linear-gradient(135deg,#3b82f6,#2563eb);box-shadow:0 2px 8px rgba(59,130,246,.2)}"
".a:hover{box-shadow:0 4px 14px rgba(59,130,246,.35)}"
".row{display:flex;gap:4px;margin-top:6px}"
".row .b{flex:1}"
"#log{max-width:1000px;margin:16px auto 0;background:rgba(0,0,0,.35);border:1px solid rgba(255,255,255,.05);border-radius:10px;padding:10px 14px;max-height:140px;overflow-y:auto;font-family:'Courier New',monospace;font-size:11px;color:#4ade80;line-height:1.8}"
"#log .e{color:#f87171}"
"#log .t{color:rgba(255,255,255,.25)}"
"@media(min-width:768px){.gc{grid-template-columns:repeat(4,1fr)}.ga{grid-template-columns:repeat(2,1fr)}.gl{grid-template-columns:repeat(6,1fr)}}"
"@media(max-width:500px){.gc,.ga,.gl{grid-template-columns:repeat(2,1fr)}}"
"</style></head><body>"
"<h1>智能网关</h1>"
"<p class=\"sub\">窗帘 &middot; 空调 &middot; 灯光</p>"
"<div class=\"section\"><h2>窗帘控制</h2>"
"<div class=\"g gc\">"
"<div class=\"c\"><h3>窗帘 1</h3>"
"<button class=\"b o\" onclick=\"s(this,'/c1/open')\">OPEN</button>"
"<button class=\"b x\" onclick=\"s(this,'/c1/close')\">CLOSE</button></div>"
"<div class=\"c\"><h3>窗帘 2</h3>"
"<button class=\"b o\" onclick=\"s(this,'/c2/open')\">OPEN</button>"
"<button class=\"b x\" onclick=\"s(this,'/c2/close')\">CLOSE</button></div>"
"<div class=\"c\"><h3>窗帘 3</h3>"
"<button class=\"b o\" onclick=\"s(this,'/c3/open')\">OPEN</button>"
"<button class=\"b x\" onclick=\"s(this,'/c3/close')\">CLOSE</button></div>"
"<div class=\"c\"><h3>窗帘 4</h3>"
"<button class=\"b o\" onclick=\"s(this,'/c4/open')\">OPEN</button>"
"<button class=\"b x\" onclick=\"s(this,'/c4/close')\">CLOSE</button></div></div></div>"
"<div class=\"section\"><h2>空调控制</h2>"
"<div class=\"g ga\">"
"<div class=\"c\"><h3>空调 1</h3>"
"<div class=\"row\"><button class=\"b o\" onclick=\"s(this,'/ir/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/ir/2')\">OFF</button></div>"
"<button class=\"b a\" onclick=\"s(this,'/ir/3')\">AUTO</button></div>"
"<div class=\"c\"><h3>空调 2</h3>"
"<div class=\"row\"><button class=\"b o\" onclick=\"s(this,'/ir2/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/ir2/2')\">OFF</button></div>"
"<button class=\"b a\" onclick=\"s(this,'/ir2/3')\">AUTO</button></div></div></div>"
"<div class=\"section\"><h2>灯光控制</h2>"
"<div class=\"g gl\">"
"<div class=\"c\"><h3>L1</h3>"
"<div class=\"row\"><button class=\"b o\" onclick=\"s(this,'/l/1/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/l/1/0')\">OFF</button></div></div>"
"<div class=\"c\"><h3>L2</h3>"
"<div class=\"row\"><button class=\"b o\" onclick=\"s(this,'/l/2/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/l/2/0')\">OFF</button></div></div>"
"<div class=\"c\"><h3>L3</h3>"
"<div class=\"row\"><button class=\"b o\" onclick=\"s(this,'/l/3/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/l/3/0')\">OFF</button></div></div>"
"<div class=\"c\"><h3>L4</h3>"
"<div class=\"row\"><button class=\"b o\" onclick=\"s(this,'/l/4/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/l/4/0')\">OFF</button></div></div>"
"<div class=\"c\"><h3>L5</h3>"
"<div class=\"row\"><button class=\"b o\" onclick=\"s(this,'/l/5/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/l/5/0')\">OFF</button></div></div>"
"<div class=\"c\"><h3>L6</h3>"
"<div class=\"row\"><button class=\"b o\" onclick=\"s(this,'/l/6/1')\">ON</button>"
"<button class=\"b x\" onclick=\"s(this,'/l/6/0')\">OFF</button></div></div></div>"
"<div class=\"g\" style=\"max-width:1000px;margin:10px auto 0;grid-template-columns:1fr 1fr\">"
"<button class=\"b o\" onclick=\"s(this,'/l/0/1')\">全部开</button>"
"<button class=\"b x\" onclick=\"s(this,'/l/0/0')\">全部关</button></div></div>"
"<div id=\"log\"></div>"
"<script>"
"var lg=document.getElementById('log');"
"function a(m,c){var d=document.createElement('div');d.className=c||'';var t=new Date();d.innerHTML='<span class=t>'+('0'+t.getHours()).slice(-2)+':'+('0'+t.getMinutes()).slice(-2)+':'+('0'+t.getSeconds()).slice(-2)+'</span> '+m;lg.appendChild(d);lg.scrollTop=lg.scrollHeight;if(lg.children.length>50)lg.removeChild(lg.firstChild)}"
"function s(b,p){b.classList.add('d');a('>> '+p);fetch(p).then(function(r){return r.text()}).then(function(t){a('<< '+t);b.classList.remove('d')}).catch(function(e){a('ERR '+e,'e');b.classList.remove('d')})}"
"a('网关就绪','t')"
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

    /* IR2 control: /ir2/1 /ir2/2 /ir2/3 */
    if (path[0] == '/' && path[1] == 'i' && path[2] == 'r' && path[3] == '2' && path[4] == '/')
    {
        uint8_t ir_id = path[5] - '0';
        if (ir_id >= 1 && ir_id <= 3)
        {
            char json[24];
            snprintf(json, sizeof(json), "{\"ir%d\": 2}", ir_id);
            printf("[WEB] -> IR2-%d send\r\n", ir_id);

            if (bsp_esp01s_http_post(IR2_MODULE_IP, IR2_MODULE_PORT, json))
                snprintf(msg, sizeof(msg), "IR2-%d OK", ir_id);
            else
                snprintf(msg, sizeof(msg), "IR2-%d FAIL", ir_id);

            bsp_esp01s_send_response(link_id, http_200_text, msg);
            return;
        }
    }

    /* Light control: /l/{id}/{state} id:0=all,1-6=light state:1=on,0=off */
    if (path[0] == '/' && path[1] == 'l' && path[2] == '/')
    {
        uint8_t light_id = path[3] - '0';
        uint8_t state = path[5] - '0';
        char json[24];
        const char *ip;

        if (light_id == 0)
        {
            snprintf(json, sizeof(json), "{\"power0\": %d}", state);
            printf("[WEB] -> Light ALL %s\r\n", state ? "开" : "关");
            bsp_esp01s_http_post(LIGHT1_IP, LIGHT_PORT, json);
            if (bsp_esp01s_http_post(LIGHT2_IP, LIGHT_PORT, json))
                snprintf(msg, sizeof(msg), "ALL %s OK", state ? "开" : "关");
            else
                snprintf(msg, sizeof(msg), "ALL %s FAIL", state ? "开" : "关");
        }
        else if (light_id <= 6)
        {
            uint8_t pw = (light_id <= 3) ? light_id : light_id - 3;
            ip = (light_id <= 3) ? LIGHT1_IP : LIGHT2_IP;
            snprintf(json, sizeof(json), "{\"power%d\": %d}", pw, state);
            printf("[WEB] -> Light%d %s\r\n", light_id, state ? "开" : "关");

            if (bsp_esp01s_http_post(ip, LIGHT_PORT, json))
                snprintf(msg, sizeof(msg), "L%d %s OK", light_id, state ? "开" : "关");
            else
                snprintf(msg, sizeof(msg), "L%d %s FAIL", light_id, state ? "开" : "关");
        }
        else
        {
            bsp_esp01s_send_response(link_id, http_404, NULL);
            return;
        }

        bsp_esp01s_send_response(link_id, http_200_text, msg);
        return;
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
        oled_show_string(0, 0, "智能网关", 12);
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
