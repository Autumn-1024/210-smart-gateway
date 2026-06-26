# 窗帘网关 项目记忆

## 项目信息
- **MCU**: STM32F103C8T6 (Cortex-M3, 64KB Flash, 20KB RAM)
- **本地路径**: C:\Users\Autumn\Desktop\窗帘网关
- **GitHub**: https://github.com/Autumn-1024/curtain-gateway
- **创建日期**: 2026-06-16
- **参考工程**: D:\MiCloud\项目\智能窗帘控制\F429系列RS485

## 硬件配置
- **LED**: PC13 (低电平点亮)
- **按键**: KEY0=PA4, KEY1=PA5, KEY2=PA6, KEY_UP=PA7 (KEY0/1/2低有效, KEY_UP高有效)
- **OLED**: SSD1306 128x64, IIC (PB6=SCL, PB7=SDA, 地址0x78)
- **RS485**: USART2 PA2(TX)/PA3(RX), 9600bps, 自动方向
- **ESP01S**: WiFi模块，提供Web控制
- **调试串口**: USART1 PA9(TX)/PA10(RX), 115200bps, printf重定向

## 窗帘协议 (杜亚标准)
- 帧格式: `55` + 设备地址(2) + 功能码(1) + 数据地址(1) + 数据长度(1) + 数据(N) + CRC16(2)
- 功能码: 0x01=读, 0x02=写, 0x03=控制, 0x04=从机请求
- 控制命令(0x03): 0x01=打开, 0x02=关闭, 0x03=停止, 0x04=百分比, 0x07=删行程, 0x08=恢复出厂
- 读寄存器(0x01): 0x02=位置, 0x05=状态
- CRC16: 多项式0xA001, 初始值0xFFFF
- 预计算帧(地址FE FE): 打开=55 FE FE 03 01 B9 24, 关闭=55 FE FE 03 02 F9 25, 停止=55 FE FE 03 03 38 E5

## 按键映射
| 按键 | 功能 |
|------|------|
| KEY0 (PA4) | 返回 |
| KEY1 (PA5) | 下移/减少 |
| KEY2 (PA6) | 上移/增加 |
| KEY_UP (PA7) | 确认/执行 |

## 工程结构
```
窗帘网关/
├── Core/Inc/          ← main.h, hal_conf.h, stm32f1xx_it.h
├── Core/Src/          ← main.c, stm32f1xx_it.c, system_stm32f1xx.c
├── Core/Startup/      ← startup_stm32f103xe.s
├── Drivers/           ← CMSIS + HAL_Driver (零修改)
├── BSP/Inc/           ← bsp_*.h
├── BSP/Src/           ← bsp_*.c (led, key, uart, oled, rs485, curtain, esp01s)
├── App/Inc/           ← app_task.h, app_menu.h, app_web.h
├── App/Src/           ← app_task.c, app_menu.c, app_web.c
├── Docs/              ← Web接口文档, 杜亚电机RS485通信协议
├── MDK-ARM/           ← Keil工程
├── Output/            ← 编译产物
├── claw.md            ← 本文件
└── README.md
```

## Keil 配置
- **启动文件**: startup_stm32f103xe.s (模板原始配置)
- **宏定义**: STM32F103xE,USE_HAL_DRIVER
- **烧录器**: ST-Link (模板已配置)
- **编译器**: V5.06 ARMCC

## HAL模块开关
- GPIO, RCC, FLASH, PWR, CORTEX, DMA, UART

## 开发注意事项
1. OLED使用软件I2C模式 (PB6=SCL, PB7=SDA)
2. RS485使用HAL半双工自动方向管理
3. KEY_UP是高电平有效，其他按键低电平有效
4. 菜单驱动模式：KEY0=返回, KEY1=下移, KEY2=上移, KEY_UP=确认

## 常用命令
```powershell
# 编译
& "C:\Program Files (x86)\MDK\core\UV4\UV4.exe" -b "C:\Users\Autumn\Desktop\窗帘网关\MDK-ARM\atk_f103c8t6.uvprojx" -o "C:\Users\Autumn\Desktop\窗帘网关\Output\build_log.txt" -j0

# 烧录
& "C:\Program Files (x86)\MDK\core\UV4\UV4.exe" -f "C:\Users\Autumn\Desktop\窗帘网关\MDK-ARM\atk_f103c8t6.uvprojx" -o "C:\Users\Autumn\Desktop\窗帘网关\Output\flash_log.txt" -j0
```

## 更新日志
- 2026-06-16: 创建工程，基于F1模板，添加OLED+KEY+RS485+CURTAIN驱动
- 2026-06-26: 复制到桌面改名"窗帘网关"，上传GitHub (curtain-gateway)
