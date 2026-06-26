# 窗帘网关

STM32F103C8T6 智能窗帘网关 — 本地按键控制 + WiFi 网络控制

通过 RS485 总线控制杜亚标准协议的窗帘电机，支持 4 路窗帘独立控制。

## 硬件配置

| 模块 | 型号/引脚 | 说明 |
|------|----------|------|
| MCU | STM32F103C8T6 | Cortex-M3, 64KB Flash, 20KB RAM |
| OLED | SSD1306 128x64 | I2C (PB6=SCL, PB7=SDA), 地址 0x78 |
| 按键 | KEY0=PA4, KEY1=PA5, KEY2=PA6, KEY_UP=PA7 | KEY0/1/2 低有效, KEY_UP 高有效 |
| RS485 | USART2 PA2(TX)/PA3(RX) | 9600bps, 自动方向控制 |
| WiFi | ESP01S (ESP8266) | AP 模式, 提供 HTTP 服务 |
| 调试串口 | USART1 PA9(TX)/PA10(RX) | 115200bps, printf 重定向 |
| LED | PC13 | 低电平点亮, 心跳指示 |

## 控制方式

### 1. 网络控制（WiFi Web）

ESP01S 连接 WiFi 后启动 HTTP 服务器，局域网内任意设备可通过浏览器或 API 控制窗帘。

**WiFi 配置：**
- SSID：`210`
- 密码：`12345678`
- 端口：`80`
- IP：由路由器 DHCP 分配，开机后显示在 OLED 屏幕上

**Web 控制页面：**

浏览器访问 `http://{ip}/` 即可打开控制页面，包含 4 个窗帘的 OPEN/CLOSE 按钮，支持手机/电脑/平板。

**HTTP API：**

```
GET http://{ip}/c{编号}/{动作}
```

| 参数 | 取值 | 说明 |
|------|------|------|
| 编号 | 1, 2, 3, 4 | 窗帘编号 |
| 动作 | open, close | 打开 / 关闭 |

**示例：**

```bash
# 打开窗帘 1
curl http://192.168.1.100/c1/open

# 关闭窗帘 3
curl http://192.168.1.100/c3/close
```

**响应：**
```
C1 OPEN OK
C3 CLOSE OK
```

**调用方式：**

```python
# Python
import requests
r = requests.get("http://192.168.1.100/c1/open")
print(r.text)  # C1 OPEN OK
```

```javascript
// JavaScript
fetch('http://192.168.1.100/c1/open')
  .then(r => r.text())
  .then(console.log);  // C1 OPEN OK
```

### 2. 终端控制（OLED 菜单 + 按键）

通过板载 4 个按键和 OLED 屏幕进行本地控制，无需网络。

**按键映射：**

| 按键 | 功能 |
|------|------|
| KEY0 (PA4) | 返回上级菜单 |
| KEY1 (PA5) | 上移 / 增加 |
| KEY2 (PA6) | 下移 / 减少 |
| KEY_UP (PA7) | 确认 / 执行 |

**菜单结构：**

```
欢迎界面（任意键进入）
└── 主菜单
    ├── 1. Set Address（设置设备地址）
    │   ├── Write Address    → 输入4位十六进制地址 → 发送写地址命令
    │   └── Slave Request    → 输入地址 → 等待电机配对 → 发送从机请求
    │
    └── 2. Control Test（控制测试）
        ├── Default (FEFE)   → 使用默认广播地址
        └── Custom Addr      → 输入自定义地址
            └── 控制菜单
                ├── Open       → 打开窗帘
                ├── Close      → 关闭窗帘
                ├── Stop       → 停止运行
                ├── Percent    → 设置百分比（0~100%，步进10%）
                └── Query Pos  → 查询当前位置
```

**操作流程：**

1. 开机显示欢迎界面，按任意键进入主菜单
2. 选择 "Control Test" → 选择地址模式（默认或自定义）
3. 在控制菜单中选择操作（开/关/停/百分比/查询）
4. OLED 显示 "Sent!" 表示命令已发送
5. 串口打印详细日志（115200bps）

