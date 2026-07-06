# ColdChainMonitor — 项目结构与模块说明

> 项目路径: `E:\jinjiangxueyuan\Stm32Project\ColdChainMonitor`
> 主控芯片: STM32F051K8 | 编译器: Keil MDK-ARM V5

---

## 一、目录结构

```
ColdChainMonitor/
├── Docs/                      # 文档
│   └── BOM.md                 # 物料清单
├── Drivers/                   # STM32 驱动库 (CMSIS + HAL/LL)
│   ├── CMSIS/                 # ARM Cortex 内核与设备外设层
│   └── STM32F0xx_HAL_Driver/ # STM32F0 HAL/LL 外设驱动
├── Inc/                       # 项目头文件 (.h)
│   ├── main.h                 # 系统参数宏定义
│   ├── alarm.h                # 报警模块
│   ├── dht22.h                # 温湿度传感器
│   ├── esp8266.h              # WiFi 模块
│   ├── font_lcd.h             # TFT 字库
│   ├── gps.h                  # GPS 模块
│   ├── lcd.h                  # TFT 彩屏驱动
│   ├── mpu6050.h              # 加速度传感器
│   ├── mqtt.h                 # MQTT 协议
│   └── spi.h                  # SPI 总线定义
├── Src/                       # 项目源码 (.c)
│   ├── main.c                 # 主程序入口
│   ├── alarm.c                # 报警控制
│   ├── dht22.c                # DHT22 驱动
│   ├── esp8266.c              # ESP8266 AT 指令驱动
│   ├── gps.c                  # GPS 软件串口解析
│   ├── lcd.c                  # ST7735R TFT 驱动
│   ├── mpu6050.c              # MPU6050 软件I2C 驱动
│   ├── mqtt.c                 # MQTT 协议实现
│   ├── stm32f0xx_hal_msp.c   # HAL 硬件初始化配置
│   ├── stm32f0xx_it.c        # 中断服务函数
│   └── system_stm32f0xx.c    # 系统时钟配置
├── MDK-ARM/                   # Keil 工程文件
│   ├── ColdChainMonitor.uvprojx   # 主工程文件
│   └── startup_stm32f051x8.s      # 启动文件
├── Proteus/                   # 仿真文件
│   ├── ColdChainMonitor.pdsprj    # Proteus 仿真
│   ├── screen.png                 # 仿真截图
│   └── wiring_diagram.html        # 接线图
├── 上位机/                     # 上位机 (Python)
│   ├── main.py                     # 监控主程序
│   └── requirements.txt            # Python 依赖
├── build_keil.cmd                  # 一键编译脚本
└── README.md                       # 项目说明
```

---

## 二、模块调用关系

```
main.c  [主控调度]
  ├── system_stm32f0xx.c    [系统时钟: SystemInit, SystemCoreClockUpdate]
  ├── stm32f0xx_hal_msp.c   [HAL 外设引脚初始化]
  ├── stm32f0xx_it.c        [中断服务: SysTick, TIM2, USART1]
  │
  ├── dht22.c               [温湿度采集]
  │   └── GPIO PA5 (单总线, open-drain)
  │
  ├── mpu6050.c             [加速度/振动检测]
  │   └── PA2(SCL)/PA3(SDA) — 软件I2C实现, ~100kHz
  │
  ├── gps.c                 [GPS 定位解析]
  │   └── 软件串口 PA0(RX) — 9600bps, NMEA $GPGGA
  │
  ├── esp8266.c             [WiFi 通信]
  │   └── USART1 PA9(TX)/PA10(RX) — 115200bps
  │       └── ISR (stm32f0xx_it.c) 接收数据填充 esp_rx_buf
  │
  ├── mqtt.c                [MQTT 协议封装]
  │   └── 调用 ESP8266_SendData() 发送报文
  │
  ├── alarm.c               [报警逻辑: 蜂鸣器 + RGB LED]
  │   └── 蜂鸣器 PA1 | LED_R PB0 | LED_G PB1 | LED_B PB2
  │
  └── lcd.c                 [TFT 彩屏显示]
      └── SPI1 PA7(MOSI)/PB3(SCK), PA15(CS)/PA8(DC)/PA4(BL)

上位机 (Python)
  └── main.py
       ├── MQTTWorker  ←→ 下位机 (ESP8266+MQTT)
       ├── PyQt5 GUI    ← 实时仪表盘/数据日志/报警/配置
       └── 依赖: paho-mqtt, PyQt5
```

