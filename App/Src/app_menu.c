/**
 ****************************************************************************************************
 * @file        app_menu.c
 * @author      Autumn
 * @version     V2.0
 * @date        2026-06-17
 * @brief       OLED菜单系统
 ****************************************************************************************************
 * @attention
 *
 * 0.96寸 OLED 128x64, 12号字体 (每行21字符, 共5行)
 * 按键: KEY0=返回, KEY1=上移, KEY2=下移, KEY3=确认
 *
 ****************************************************************************************************
 */

#include "app_menu.h"
#include "bsp_key.h"
#include "bsp_oled.h"
#include "bsp_curtain.h"
#include <stdio.h>

/******************************************************************************************/
/* 菜单文本 */

static const char *main_menu_items[MAIN_MAX] = {
    "1.Set Address",
    "2.Control Test",
};

static const char *addr_menu_items[ADDR_MAX] = {
    "1.Write Address",
    "2.Slave Request",
};

static const char *ctrl_menu_items[CTRL_MAX] = {
    "1.Open",
    "2.Close",
    "3.Stop",
    "4.Percent",
    "5.Query Pos",
};

static const char *ctrl_addr_items[CTRL_ADDR_MAX] = {
    "1.Default(FEFE)",
    "2.Custom Addr",
};

/* 十六进制字符表 (用于地址输入) */
static const char hex_chars[] = "0123456789ABCDEF";
#define HEX_CHAR_COUNT  16

/******************************************************************************************/
/* 状态变量 */

static menu_page_t cur_page = PAGE_WELCOME;
static uint8_t cur_index = 0;
static uint8_t need_refresh = 1;
static uint8_t percent_val = 50;

/* 地址输入状态 */
static uint8_t addr_input[4];       /* 4个十六进制字符 */
static uint8_t addr_pos = 0;        /* 当前输入位置 0~3 */
static uint8_t addr_sel = 0;        /* 当前选中的hex字符索引 0~15 */
static uint8_t addr_is_slave = 0;   /* 1=从机请求模式, 0=主动写模式 */

/******************************************************************************************/
/* 初始化 */

void app_menu_init(void)
{
    cur_page = PAGE_WELCOME;
    cur_index = 0;
    need_refresh = 1;
    percent_val = 50;
}

/******************************************************************************************/
/* 页面绘制函数 */

/**
 * @brief       绘制欢迎界面
 */
static void draw_welcome(void)
{
    oled_clear();
    oled_show_string(18, 8,  "Curtain", 16);
    oled_show_string(6,  28, "Controller", 16);
    oled_show_string(0,  52, "Press any key...", 12);
    oled_refresh_gram();
}

/**
 * @brief       绘制主菜单
 */
static void draw_main_menu(void)
{
    uint8_t i;

    oled_clear();
    oled_show_string(0, 0, "== Main Menu ==", 12);

    for (i = 0; i < MAIN_MAX; i++)
    {
        if (i == cur_index)
            oled_show_string(0, 16 + i * 14, ">", 12);
        oled_show_string(8, 16 + i * 14, main_menu_items[i], 12);
    }

    oled_refresh_gram();
}

/**
 * @brief       绘制地址选择菜单
 */
static void draw_addr_menu(void)
{
    uint8_t i;

    oled_clear();
    oled_show_string(0, 0, "-- Set Address --", 12);

    for (i = 0; i < ADDR_MAX; i++)
    {
        if (i == cur_index)
            oled_show_string(0, 16 + i * 14, ">", 12);
        oled_show_string(8, 16 + i * 14, addr_menu_items[i], 12);
    }

    oled_refresh_gram();
}

/**
 * @brief       绘制地址输入界面
 */
static void draw_address_input(void)
{
    char buf[22];
    uint8_t i;

    oled_clear();
    oled_show_string(0, 0, "-- Input Address --", 12);

    /* 显示4个十六进制字符, 当前位置用[]标记 */
    for (i = 0; i < 4; i++)
    {
        if (i == addr_pos)
        {
            buf[i * 2]     = '[';
            buf[i * 2 + 1] = hex_chars[addr_input[i]];
        }
        else
        {
            buf[i * 2]     = ' ';
            buf[i * 2 + 1] = hex_chars[addr_input[i]];
        }
    }
    buf[8] = 'H';  /* 0x前缀后缀 */
    buf[9] = '\0';
    oled_show_string(8, 20, "0x", 16);
    oled_show_string(28, 20, buf, 16);

    /* 显示当前可选字符 */
    buf[0] = '<';
    buf[1] = hex_chars[addr_sel];
    buf[2] = '>';
    buf[3] = '\0';
    oled_show_string(50, 44, buf, 12);

    oled_refresh_gram();
}

