# 窗帘控制器 Web 接口文档

> 适用于 F103-Curtain-Control 窗帘控制系统
> 通过 ESP01S WiFi 模块提供 HTTP 接口

---

## 1. 基本信息

| 项目 | 值 |
|------|-----|
| 协议 | HTTP/1.1 |
| 默认端口 | 80 |
| IP 地址 | 由路由器 DHCP 分配，开机后显示在 OLED 屏幕上 |
| 网络 | WiFi (ESP01S) |
| 支持设备 | 局域网内任意设备（手机/电脑/平板） |

---

## 2. 控制的窗帘设备

| 编号 | 设备地址 | 说明 |
|------|---------|------|
| C1 | 0x0201 | 窗帘 1 |
| C2 | 0x0202 | 窗帘 2 |
| C3 | 0x0203 | 窗帘 3 |
| C4 | 0x0204 | 窗帘 4 |

---

## 3. API 接口

### 3.1 控制窗帘

**请求方式：** GET

**URL 格式：** `http://{ip}/c{编号}/{动作}`

| 参数 | 说明 | 取值 |
|------|------|------|
| ip | 控制器 IP 地址 | 如 192.168.1.100 |
| 编号 | 窗帘编号 | 1, 2, 3, 4 |
| 动作 | 控制动作 | open, close |

**示例：**

```
GET http://192.168.1.100/c1/open    → 打开窗帘 1
GET http://192.168.1.100/c1/close   → 关闭窗帘 1
GET http://192.168.1.100/c2/open    → 打开窗帘 2
GET http://192.168.1.100/c3/close   → 关闭窗帘 3
GET http://192.168.1.100/c4/open    → 打开窗帘 4
```

**成功响应：**

```
HTTP/1.1 200 OK
Content-Type: text/plain
Connection: close

C1 OPEN OK
```

```
HTTP/1.1 200 OK
Content-Type: text/plain
Connection: close

C2 CLOSE OK
```

**失败响应：**

```
HTTP/1.1 404 Not Found
Content-Type: text/plain
Connection: close

Not Found
```

### 3.2 获取控制页面

**请求方式：** GET

**URL：** `http://{ip}/`

**响应：** 返回 HTML 控制页面（包含 4 个窗帘的 OPEN/CLOSE 按钮）

---

## 4. 调用示例

### 4.1 浏览器

直接在地址栏输入：
```
http://192.168.1.100/c1/open
```

### 4.2 JavaScript (Fetch)

```javascript
// 打开窗帘 1
fetch('http://192.168.1.100/c1/open')
  .then(response => response.text())
  .then(data => console.log(data));  // "C1 OPEN OK"

// 关闭窗帘 3
fetch('http://192.168.1.100/c3/close')
  .then(response => response.text())
  .then(data => console.log(data));  // "C3 CLOSE OK"
```

### 4.3 Python (requests)

```python
import requests

ip = "192.168.1.100"

# 打开窗帘 1
r = requests.get(f"http://{ip}/c1/open")
print(r.text)  # C1 OPEN OK

# 关闭窗帘 4
r = requests.get(f"http://{ip}/c4/close")
print(r.text)  # C4 CLOSE OK
```

### 4.4 cURL

```bash
curl http://192.168.1.100/c1/open
curl http://192.168.1.100/c2/close
```

---

## 5. RS485 协议参考

控制器通过 RS485 总线向窗帘电机发送杜亚标准协议帧：

| 操作 | RS485 帧数据 (HEX) |
|------|-------------------|
| C1 open | 55 02 01 03 01 B9 44 |
| C1 close | 55 02 01 03 02 F9 45 |
| C2 open | 55 02 02 03 01 49 44 |
| C2 close | 55 02 02 03 02 09 45 |
| C3 open | 55 02 03 03 01 18 84 |
| C3 close | 55 02 03 03 02 58 85 |
| C4 open | 55 02 04 03 01 A9 45 |
| C4 close | 55 02 04 03 02 E9 44 |

帧格式：`起始码(55) + 设备地址(2) + 功能码(03) + 命令(01开/02关) + CRC16(2)`

详细协议见 `杜亚电机RS485通信协议.md`

---

## 6. 注意事项

1. 控制器与设备需在同一局域网内
2. IP 地址由路由器分配，可能变化（建议路由器绑定固定 IP）
3. 每次 HTTP 请求发送后会自动关闭连接（Connection: close）
4. 请求超时建议设置 3~5 秒
5. 不支持并发请求（单连接模式）

---

## 作者

Autumn