---

## 三、各文件函数清单

### main.c — 主控调度 (heartbeat: PC13 1Hz LED)

| 函数 | 说明 |
|------|------|
| `SystemClock_Config` | 配置 HSI 8MHz → PLLx6 → 48MHz 系统时钟 |
| `MX_GPIO_Init` | GPIO 初始化 (DHT22 PA5 / 蜂鸣器 PA1 / RGB LED PB0-2) |
| `MX_USART1_UART_Init_ESP` | USART1 115200bps → ESP8266 |
| `MX_SPI1_Init` | SPI1 8MHz → ST7735R TFT (PB3 SCK / PA7 MOSI) |
| `MX_TIM2_Init` | TIM2 定时器 → 软件串口波特率时钟 |
| `MX_IWDG_Init` | 独立看门狗初始化 (直接寄存器操作, PR=1, RLR=4095) |
| `GetTick` | 获取系统运行毫秒数 (基于 SysTick) |
| `delay_ms` | 毫秒延时 (HAL_Delay) |
| `delay_us` | 微秒延时 (循环计数, ~12 cycles/us @ 48MHz) |
| `Error_Handler` | 错误处理 (TFT 显示红色, 挂起) |
| `Base_Periph_Init` | 基础外设初始化 (时钟+GPIO+USART+SPI+TIM) |
| `System_Init` | 系统初始化 (外设+传感器+报警+TFT+看门狗) |
| `Sample_Task` | 采样任务: DHT22 + MPU6050 + GPS |
| `MQTT_Task` | MQTT 上报任务: JSON → coldchain/data |
| `Alarm_Task` | 报警检查 |
| `LCD_UpdateDisplay` | TFT 128x128 显示更新 (温湿度/GPS/报警/状态栏) |
| `main` | 主循环: 采样间隔 10s, 看门狗喂狗, 心跳灯 PC13 |

**全局数据结构:**
```c
SensorData_t g_sensor;   // 传感器数据 (温度/湿度/加速度/GPS)
SystemState_t g_state;   // 系统状态 (WiFi/MQTT/报警/运行时间)
```

### esp8266.c — WiFi AT 指令驱动

| 函数 | 说明 |
|------|------|
| `ESP8266_Init` | 复位+AT测试+设为Station模式 (CWMODE=1) |
| `ESP8266_ConnectWiFi` | AT+CWJAP 连接WiFi, 超时 10s |
| `ESP8266_TCP_Connect` | AT+CIPSTART 建立TCP, 超时 8s |
| `ESP8266_SendData` | AT+CIPSEND 发送数据, 等待 `>` 提示符 |
| `ESP8266_CloseTCP` | AT+CIPCLOSE 关闭TCP连接 |
| `ESP8266_Reset` | AT+RST 复位模块 |
| `ESP8266_IsConnected` | 检查 WiFi + TCP 连接状态 |

**数据接收机制:** `stm32f0xx_it.c` 中 USART1_IRQHandler 将数据写入 `esp_rx_buf[]`，遇到 `\n` 时设置 `esp_data_ready = 1`，ESP8266_SendCmd 轮询此标志等待响应。

### mqtt.c — MQTT 3.1.1 协议实现 (QoS 0/1, 纯手动组包)

| 函数 | 说明 |
|------|------|
| `MQTT_EncodeRemLen` | MQTT 可变长度编码 (每组 7bit) |
| `MQTT_SendConnect` | 构建并发送 CONNECT 报文 |
| `MQTT_Connect` | 建立 TCP 连接 + 发送 CONNECT |
| `MQTT_Disconnect` | 发送 DISCONNECT + 关闭 TCP |
| `MQTT_Subscribe` | SUBSCRIBE 报文 (QoS 0) |
| `MQTT_Publish` | PUBLISH 报文 (支持 QoS 0/1) |

### dht22.c — DHT22 温湿度驱动 (GPIO PA5 单总线)

| 函数 | 说明 |
|------|------|
| `DHT22_Init` | GPIO PA5 配置为 open-drain 输出 + 上拉 |
| `DHT22_ReadData` | 发送起始信号 + 读取 40bit 数据, 校验后返回温湿度 |
| `DHT22_ReadByte` | 读取 1 字节 (8bit 时序解析) |