/**
 * @brief       绘制从机请求等待界面
 */
static void draw_slave_request(void)
{
    char buf[8];

    oled_clear();
    oled_show_string(0, 0, "-- Slave Req --", 12);

    /* 显示已输入的地址 */
    buf[0] = '0';
    buf[1] = 'x';
    buf[2] = hex_chars[addr_input[0]];
    buf[3] = hex_chars[addr_input[1]];
    buf[4] = hex_chars[addr_input[2]];
    buf[5] = hex_chars[addr_input[3]];
    buf[6] = '\0';
    oled_show_string(0, 14, buf, 12);

    oled_show_string(0, 28, "1.Motor btn 5s", 12);
    oled_show_string(0, 40, "2.LED x2->K3", 12);
    oled_show_string(0, 52, "K3:Send  K0:Back", 12);
    oled_refresh_gram();
}

/**
 * @brief       绘制控制测试地址选择
 */
static void draw_ctrl_addr_sel(void)
{
    uint8_t i;

    oled_clear();
    oled_show_string(0, 0, "-- Select Addr --", 12);

    for (i = 0; i < CTRL_ADDR_MAX; i++)
    {
        if (i == cur_index)
            oled_show_string(0, 16 + i * 14, ">", 12);
        oled_show_string(8, 16 + i * 14, ctrl_addr_items[i], 12);
    }

    oled_refresh_gram();
}

/**
 * @brief       处理控制测试地址选择按键
 */
static void handle_ctrl_addr_sel(uint8_t key)
{
    if (key == KEY1_PRES)
    {
        if (cur_index > 0) { cur_index--; need_refresh = 1; }
    }
    else if (key == KEY2_PRES)
    {
        if (cur_index < CTRL_ADDR_MAX - 1) { cur_index++; need_refresh = 1; }
    }
    else if (key == KEY3_PRES)
    {
        if (cur_index == CTRL_ADDR_DEFAULT)
        {
            printf("[KEY3] Use default addr (FEFE)\r\n");
            g_curtain_use_custom = 0;
            cur_page = PAGE_CONTROL_MENU;
            cur_index = 0;
            need_refresh = 1;
        }
        else
        {
            printf("[KEY3] Custom addr input\r\n");
            cur_page = PAGE_CTRL_ADDR_INPUT;
            addr_is_slave = 0;
            addr_pos = 0;
            addr_sel = 0;
            addr_input[0] = addr_input[1] = addr_input[2] = addr_input[3] = 0;
            need_refresh = 1;
        }
    }
    else if (key == KEY0_PRES)
    {
        cur_page = PAGE_MAIN_MENU;
        cur_index = 0;
        need_refresh = 1;
    }
}

/**
 * @brief       绘制控制测试自定义地址输入
 */
static void draw_ctrl_addr_input(void)
{
    char buf[22];
    uint8_t i;

    oled_clear();
    oled_show_string(0, 0, "-- Custom Addr --", 12);

    for (i = 0; i < 4; i++)
    {
        buf[i * 2]     = (i == addr_pos) ? '[' : ' ';
        buf[i * 2 + 1] = hex_chars[addr_input[i]];
    }
    buf[8] = '\0';
    oled_show_string(8, 16, "0x", 16);
    oled_show_string(28, 16, buf, 16);

    buf[0] = '<';
    buf[1] = hex_chars[addr_sel];
    buf[2] = '>';
    buf[3] = '\0';
    oled_show_string(50, 44, buf, 12);

    oled_refresh_gram();
}

/**
 * @brief       处理控制测试自定义地址输入
 */
static void handle_ctrl_addr_input(uint8_t key)
{
    if (key == KEY1_PRES)
    {
        addr_sel = (addr_sel > 0) ? addr_sel - 1 : HEX_CHAR_COUNT - 1;
        addr_input[addr_pos] = addr_sel;
        need_refresh = 1;
    }
    else if (key == KEY2_PRES)
    {
        addr_sel = (addr_sel < HEX_CHAR_COUNT - 1) ? addr_sel + 1 : 0;
        addr_input[addr_pos] = addr_sel;
        need_refresh = 1;
    }
    else if (key == KEY3_PRES)
    {
        printf("[INPUT] pos=%d char=%c\r\n", addr_pos, hex_chars[addr_input[addr_pos]]);

        if (addr_pos < 3)
        {
            addr_pos++;
            addr_sel = addr_input[addr_pos];
            need_refresh = 1;
        }
        else
        {
            g_curtain_custom_addr_h = (addr_input[0] << 4) | addr_input[1];
            g_curtain_custom_addr_l = (addr_input[2] << 4) | addr_input[3];
            g_curtain_use_custom = 1;
            printf("[ADDR] Custom addr: 0x%02X%02X\r\n",
                   g_curtain_custom_addr_h, g_curtain_custom_addr_l);
            cur_page = PAGE_CONTROL_MENU;
            cur_index = 0;
            need_refresh = 1;
        }
    }
    else if (key == KEY0_PRES)
    {
        cur_page = PAGE_CTRL_ADDR_SEL;
        cur_index = 0;
        need_refresh = 1;
    }
}