### 3. 串口调试（USART1）

通过 USB 转串口连接 USART1 (PA9/PA10, 115200bps)，可查看实时日志：

```
[WEB] WiFi connected!
[WEB] IP: 192.168.1.100
[WEB] Server started on port 80
[WEB] 0 GET /c1/open
[WEB] -> C1 open
[KEY3] Open curtain
[KEY3] Send percent: 70%
```

## 窗帘协议（杜亚标准）

通过 RS485 总线通信，帧格式：

```
起始码(1) + 设备地址(2) + 功能码(1) + 数据地址(1) + 数据长度(1) + 数据(N) + CRC16(2)
```

**功能码：**

| 功能码 | 说明 |
|--------|------|
| 0x01 | 读寄存器 |
| 0x02 | 写寄存器 |
| 0x03 | 控制命令 |
| 0x04 | 从机请求 |

**控制命令（功能码 0x03）：**

| 命令 | 说明 |
|------|------|
| 0x01 | 打开 |
| 0x02 | 关闭 |
| 0x03 | 停止 |
| 0x04 | 设置百分比 |
| 0x07 | 删行程 |
| 0x08 | 恢复出厂 |

**预计算帧（默认地址 0xFEFE）：**

| 操作 | RS485 帧 (HEX) |
|------|---------------|
| 打开 | `55 FE FE 03 01 B9 24` |
| 关闭 | `55 FE FE 03 02 F9 25` |
| 停止 | `55 FE FE 03 03 38 E5` |
| 查询 | `55 FE FE 01 00 79 84` |

CRC16 采用 MODBUS 算法（多项式 0xA001，初始值 0xFFFF）。

详细协议见 `Docs/杜亚电机RS485通信协议.md`。

## 工程结构

```
窗帘网关/
├── Core/
│   ├── Inc/           ← main.h, hal_conf.h, stm32f1xx_it.h
│   ├── Src/           ← main.c, stm32f1xx_it.c, system_stm32f1xx.c
│   └── Startup/       ← startup_stm32f103xe.s
├── Drivers/           ← CMSIS + HAL_Driver（零修改）
├── BSP/
│   ├── Inc/           ← bsp_*.h
│   └── Src/           ← bsp_led.c, bsp_key.c, bsp_uart.c, bsp_oled.c,
│                        bsp_rs485.c, bsp_curtain.c, bsp_esp01s.c
├── App/
│   ├── Inc/           ← app_task.h, app_menu.h, app_web.h
│   └── Src/           ← app_task.c（主循环）, app_menu.c（OLED菜单）, app_web.c（Web控制）
├── Docs/              ← Web接口文档, 杜亚电机RS485通信协议
├── MDK-ARM/           ← Keil 工程文件
├── Output/            ← 编译产物
├── claw.md            ← 项目记忆
└── README.md
```

## 编译与烧录

**编译：**

```powershell
& "C:\Program Files (x86)\MDK\core\UV4\UV4.exe" -r "MDK-ARM\atk_f103c8t6.uvprojx" -o "Output\build_log.txt" -j0
```

**烧录：**

```powershell
& "C:\Program Files (x86)\MDK\core\UV4\UV4.exe" -f "MDK-ARM\atk_f103c8t6.uvprojx" -o "Output\flash_log.txt" -j0
```

**编译 + 烧录一步到位：**

```powershell
& "C:\Program Files (x86)\MDK\core\UV4\UV4.exe" -r "MDK-ARM\atk_f103c8t6.uvprojx" -o "Output\build_log.txt" -j0
& "C:\Program Files (x86)\MDK\core\UV4\UV4.exe" -f "MDK-ARM\atk_f103c8t6.uvprojx" -o "Output\flash_log.txt" -j0
```

烧录器：ST-Link（SWD 模式，Connect Under Reset）

## 参考

- 窗帘协议来源：`D:\MiCloud\项目\智能窗帘控制\F429系列RS485`
- Web 接口文档：`Docs/Web接口文档.md`
- 杜亚协议文档：`Docs/杜亚电机RS485通信协议.md`

## 作者

Autumn
