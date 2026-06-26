/**
 ****************************************************************************************************
 * @file        bsp_curtain.h
 * @author      Autumn
 * @version     V2.0
 * @date        2026-06-16
 * @brief       杜亚窗帘电机控制 (RS485协议)
 ****************************************************************************************************
 * @attention
 *
 * 协议: 杜亚RS485电动窗帘通信协议
 * 帧格式: 55 + 设备地址(2) + 功能码(1) + 数据地址(1) + 数据长度(1) + 数据(N) + CRC16(2)
 * 设备地址: 默认 0xFEFE
 *
 ****************************************************************************************************
 */

#ifndef __BSP_CURTAIN_H
#define __BSP_CURTAIN_H

#include "stm32f1xx_hal.h"

/******************************************************************************************/
/* 设备地址 (默认出厂地址) */

#define CURTAIN_ADDR_H      0xFE
#define CURTAIN_ADDR_L      0xFE

/******************************************************************************************/
/* 可配置设备地址 */

extern uint8_t g_curtain_use_custom;    /* 1=使用自定义地址, 0=使用默认地址 */
extern uint8_t g_curtain_custom_addr_h; /* 自定义地址高字节 */
extern uint8_t g_curtain_custom_addr_l; /* 自定义地址低字节 */

/******************************************************************************************/
/* 功能码定义 */

#define CURTAIN_FUNC_READ       0x01    /* 读寄存器 */
#define CURTAIN_FUNC_WRITE      0x02    /* 写寄存器 */
#define CURTAIN_FUNC_CTRL       0x03    /* 控制命令 */
#define CURTAIN_FUNC_SLAVE_REQ  0x04    /* 从机请求 */

/******************************************************************************************/
/* 控制命令 (功能码 0x03) */

#define CURTAIN_CMD_OPEN        0x01    /* 打开 */
#define CURTAIN_CMD_CLOSE       0x02    /* 关闭 */
#define CURTAIN_CMD_STOP        0x03    /* 停止 */
#define CURTAIN_CMD_PERCENT     0x04    /* 百分比控制 */
#define CURTAIN_CMD_DEL_TRAVEL  0x07    /* 删除行程 */
#define CURTAIN_CMD_RESET       0x08    /* 恢复出厂 */

/******************************************************************************************/
/* 寄存器地址 (功能码 0x01/0x02) */

#define CURTAIN_REG_ADDR_L      0x00    /* 设备地址低字节 */
#define CURTAIN_REG_ADDR_H      0x01    /* 设备地址高字节 */
#define CURTAIN_REG_POSITION    0x02    /* 当前位置 (0~100%, 0xFF=无行程) */
#define CURTAIN_REG_DIRECTION   0x03    /* 电机方向 */
#define CURTAIN_REG_HAND_PULL   0x04    /* 手拉使能 */
#define CURTAIN_REG_STATUS      0x05    /* 运行状态 (0=STOP,1=OPEN,2=CLOSE,3=SETTING) */
#define CURTAIN_REG_DEVICE_TYPE 0xF0    /* 设备类型 */
#define CURTAIN_REG_VERSION     0xFD    /* 软件版本 */
#define CURTAIN_REG_PROTOCOL    0xFE    /* 协议版本 */

/******************************************************************************************/
/* 外部变量 (OLED显示用) */

extern uint8_t g_curtain_last_cmd[16];
extern uint8_t g_curtain_last_cmd_len;

/******************************************************************************************/
/* 函数声明 */

void bsp_curtain_init(void);
void bsp_curtain_open(void);
void bsp_curtain_close(void);
void bsp_curtain_stop(void);
void bsp_curtain_set_percent(uint8_t percent);
void bsp_curtain_query_position(void);
void bsp_curtain_query_status(void);
void bsp_curtain_del_travel(void);
void bsp_curtain_factory_reset(void);
void bsp_curtain_set_address(uint8_t addr_h, uint8_t addr_l);

#endif