/**
 * @brief       绘制控制测试菜单
 */
static void draw_ctrl_menu(void)
{
    uint8_t i;

    oled_clear();
    oled_show_string(0, 0, "-- Control --", 12);

    for (i = 0; i < CTRL_MAX; i++)
    {
        if (i == cur_index)
            oled_show_string(0, 12 + i * 10, ">", 12);
        oled_show_string(8, 12 + i * 10, ctrl_menu_items[i], 12);
    }

    oled_refresh_gram();
}

/**
 * @brief       绘制百分比选择界面
 */
static void draw_percent(void)
{
    char buf[22];
    uint8_t bar_len;
    uint8_t i;

    oled_clear();
    oled_show_string(0, 0, "-- Percent --", 12);

    oled_show_string(0, 16, "Value:", 12);
    oled_show_num(42, 16, percent_val, 3, 12);
    oled_show_string(66, 16, "%", 12);

    bar_len = (uint8_t)((uint16_t)percent_val * 16 / 100);
    for (i = 0; i < 16; i++)
        buf[i] = (i < bar_len) ? '#' : '-';
    buf[16] = '\0';
    oled_show_string(0, 30, buf, 12);

    oled_show_string(0, 48, "K1:UP K2:DN K3:OK", 12);
    oled_refresh_gram();
}

/******************************************************************************************/
/* 页面处理函数 */

/**
 * @brief       处理欢迎界面按键 (任意键进入主菜单)
 */
static void handle_welcome(uint8_t key)
{
    if (key != 0)
    {
        printf("[KEY] Any key -> Main Menu\r\n");
        cur_page = PAGE_MAIN_MENU;
        cur_index = 0;
        need_refresh = 1;
    }
}

/**
 * @brief       处理主菜单按键
 */
static void handle_main_menu(uint8_t key)
{
    if (key == KEY1_PRES)       /* 上移 */
    {
        if (cur_index > 0)
        {
            cur_index--;
            need_refresh = 1;
            printf("[KEY1] Cursor -> %s\r\n", main_menu_items[cur_index]);
        }
    }
    else if (key == KEY2_PRES)  /* 下移 */
    {
        if (cur_index < MAIN_MAX - 1)
        {
            cur_index++;
            need_refresh = 1;
            printf("[KEY2] Cursor -> %s\r\n", main_menu_items[cur_index]);
        }
    }
    else if (key == KEY3_PRES)  /* 确认 */
    {
        switch (cur_index)
        {
            case MAIN_SET_ADDR:
                printf("[KEY3] -> Set Address\r\n");
                cur_page = PAGE_ADDRESS_MENU;
                cur_index = 0;
                need_refresh = 1;
                break;

            case MAIN_CONTROL:
                printf("[KEY3] -> Control Test (select addr)\r\n");
                cur_page = PAGE_CTRL_ADDR_SEL;
                cur_index = 0;
                need_refresh = 1;
                break;

            default:
                break;
        }
    }
}

/**
 * @brief       处理地址选择菜单按键
 */
static void handle_addr_menu(uint8_t key)
{
    if (key == KEY1_PRES)       /* 上移 */
    {
        if (cur_index > 0)
        {
            cur_index--;
            need_refresh = 1;
        }
    }
    else if (key == KEY2_PRES)  /* 下移 */
    {
        if (cur_index < ADDR_MAX - 1)
        {
            cur_index++;
            need_refresh = 1;
        }
    }
    else if (key == KEY3_PRES)  /* 确认 */
    {
        switch (cur_index)
        {
            case ADDR_WRITE:
                printf("[KEY3] -> Write Address (input)\r\n");
                cur_page = PAGE_ADDRESS_INPUT;
                addr_is_slave = 0;
                addr_pos = 0;
                addr_sel = 0;
                addr_input[0] = 0;
                addr_input[1] = 0;
                addr_input[2] = 0;
                addr_input[3] = 0;
                need_refresh = 1;
                break;

            case ADDR_SLAVE:
                printf("[KEY3] -> Slave Request (input addr first)\r\n");
                cur_page = PAGE_ADDRESS_INPUT;
                addr_is_slave = 1;
                addr_pos = 0;
                addr_sel = 0;
                addr_input[0] = 0;
                addr_input[1] = 0;
                addr_input[2] = 0;
                addr_input[3] = 0;
                need_refresh = 1;
                break;

            default:
                break;
        }
    }
    else if (key == KEY0_PRES)  /* 返回 */
    {
        printf("[KEY0] Back to main menu\r\n");
        cur_page = PAGE_MAIN_MENU;
        cur_index = 0;
        need_refresh = 1;
    }
}