**时序关键:** 主机拉低 20ms 唤醒, 从机响应 80μs 低 + 80μs 高, 数据位 0=28μs, 1=70μs.

### mpu6050.c — MPU6050 加速度驱动 (PA2=SCL, PA3=SDA, 软件 I2C, ~100kHz)

| 函数 | 说明 |
|------|------|
| `MPU6050_Init` | 初始化 I2C 引脚, 唤醒芯片, 设置量程 ±2g |
| `MPU6050_ReadAccel` | 读取 3 轴加速度原始值, 转换为 g (÷16384) |
| `MPU6050_ReadTemperature` | 读取芯片温度 (备用) |
| `i2c_start/stop` | 软件 I2C 起始/停止条件 |
| `i2c_write_byte/read_byte` | 软件 I2C 字节读写 |
| `mpu_write_reg/read_reg` | 写/读 MPU6050 寄存器 |

### gps.c — NEO-6M GPS 驱动 (软件 UART, PA0, 9600bps)

| 函数 | 说明 |
|------|------|
| `GPS_Init` | 配置 PA0 为输入上拉 |
| `GPS_ParseBuffer` | 读取一行 → 查找 $GPGGA → 解析经纬度/高度/卫星数 |
| `GPS_ClearBuffer` | 清除接收缓冲区 |
| `SWUART_ReadLine` | 软件 UART 读取一行 (检测起始位→按位采样) |
| `ParseGPGGA` | 解析 GPGGA 语句字段: UTC/纬度/经度/高度/卫星 |

### lcd.c — ST7735R TFT 彩屏驱动 (SPI1, 128x128)

| 函数 | 说明 |
|------|------|
| `Lcd_Init` | 初始化 TFT (SWRESET/SLPOUT/MADCTL/COLMOD/DISPON) |
| `Lcd_SetRegion` | 设置绘图窗口 (CASET + RASET) |
| `Lcd_Clear` | 清屏 (逐点填充指定颜色) |
| `Lcd_WriteIndex/Data` | SPI 发送命令/数据 |
| `Gui_DrawFont_GBK16` | 显示 8x16 ASCII 字符 (字库来自 font_lcd.h) |
| `Gui_DrawFont_data` | 显示数字字符串 |

### alarm.c — 报警控制 (蜂鸣器 PA1 + RGB LED PB0/PB1/PB2)

| 函数 | 说明 |
|------|------|
| `Alarm_Init` | 初始化 GPIO: 蜂鸣器 PA1 + LED_R PB0 + LED_G PB1 + LED_B PB2 |
| `Alarm_Check` | 检查各参数是否超限 (温度/湿度/加速度阈值) |
| `Alarm_Trigger` | 触发报警: 蜂鸣器响 + 对应颜色 LED + MQTT 上报 |
| `Alarm_Clear` | 清除报警 (恢复正常状态) |
| `Alarm_SetLED` | 设置 RGB LED 颜色 |
| `Alarm_Beep` | 蜂鸣器控制 |

**默认报警阈值:**
| 参数 | 阈值 | 含义 |
|------|------|------|
| TEMP_ALARM_HIGH | 25.0°C | 高温报警 |
| TEMP_ALARM_LOW  | -5.0°C | 低温报警 |
| HUM_ALARM_HIGH  | 85.0%  | 湿度过高 |
| ACCEL_ALARM_THRESH | 2.0g | 碰撞/倾倒检测 |

### stm32f0xx_it.c — 中断服务

| 函数 | 说明 |
|------|------|
| `SysTick_Handler` | 系统滴答 1ms 中断 → HAL_IncTick |
| `TIM2_IRQHandler` | TIM2 定时中断 → 软件串口时序 |
| `USART1_IRQHandler` | USART1 中断 → **ESP8266 数据接收**, 写入 `esp_rx_buf`，遇 `\n` 置 `esp_data_ready` |
| `NMI_Handler / HardFault_Handler` | 异常处理 |

### stm32f0xx_hal_msp.c — HAL 硬件配置

| 函数 | 说明 |
|------|------|
| `HAL_MspInit` | 全局 HAL 初始化 (SYSCFG/PWR 时钟) |
| `HAL_UART_MspInit` | USART1 GPIO 引脚配置 (PA9 AF, PA10 Input) |
| `HAL_SPI_MspInit` | SPI1 GPIO 引脚配置 (PB3 SCK, PA7 MOSI) |
| `HAL_TIM_Base_MspInit` | TIM2 时钟+NIVC 配置 |

