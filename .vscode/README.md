# VS Code 配置说明

## 已创建的文件

### c_cpp_properties.json
这个文件配置了C/C++智能提示的include路径，解决了"无法打开源文件"的问题。

## 如何生效

1. **重新加载VS Code窗口**
   - 按 `Ctrl + Shift + P`
   - 输入 "Reload Window"
   - 选择 "Developer: Reload Window"

2. **或者直接重启VS Code**

## 路径配置说明

配置文件包含了以下重要路径：
- ` Drivers/BSP/** ` - 外设驱动头文件
- ` Drivers/SYSTEM/** ` - 系统头文件（sys、delay、usart等）
- ` Drivers/CMSIS/** ` - ARM Cortex-M内核头文件
- ` Drivers/STM32F1xx_HAL_Driver/Inc/** ` - STM32 HAL库头文件
- ` User/** ` - 用户程序

## 注意事项

如果include路径仍然报错，可能需要：
1. 检查路径是否正确（特别是中文路径）
2. 确保使用的是相对路径 `./SYSTEM/...`
3. 确保头文件确实存在于指定路径

## Keil MDK用户

这个配置主要用于VS Code的智能提示，不会影响Keil MDK的编译。
Keil MDK使用自己的项目配置文件（.uvprojx）来管理include路径。