/**
 * @brief       处理地址输入按键
 *              KEY1: 上一个字符  KEY2: 下一个字符
 *              KEY3: 确认当前字符  KEY0: 返回
 */
static void handle_address_input(uint8_t key)
{
    if (key == KEY1_PRES)       /* 上一个字符 */
    {
        if (addr_sel > 0)
            addr_sel--;
        else
            addr_sel = HEX_CHAR_COUNT - 1;

        addr_input[addr_pos] = addr_sel;
        need_refresh = 1;
    }
    else if (key == KEY2_PRES)  /* 下一个字符 */
    {
        if (addr_sel < HEX_CHAR_COUNT - 1)
            addr_sel++;
        else
            addr_sel = 0;

        addr_input[addr_pos] = addr_sel;
        need_refresh = 1;
    }
    else if (key == KEY3_PRES)  /* 确认当前字符, 移到下一位 */
    {
        printf("[INPUT] pos=%d char=%c\r\n", addr_pos, hex_chars[addr_input[addr_pos]]);

        if (addr_pos < 3)
        {
            addr_pos++;
            addr_sel = addr_input[addr_pos]; /* 恢复该位之前的值 */
            need_refresh = 1;
        }
        else
        {
            /* 4位全部输入完成 */
            if (addr_is_slave)
            {
                /* 从机模式: 显示提示, 等待用户按KEY3发送 */
                printf("[ADDR] Slave mode: waiting for motor button\r\n");
                cur_page = PAGE_ADDRESS_SLAVE;
                need_refresh = 1;
            }
            else
            {
                /* 主动写: 直接发送 */
                uint8_t addr_h = (addr_input[0] << 4) | addr_input[1];
                uint8_t addr_l = (addr_input[2] << 4) | addr_input[3];

                printf("[ADDR] Write address: 0x%02X%02X\r\n", addr_h, addr_l);
                bsp_curtain_set_address(addr_h, addr_l);

                oled_clear();
                oled_show_string(0, 12, "Address Sent!", 12);
                oled_show_string(0, 28, "Wait for LED x5", 12);
                oled_refresh_gram();
                HAL_Delay(2000);
                cur_page = PAGE_ADDRESS_MENU;
                cur_index = 0;
                need_refresh = 1;
            }
        }
    }
    else if (key == KEY0_PRES)  /* 返回 */
    {
        printf("[KEY0] Cancel address input\r\n");
        cur_page = PAGE_ADDRESS_MENU;
        cur_index = 0;
        need_refresh = 1;
    }
}

/**
 * @brief       处理从机请求界面按键
 */
static void handle_slave_request(uint8_t key)
{
    if (key == KEY3_PRES)  /* 发送 */
    {
        uint8_t addr_h = (addr_input[0] << 4) | addr_input[1];
        uint8_t addr_l = (addr_input[2] << 4) | addr_input[3];

        printf("[KEY3] Slave send address: 0x%02X%02X\r\n", addr_h, addr_l);
        bsp_curtain_set_address(addr_h, addr_l);

        oled_clear();
        oled_show_string(0, 12, "Address Sent!", 12);
        oled_show_string(0, 28, "Wait for LED x5", 12);
        oled_refresh_gram();
        HAL_Delay(2000);
        cur_page = PAGE_ADDRESS_MENU;
        cur_index = 0;
        need_refresh = 1;
    }
    else if (key == KEY0_PRES)  /* 返回 */
    {
        printf("[KEY0] Cancel slave request\r\n");
        cur_page = PAGE_ADDRESS_MENU;
        cur_index = 0;
        need_refresh = 1;
    }
}

/**
 * @brief       处理控制菜单按键
 */
