' =============================================
' ColdChainMonitor - Proteus 自动原理图生成脚本
' STM32F051K8 + LCD1602 + DHT22 + MPU6050 + GPS + ESP8266
' 
' 使用方法:
'   1. 打开 Proteus 8 Professional
'   2. 新建工程 (File -> New Project -> Default)
'   3. 打开原理图编辑 (点击 Schematic Capture)
'   4. 菜单 Tools -> Run Script (或 Alt+V -> Run Script)
'   5. Browse -> 选择此文件 -> Run
' =============================================

Sub CreateSchematic()
    Dim doc
    Set doc = ISIS.Schematic
    
    ' 清空当前原理图
    doc.Clear
    
    ' ========================================
    ' 放置元件 - X,Y 坐标单位: mils (1 inch = 1000 mils)
    ' ========================================
    
    ' --- STM32F051K8 (居中) ---
    Dim mcu
    Set mcu = doc.CreateComponent("MCU_IC_ST", "STM32F051K8", "U1", 4000, 3000, 0)
    
    ' --- LCD1602 (右侧) ---
    Dim lcd
    Set lcd = doc.CreateComponent("DISPLAY (LCD)", "LM016L", "LCD1", 7000, 3000, 0)
    
    ' --- DHT22 (左侧) ---
    Dim dht
    Set dht = doc.CreateComponent("SENSOR", "DHT22", "U2", 1000, 2000, 0)
    
    ' --- MPU6050 (左下) ---
    Dim mpu
    Set mpu = doc.CreateComponent("IC", "MPU6050", "U3", 1000, 4000, 0)
    
    ' --- GPS (左上 COMPIM) ---
    Dim gps
    Set gps = doc.CreateComponent("SIMULATION", "COMPIM", "GPS1", 1000, 1000, 0)
    gps.SetProperty("BAUDRATE", "9600")
    gps.SetProperty("BITS", "8")
    
    ' --- ESP8266 (左中 COMPIM) ---
    Dim esp
    Set esp = doc.CreateComponent("SIMULATION", "COMPIM", "ESP1", 1000, 3000, 0)
    esp.SetProperty("BAUDRATE", "115200")
    esp.SetProperty("BITS", "8")
    
    ' --- RGB LED 红 (右侧上方) ---
    Dim ledR
    Set ledR = doc.CreateComponent("OPTOELECTRONIC", "LED-RED", "LED1", 6500, 1500, 0)
    
    ' --- RGB LED 绿 ---
    Dim ledG
    Set ledG = doc.CreateComponent("OPTOELECTRONIC", "LED-GREEN", "LED2", 6500, 2000, 0)
    
    ' --- RGB LED 蓝 ---
    Dim ledB
    Set ledB = doc.CreateComponent("OPTOELECTRONIC", "LED-BLUE", "LED3", 6500, 2500, 0)
    
    ' --- 心跳 LED 黄 ---
    Dim ledHB
    Set ledHB = doc.CreateComponent("OPTOELECTRONIC", "LED-YELLOW", "LED4", 6500, 4500, 0)
    
    ' --- 蜂鸣器 ---
    Dim buzzer
    Set buzzer = doc.CreateComponent("ACTIVE", "BUZZER", "BZ1", 5500, 5000, 0)
    
    ' --- NPN 三极管 (蜂鸣器驱动) ---
    Dim npn
    Set npn = doc.CreateComponent("TRANSISTOR", "2N2222", "Q1", 5500, 5500, 0)
    
    ' ========================================
    ' 电阻
    ' ========================================
    
    ' LED 限流电阻 220R x4
    Dim r1, r2, r3, r4
    Set r1 = doc.CreateComponent("RES", "220", "R1", 6800, 1500, 0)
    Set r2 = doc.CreateComponent("RES", "220", "R2", 6800, 2000, 0)
    Set r3 = doc.CreateComponent("RES", "220", "R3", 6800, 2500, 0)
    Set r4 = doc.CreateComponent("RES", "220", "R4", 6800, 4500, 0)
    
    ' I2C 上拉 4.7k x2
    Dim r5, r6
    Set r5 = doc.CreateComponent("RES", "4.7k", "R5", 2500, 1800, 0)
    Set r6 = doc.CreateComponent("RES", "4.7k", "R6", 2500, 2200, 0)
    
    ' ESP8266 上拉 10k x4
    Dim r7, r8, r9, r10
    Set r7 = doc.CreateComponent("RES", "10k", "R7", 2500, 2800, 0)
    Set r8 = doc.CreateComponent("RES", "10k", "R8", 2500, 3200, 0)
    Set r9 = doc.CreateComponent("RES", "10k", "R9", 2500, 3600, 0)
    Set r10 = doc.CreateComponent("RES", "10k", "R10", 2500, 4000, 0)
    
    ' NPN 基极电阻 10k
    Dim r11
    Set r11 = doc.CreateComponent("RES", "10k", "R11", 5200, 5500, 0)
    
    ' ========================================
    ' 电源终端
    ' ========================================
    
    Dim pwr3v3, pwr5v, gnd1, gnd2, gnd3
    
    Set pwr3v3 = doc.CreatePowerTerminal("DC", "3.3V", 3500, 500)
    Set pwr5v = doc.CreatePowerTerminal("DC", "5V", 7500, 500)
    Set gnd1 = doc.CreatePowerTerminal("DC", "GROUND", 3500, 6000)
    Set gnd2 = doc.CreatePowerTerminal("DC", "GROUND", 7500, 6000)
    
    ' ========================================
    ' 网络标签 (Net Labels)
    ' ========================================
    
    ' 用 Net Label 连接引脚，避免大量布线
    ' PA1 -> LCD RS
    doc.CreateNetLabel("LCD_RS", 4500, 2800)
    doc.CreateNetLabel("LCD_RS", 7200, 2600)
    
    ' PA2 -> LCD EN
    doc.CreateNetLabel("LCD_EN", 4500, 3000)
    doc.CreateNetLabel("LCD_EN", 7200, 2400)
    
    ' PA3 -> LCD D4
    doc.CreateNetLabel("LCD_D4", 4500, 3200)
    doc.CreateNetLabel("LCD_D4", 7200, 1800)
    
    ' PA4 -> LCD D5
    doc.CreateNetLabel("LCD_D5", 4500, 3400)
    doc.CreateNetLabel("LCD_D5", 7200, 1600)
    
    ' PA8 -> LCD D6
    doc.CreateNetLabel("LCD_D6", 4800, 2600)
    doc.CreateNetLabel("LCD_D6", 7200, 1400)
    
    ' PA15 -> LCD D7
    doc.CreateNetLabel("LCD_D7", 4800, 2800)
    doc.CreateNetLabel("LCD_D7", 7200, 1200)
    
    ' PA0 -> GPS TX
    doc.CreateNetLabel("GPS_TX", 4200, 2400)
    doc.CreateNetLabel("GPS_TX", 1200, 1400)
    
    ' PA9 -> ESP8266 RXD
    doc.CreateNetLabel("ESP_RXD", 4200, 3200)
    doc.CreateNetLabel("ESP_RXD", 1200, 2800)
    
    ' PA10 -> ESP8266 TXD
    doc.CreateNetLabel("ESP_TXD", 4200, 3400)
    doc.CreateNetLabel("ESP_TXD", 1200, 3000)
    
    ' PB6 -> DHT22 DATA + MPU6050 SCL
    doc.CreateNetLabel("SDA_SCL", 5000, 2600)
    doc.CreateNetLabel("SDA_SCL", 1200, 1800)
    doc.CreateNetLabel("SDA_SCL", 1200, 3800)
    
    ' PB7 -> MPU6050 SDA
    doc.CreateNetLabel("I2C_SDA", 5000, 2800)
    doc.CreateNetLabel("I2C_SDA", 1200, 4000)
    
    ' PB0 -> LED RED (via R)
    doc.CreateNetLabel("LED_R", 5000, 2200)
    doc.CreateNetLabel("LED_R", 6500, 1700)
    
    ' PB1 -> LED GREEN (via R)
    doc.CreateNetLabel("LED_G", 5000, 2000)
    doc.CreateNetLabel("LED_G", 6500, 2200)
    
    ' PB2 -> LED BLUE (via R)
    doc.CreateNetLabel("LED_B", 5200, 2400)
    doc.CreateNetLabel("LED_B", 6500, 2700)
    
    ' PB4 -> NPN Base (via R)
    doc.CreateNetLabel("BUZ_CTL", 5200, 2200)
    doc.CreateNetLabel("BUZ_CTL", 5000, 5500)
    
    ' PC13 -> Heartbeat LED
    doc.CreateNetLabel("HB_LED", 5200, 3400)
    doc.CreateNetLabel("HB_LED", 6500, 4700)
    
    MsgBox "ColdChainMonitor 原理图已生成!" & vbCrLf & vbCrLf & _
           "请检查以下连接:" & vbCrLf & _
           "- LCD1602: PA1->RS, PA2->EN, PA3->D4, PA4->D5, PA8->D6, PA15->D7" & vbCrLf & _
           "- DHT22: PB6 (DATA)" & vbCrLf & _
           "- MPU6050: PB6 (SCL), PB7 (SDA)" & vbCrLf & _
           "- GPS: PA0 (RX)" & vbCrLf & _
           "- ESP8266: PA9 (TX), PA10 (RX)" & vbCrLf & _
           "- LED: PB0(R), PB1(G), PB2(B), PC13(HB)" & vbCrLf & _
           "- BUZZER: PB4->R->NPN Base", _
           vbInformation, "ColdChainMonitor Schematic"
    
End Sub

Call CreateSchematic