### 上位机 main.py (Python) — 远程监控

| 类/函数 | 说明 |
|---------|------|
| `MQTTWorker` | MQTT 客户端线程 (paho-mqtt) |
| `MQTTWorker._on_connect` | 连接成功 → 订阅 coldchain/data, coldchain/alarm |
| `MQTTWorker._on_message` | JSON 解析 → 发射信号 (data_received/alarm_received) |
| `MainWindow` | PyQt5 主窗口 (Fusion 暗色主题) |
| `MainWindow._on_data` | 更新实时数值/日志/缓冲区 |
| `MainWindow._on_alarm` | 报警列表追加 + 自动切换 Tab |
| `MainWindow._apply_config` | 下发采样间隔/温度阈值到设备 |

**上位机依赖:** paho-mqtt, PyQt5

---

## 四、硬件引脚映射

| STM32F051K8 引脚 | 外设 | 功能 | 备注 |
|-----------------|------|------|------|
| PA0 | GPS | 软件串口 RX | 9600bps, 内部上拉 |
| PA1 | 蜂鸣器 | NPN 驱动输出 | 2N2222/S8050 驱动 |
| PA2 | MPU6050 | 软件 I2C SCL | 4.7k 上拉 |
| PA3 | MPU6050 | 软件 I2C SDA | 4.7k 上拉 |
| PA4 | ST7735R | BL 背光控制 | GPIO 输出 |
| PA5 | DHT22 | 单总线 DATA | 4.7k 上拉, open-drain |
| PA7 | ST7735R | SPI1 MOSI | AF0 |
| PA8 | ST7735R | RS/DC 数据/命令 | GPIO 输出 |
| PA9 | ESP8266 | USART1 TX | 115200bps, AF |
| PA10 | ESP8266 | USART1 RX | 115200bps, Input |
| PA15 | ST7735R | CS 片选 | GPIO 输出 |
| PB0 | LED 红 | 报警指示 | 220Ω 限流 |
| PB1 | LED 绿 | 运行指示 | 220Ω 限流 |
| PB2 | LED 蓝 | 上传指示 | 220Ω 限流 |
| PB3 | ST7735R | SPI1 SCK | AF0, 8MHz |
| PC13 | 板载 LED | 心跳灯 1Hz | 低电平点亮 |

---

## 五、关键设计决策

| 决策 | 说明 |
|------|------|
| **软件 I2C (MPU6050)** | F051K8 32 脚封装无 PB6/PB7, 改用 PA2/PA3 软件模拟 I2C, 约 100kHz |
| **GPS 软件串口** | USART1 被 ESP8266 占用, GPS 用 PA0 + TIM2 中断精确定时模拟 9600bps UART |
| **IWDG 直接寄存器操作** | HAL 库对 F0 的 IWDG 支持不完整, 直接操作 IWDG->KR/PR/RLR 寄存器 |
| **TFT 取代 LCD1602** | 128x128 彩屏可显示多行+颜色区分, 信息量远超 2x16 字符屏 |
| **SD 卡已移除** | MQTT 云平台已有持久化, 代码与文件已从工程中清理 |
| **USART1 ISR 直接填充 esp_rx_buf** | 中断中逐字节接收, 遇 `\n` 置 esp_data_ready 标志, 上层轮询等待 |
| **MQTT 手动组包** | 不依赖第三方库, 直接在 MCU 上构建 MQTT 3.1.1 报文, 节省 Flash |

---

## 六、数据流图

```
DHT22 --GPIO PA5--|
MPU6050-I2C PA2/PA3|
GPS --软串口 PA0---|
                   |--STM32F051K8--SPI1---> ST7735R TFT (本地显示)
                   |                 |    (PA7 MOSI, PB3 SCK)
                   |          USART1 PA9/PA10
                   |                 |
                   |             ESP8266
                   |                 |
                   |             MQTT/TCP
                   |                 |
                   +---------> MQTT Broker (broker.emqx.io)
                                    |
                              上位机 Python
                            (PyQt5 GUI 监控)
```

---

*文档版本: V2.0 | 日期: 2026-07-06 | 项目: ColdChainMonitor*