static void handle_ctrl_menu(uint8_t key)
{
    if (key == KEY1_PRES)       /* 上移 */
    {
        if (cur_index > 0)
        {
            cur_index--;
            need_refresh = 1;
            printf("[KEY1] Cursor -> %s\r\n", ctrl_menu_items[cur_index]);
        }
    }
    else if (key == KEY2_PRES)  /* 下移 */
    {
        if (cur_index < CTRL_MAX - 1)
        {
            cur_index++;
            need_refresh = 1;
            printf("[KEY2] Cursor -> %s\r\n", ctrl_menu_items[cur_index]);
        }
    }
    else if (key == KEY3_PRES)  /* 确认 */
    {
        switch (cur_index)
        {
            case CTRL_OPEN:
                printf("[KEY3] Open curtain\r\n");
                bsp_curtain_open();
                break;

            case CTRL_CLOSE:
                printf("[KEY3] Close curtain\r\n");
                bsp_curtain_close();
                break;

            case CTRL_STOP:
                printf("[KEY3] Stop curtain\r\n");
                bsp_curtain_stop();
                break;

            case CTRL_PERCENT:
                printf("[KEY3] -> Percent page\r\n");
                cur_page = PAGE_PERCENT;
                need_refresh = 1;
                return;

            case CTRL_QUERY:
                printf("[KEY3] Query position\r\n");
                bsp_curtain_query_position();
                break;

            default:
                break;
        }

        oled_clear();
        oled_show_string(0, 24, "  Sent!", 16);
        oled_refresh_gram();
        HAL_Delay(500);
        need_refresh = 1;
    }
    else if (key == KEY0_PRES)  /* 返回 */
    {
        printf("[KEY0] Back to main menu\r\n");
        cur_page = PAGE_MAIN_MENU;
        cur_index = 0;
        need_refresh = 1;
    }
}

/**
 * @brief       处理百分比页面按键
 */
static void handle_percent(uint8_t key)
{
    if (key == KEY2_PRES)       /* 下移 = 减少10% */
    {
        if (percent_val >= 10)
        {
            percent_val -= 10;
            need_refresh = 1;
            printf("[KEY2] Percent -> %d%%\r\n", percent_val);
        }
    }
    else if (key == KEY1_PRES)  /* 上移 = 增加10% */
    {
        if (percent_val <= 90)
        {
            percent_val += 10;
            need_refresh = 1;
            printf("[KEY1] Percent -> %d%%\r\n", percent_val);
        }
    }
    else if (key == KEY3_PRES)  /* 确认 = 发送 */
    {
        printf("[KEY3] Send percent: %d%%\r\n", percent_val);
        bsp_curtain_set_percent(percent_val);

        oled_clear();
        oled_show_string(0, 24, "  Sent!", 16);
        oled_refresh_gram();
        HAL_Delay(500);
        need_refresh = 1;
    }
    else if (key == KEY0_PRES)  /* 返回 */
    {
        printf("[KEY0] Back to control menu\r\n");
        cur_page = PAGE_CONTROL_MENU;
        cur_index = CTRL_PERCENT;
        need_refresh = 1;
    }
}

/******************************************************************************************/
/* 主处理函数 */

/**
 * @brief       菜单处理(主循环调用)
 * @param       key: 当前按键值 (0=无按键)
 */
void app_menu_process(uint8_t key)
{
    /* 刷新显示 */
    if (need_refresh)
    {
        need_refresh = 0;

        switch (cur_page)
        {
            case PAGE_WELCOME:       draw_welcome();       break;
            case PAGE_MAIN_MENU:     draw_main_menu();     break;
            case PAGE_ADDRESS_MENU:  draw_addr_menu();     break;
            case PAGE_ADDRESS_INPUT: draw_address_input(); break;
            case PAGE_ADDRESS_SLAVE: draw_slave_request(); break;
            case PAGE_CTRL_ADDR_SEL:    draw_ctrl_addr_sel();    break;
            case PAGE_CTRL_ADDR_INPUT:  draw_ctrl_addr_input();  break;
            case PAGE_CONTROL_MENU:  draw_ctrl_menu();     break;
            case PAGE_PERCENT:       draw_percent();       break;
            default: break;
        }
    }

    /* 处理按键 */
    switch (cur_page)
    {
        case PAGE_WELCOME:       handle_welcome(key);       break;
        case PAGE_MAIN_MENU:     handle_main_menu(key);     break;
        case PAGE_ADDRESS_MENU:  handle_addr_menu(key);     break;
        case PAGE_ADDRESS_INPUT: handle_address_input(key); break;
        case PAGE_ADDRESS_SLAVE: handle_slave_request(key); break;
        case PAGE_CTRL_ADDR_SEL:    handle_ctrl_addr_sel(key);    break;
        case PAGE_CTRL_ADDR_INPUT:  handle_ctrl_addr_input(key);  break;
        case PAGE_CONTROL_MENU:  handle_ctrl_menu(key);     break;
        case PAGE_PERCENT:       handle_percent(key);       break;
        default: break;
    }
}
