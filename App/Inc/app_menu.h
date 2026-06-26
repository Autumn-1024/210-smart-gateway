/**
 ****************************************************************************************************
 * @file        app_menu.h
 * @author      Autumn
 * @version     V2.0
 * @date        2026-06-17
 * @brief       OLED菜单系统
 ****************************************************************************************************
 * @attention
 *
 * 0.96寸 OLED 128x64, 12号字体
 * 按键: KEY0=返回, KEY1=上移, KEY2=下移, KEY3=确认
 *
 ****************************************************************************************************
 */

#ifndef __APP_MENU_H
#define __APP_MENU_H

#include "stm32f1xx_hal.h"

/******************************************************************************************/
/* 菜单页面枚举 */

typedef enum
{
    PAGE_WELCOME = 0,       /* 欢迎界面 */
    PAGE_MAIN_MENU,         /* 主菜单 */
    PAGE_ADDRESS_MENU,      /* 修改地址 - 选择方式 */
    PAGE_ADDRESS_INPUT,     /* 修改地址 - 手动输入 */
    PAGE_ADDRESS_SLAVE,     /* 修改地址 - 从机请求 */
    PAGE_CTRL_ADDR_SEL,     /* 控制测试 - 选择地址 */
    PAGE_CTRL_ADDR_INPUT,   /* 控制测试 - 输入自定义地址 */
    PAGE_CONTROL_MENU,      /* 控制测试菜单 */
    PAGE_PERCENT,           /* 百分比选择 */
    PAGE_MAX
} menu_page_t;

/******************************************************************************************/
/* 主菜单项 */

typedef enum
{
    MAIN_SET_ADDR = 0,      /* 修改电机地址 */
    MAIN_CONTROL,           /* 控制测试 */
    MAIN_MAX
} main_menu_item_t;

/******************************************************************************************/
/* 地址菜单项 */

typedef enum
{
    ADDR_WRITE = 0,         /* 主动写地址 */
    ADDR_SLAVE,             /* 从机请求分配地址 */
    ADDR_MAX
} addr_menu_item_t;

/******************************************************************************************/
/* 控制地址选择项 */

typedef enum
{
    CTRL_ADDR_DEFAULT = 0,  /* 默认地址 FEFE */
    CTRL_ADDR_CUSTOM,       /* 自定义地址 */
    CTRL_ADDR_MAX
} ctrl_addr_item_t;

/******************************************************************************************/
/* 控制菜单项 */

typedef enum
{
    CTRL_OPEN = 0,          /* 正转(打开) */
    CTRL_CLOSE,             /* 反转(关闭) */
    CTRL_STOP,              /* 停止 */
    CTRL_PERCENT,           /* 百分比控制 */
    CTRL_QUERY,             /* 查询位置 */
    CTRL_MAX
} ctrl_menu_item_t;

/******************************************************************************************/
/* 函数声明 */

void app_menu_init(void);
void app_menu_process(uint8_t key);

#endif
