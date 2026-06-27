bavege# F103-Curtain-Control

STM32F103C8T6 窗帘控制系统 — OLED + 4按键 + RS485

## 硬件配置
- **MCU**: STM32F103C8T6
- **OLED**: SSD1306 128x64, 4线SPI (PB12~PB15, PA4)
- **按键**: KEY0=PB5, KEY1=PB6, KEY2=PB7, KEY_UP=PA1
- **RS485**: USART2 PA2/PA3, PA5方向控制, 9600bps
- **调试串口**: USART1 PA9/PA10, 115200bps

## 功能说明
- KEY0: 打开窗帘
- KEY1: 关闭窗帘
- KEY2: 停止运行
- KEY_UP: 查询状态
- OLED显示操作状态
- RS485收发窗帘控制协议

## 窗帘协议
7字节帧: `55 FE FE [长度] [功能码] [校验...]`

## 编译
使用 Keil MDK 打开 `MDK-ARM/atk_f103c8t6.uvprojx`

## 烧录
ST-Link 烧录，已在工程中配置

## 参考
- 窗帘协议来源: `D:\MiCloud\项目\智能窗帘控制\F429系列RS485`

## 作者
Autumn
