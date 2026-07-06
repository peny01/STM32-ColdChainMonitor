# 基于STM32的冷链运输环境监测系统

## 项目概述
本系统基于STM32F051K8微控制器，集成DHT22温湿度传感器、MPU6050加速度传感器、
NEO-6M GPS定位模块、ESP8266 WiFi模块和ST7735R TFT彩屏，实现对冷链运输过程中
温度、湿度、振动和位置信息的实时采集、本地显示和远程监控。

## 项目结构
```
ColdChainMonitor/
├── MDK-ARM/              # Keil MDK工程文件
│   ├── ColdChainMonitor.uvprojx   # 工程文件
│   └── ColdChainMonitor.uvoptx    # 调试选项
├── Src/                  # STM32固件源码
│   ├── main.c            # 主程序 - 系统初始化、主循环调度
│   ├── dht22.c           # DHT22温湿度传感器驱动(单总线)
│   ├── mpu6050.c         # MPU6050加速度传感器驱动(软件I2C)
│   ├── gps.c             # NEO-6M GPS模块驱动(软件UART+NMEA解析)
│   ├── esp8266.c         # ESP8266 WiFi模块驱动(USART1+AT指令)
│   ├── mqtt.c            # MQTT 3.1.1协议客户端
│   ├── alarm.c           # 声光报警模块
│   ├── lcd.c             # ST7735R TFT彩屏驱动
│   ├── stm32f0xx_hal_msp.c  # HAL硬件初始化配置
│   ├── stm32f0xx_it.c    # 中断服务函数
│   └── system_stm32f0xx.c   # 系统时钟配置
├── Inc/                  # 头文件
├── 上位机/               # Python上位机监控软件
│   ├── main.py           # PyQt5图形界面
│   └── requirements.txt  # Python依赖
├── Proteus/              # Proteus仿真文件
├── Drivers/              # STM32 HAL库驱动
├── Docs/                 # 文档
├── build_keil.cmd        # 一键编译脚本
└── README.md             # 本文件
```

## 硬件引脚连接

| 模块 | 接口 | STM32引脚 |
|------|------|-----------|
| DHT22 | 单总线 | PA5 |
| MPU6050 | 软件I2C | PA2(SCL), PA3(SDA) |
| NEO-6M GPS | 软件UART | PA0(RX), PA1(TX) |
| ESP8266 | USART1 | PA9(TX), PA10(RX) |
| ST7735R TFT | SPI1 | PA7(MOSI), PB3(SCK), PA15(CS), PA8(DC), PA4(BL) |
| 蜂鸣器 | GPIO | PA1 |
| RGB LED | GPIO | PB0(R), PB1(G), PB2(B) |

## 编译与烧录

### Keil MDK编译
1. 安装Keil MDK v5 + STM32F0xx_DFP Pack
2. 双击 `build_keil.cmd` 一键编译
3. 或打开 `MDK-ARM/ColdChainMonitor.uvprojx` 手动编译
4. 使用ST-Link/JLink烧录

## 上位机启动
```bash
cd 上位机
pip install -r requirements.txt
python main.py
```

## 功能说明
- 温湿度采集(DHT22): -40~80°C, 0~100%RH
- 振动检测(MPU6050): 三轴加速度 ±2g
- GPS定位(NEO-6M): 精度<2.5m
- 数据上传(ESP8266+MQTT): JSON格式, MQTT QoS 1
- 本地显示(ST7735R TFT): 128x128彩色, 温度/湿度/GPS/状态
- 声光报警: RGB LED + 蜂鸣器
- 上位机监控: 实时数据、报警管理、参数配置