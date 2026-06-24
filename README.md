# Example STM32F407ZGT6

基于正点原子探索者V3开发板的 STM32F407ZGT6 裸机项目，使用 STM32CubeMX HAL 库 + CMake 构建，采用结构体+函数指针的模块化封装风格。

## 硬件平台

| 项目 | 说明 |
|------|------|
| 开发板 | 正点原子 探索者V3 |
| MCU | STM32F407ZGT6 (Cortex-M4F, 168 MHz) |
| 调试器 | DAPLink |
| 时钟 | HSI (16 MHz) → PLL → SYSCLK 168 MHz |

## 目录结构

```
├── Core/               # CubeMX 生成的 HAL 初始化代码
│   ├── Inc/            # 头文件 (main.h, gpio.h, adc.h, usart.h, fsmc.h, spi.h, i2c.h, ...)
│   └── Src/            # 源文件 (main.c, gpio.c, adc.c, usart.c, fsmc.c, spi.c, i2c.c, ...)
├── Drivers/            # CMSIS + STM32F4xx HAL 驱动库
├── Modules/            # 自定义模块封装（外设驱动层）
│   ├── Inc/            # 模块头文件
│   └── Src/            # 模块源文件
├── GUI/                # GUI 组件（按钮、复选框、滑块）
│   ├── Inc/            # GUI 头文件
│   └── Scr/            # GUI 源文件
├── Functions/          # 应用示例
│   ├── Inc/            # 应用头文件
│   └── Scr/            # 应用源文件
├── Tools/              # 辅助工具 (字库烧录脚本等)
├── cmake/              # CMake 工具链文件 (含 stm32cubemx 子目录)
├── Docs/               # 开发板文档 (不纳入版本管理)
├── CMakeLists.txt      # 顶层构建文件
└── daplink.cfg         # OpenOCD DAPLink 配置
```

## 已封装模块

所有模块以结构体封装外设状态，函数接收结构体指针操作。多数模块依赖 `soft_timer` 提供定时能力，`hw_timer`、I2C、SPI 等总线层模块独立。

### SoftTimer — 软件定时器

基于 `HAL_GetTick()` 的轻量级毫秒定时器，不占用硬件定时器资源。

```c
SOFT_TIMER t;
SoftTimer_Start(&t, 500);
if (SoftTimer_Expired(&t)) { /* 每500ms执行一次 */ }
```

| 函数 | 说明 |
|------|------|
| `SoftTimer_Start(t, ms)` | 设置间隔并记录起始时刻 |
| `SoftTimer_Reset(t)` | 重置起始时刻 |
| `SoftTimer_Expired(t)` | 到期检查，到期后自动重置 |
| `SoftTimer_Elapsed(t)` | 仅检查到期，不重置 |

### LED — 板载LED控制

控制 LED0 (PF9) 和 LED1 (PF10)，支持流水灯效果。

```c
Leds_Init();
Led_On(&led0);
Led_Flow_Init(200);  // 200ms 流水灯
while (1) Led_Flow();
```

| 函数 | 说明 |
|------|------|
| `Led_On/Off/Toggle(led)` | 单灯控制 |
| `Led_OnAll/OffAll/ToggleAll()` | 全局控制 |
| `Led_Flow_Init(ms)` | 初始化流水灯 |
| `Led_Flow_On/Off()` | 开启 / 关闭流水灯 |
| `Led_Flow()` | 流水灯状态机（需周期性调用） |

### Buzzer — 蜂鸣器

控制 PF8 蜂鸣器，支持定时自动关闭的蜂鸣功能。

```c
Buzzer_Init(&buzzer);
Buzzer_Beep(&buzzer, 200);  // 蜂鸣200ms
while (1) Buzzer_Loop(&buzzer);  // 后台处理，到期自动关闭
```

| 函数 | 说明 |
|------|------|
| `Buzzer_On/Off/Toggle(bzr)` | 开关蜂鸣器 |
| `Buzzer_Beep(bzr, ms)` | 蜂鸣指定毫秒后自动关闭 |
| `Buzzer_Loop(bzr)` | 后台处理，需在主循环中调用 |

### Key_Press — 按键事件

4个按键 (KEY0/KEY1/KEY2/KEY_UP) 的软件消抖与事件解码，支持短按、长按、连发。

```c
Key_Scan(&key0);
switch (key0.event) {
    case KEY_EVENT_SHORT:  /* 短按 */ break;
    case KEY_EVENT_LONG:   /* 长按 (>500ms) */ break;
    case KEY_EVENT_REPEAT: /* 连发 (每200ms) */ break;
}
```

消抖 20ms，长按阈值 500ms，连发间隔 200ms。`Key_Scan()` 需周期性调用。

### Light_Sensor — 光敏传感器

读取 PF7 光敏电阻数字量，判断环境亮暗。

```c
LightSensor_Init(&light_sensor);
LightSensor_Scan(&light_sensor);
if (LightSensor_IsDark(&light_sensor)) { /* 暗 */ }
if (LightSensor_IsLight(&light_sensor)) { /* 亮 */ }
```

| 函数 | 说明 |
|------|------|
| `LightSensor_Init(lts)` | 初始化并首次扫描 |
| `LightSensor_Scan(lts)` | 读取当前状态 |
| `LightSensor_IsDark(lts)` | 是否处于暗环境 |
| `LightSensor_IsLight(lts)` | 是否处于亮环境 |

### Touch_Pad — 电容触摸

通过 RC 充电延迟法实现的电容触摸感应 (PA5)，无需 ADC。先放电，再将引脚设为浮空输入，计数至充电完成，触摸时电容增大导致充电时间变长。自适应基线校准。

```c
TouchPad_Init(&touch_pad);
TouchPad_Scan(&touch_pad);
if (TouchPad_IsPressed(&touch_pad)) { /* 被触摸 */ }
```

### Serial — 串口收发

通用串口封装，不写死 USART 外设，传入 `Serial_t` 结构体即可驱动任意 USART。中断接收 + 512 字节环形缓冲区。全局实例：`serial1` (USART1, PA9/PA10, 115200 8N1), `serial3` (USART3, PB10/PB11)。

```c
Serial_Init(&serial1);
Serial_SendString(&serial1, "Hello\r\n");
Serial_PrintU32(&serial1, 12345);

if (Serial_Available(&serial1)) {
    uint8_t ch = Serial_ReadByte(&serial1);
}
```

| 函数 | 说明 |
|------|------|
| `Serial_Init(s)` | 清空缓冲区，启动中断接收 |
| `Serial_SendByte/Data/String(s, ...)` | 阻塞发送 |
| `Serial_PrintU32(s, val)` | 打印32位无符号整数 |
| `Serial_Println(s, str)` | 发送字符串 + 换行 |
| `Serial_Available(s)` | 可读字节数 |
| `Serial_ReadByte(s)` | 读取一个字节 |

### ADC — 模拟采集

ADC1 单通道采集封装，支持原始值、电压值和滑动平均。12 位分辨率，3.3V 参考电压。全局实例：`adc1_ch5` (绑定 `hadc1`, ADC_CHANNEL_5 / PA5)。

```c
ADC_Init(&adc1_ch5);

uint16_t raw = ADC_ReadRaw(&adc1_ch5);          // 0~4095
uint16_t mv  = ADC_ReadVoltage(&adc1_ch5);      // 0~3300 mV
uint16_t avg = ADC_ReadAvg(&adc1_ch5, 10);      // 10次采样平均
```

| 函数 | 说明 |
|------|------|
| `ADC_Init(ch)` | 初始化，读取首值填入 raw/voltage_mv/avg_raw |
| `ADC_ReadRaw(ch)` | 单次采集，返回原始值 0~4095 |
| `ADC_ReadVoltage(ch)` | 单次采集，返回电压值 (mV) |
| `ADC_ReadAvg(ch, times)` | 采集 times 次，返回平均原始值 |

### HW_Timer — 硬件定时器

基于 TIM6 的硬件微秒级定时器，用于精确延时和计时。TIM6 挂载 APB1 (42 MHz ×2 = 84 MHz 定时器时钟)，预分频 168-1 → 计数频率 500 kHz (每 tick = 2 μs)，周期 1000-1。

```c
HwTimer_Init(&hwtim6);

HwTimer_DelayUs(&hwtim6, 50);   // 阻塞延时 50μs
HwTimer_DelayMs(&hwtim6, 10);   // 阻塞延时 10ms

// 用于自定义超时控制
HwTimer_Reset(&hwtim6);
HwTimer_Start(&hwtim6);
// ... 等待某事件 ...
if (HwTimer_GetCount(&hwtim6) > 5000) { /* 超时 */ }
HwTimer_Stop(&hwtim6);
```

| 函数 | 说明 |
|------|------|
| `HwTimer_Init(t)` | 根据 APB1 时钟和预分频计算实际 freq_hz |
| `HwTimer_DelayUs(t, us)` | 阻塞延时微秒（16 位计数，单次最长 ~131ms） |
| `HwTimer_DelayMs(t, ms)` | 阻塞延时毫秒 |
| `HwTimer_Start(t)` | 启动计数器 |
| `HwTimer_Stop(t)` | 停止计数器 |
| `HwTimer_Reset(t)` | 清零计数器 |
| `HwTimer_GetCount(t)` | 读取当前计数值 |

### OLED — 0.96寸显示屏

ATK-MD0096 0.96" OLED 显示屏 (SSD1306)，8080 并行接口，128×64 分辨率。内部维护 1 KB 帧缓存 (`g_gram[8][128]`)，所有绘制操作先写入缓存，调用 `OLED_Update()` 后整屏刷新。

```c
OLED_Init();
OLED_ShowString(0, 0, "Hello", 16);
OLED_Update();
```

**API**

| 函数 | 说明 |
|------|------|
| `OLED_Init()` | 硬件复位 + SSD1306 寄存器初始化，清屏 |
| `OLED_Clear()` | 帧缓存清零（不刷新到屏幕） |
| `OLED_Update()` | 将帧缓存全量写入 SSD1306 |
| `OLED_DrawPoint(x, y, dot)` | 绘制 / 擦除一个点 |
| `OLED_ShowChar(x, y, ch, size)` | 显示一个字符，`size` = 8/12/16 |
| `OLED_ShowString(x, y, str, size)` | 显示字符串，自动换行 |
| `OLED_ShowNum(x, y, num, len, size)` | 显示数字（高位补空格） |
| `OLED_DrawLine(x1, y1, x2, y2)` | Bresenham 直线 |
| `OLED_DrawRect(x, y, w, h)` | 空心矩形 |
| `OLED_DrawFillRect(x, y, w, h)` | 实心矩形 |
| `OLED_DrawCircle(x, y, r)` | 空心圆 |
| `OLED_DrawFillCircle(x, y, r)` | 实心圆 |

**例1：基本文字显示**

```c
OLED_Init();

OLED_ShowString(0, 0,  "STM32F407", 16);   // 8×16 字体
OLED_ShowString(0, 16, "OLED Demo", 12);    // 6×12 字体
OLED_ShowNum(0, 32, 12345, 5, 16);          // 显示 "12345"
OLED_Update();
```

**例2：传感器数值实时刷新**

```c
OLED_Init();
ADC_Init(&adc1_ch5);

while (1) {
    uint16_t mv = ADC_ReadVoltage(&adc1_ch5);

    OLED_Clear();
    OLED_ShowString(0, 0,  "ADC Voltage", 12);
    OLED_ShowNum(0, 20, mv, 4, 16);          // 显示 mV
    OLED_ShowString(30, 20, "mV", 12);
    OLED_Update();

    HAL_Delay(500);
}
```

**例3：简单图形**

```c
OLED_Init();

OLED_DrawRect(10, 10, 50, 30);               // 空心矩形
OLED_DrawFillRect(70, 10, 30, 30);            // 实心矩形
OLED_DrawCircle(30, 50, 15);                  // 空心圆
OLED_DrawFillCircle(90, 50, 10);              // 实心圆
OLED_DrawLine(0, 0, 127, 63);                 // 对角线
OLED_Update();
```

### TFT LCD — 彩色液晶屏

FSMC 驱动的 MCU 屏，支持 ILI9341 / ST7789 / ST7796 / NT35310 / NT35510 / ILI9806 / SSD1963 多种驱动 IC，通过读取 ID 自动适配。含内置 6×8 英文 ASCII 字库，支持 2×、3× 整数缩放，文字绘制自动用 `tft_bg_color` 填充背景。全局实例：`tftlcd`。

```c
TFTLCD_Init();                           // 自动检测 IC，适配分辨率
TFTLCD_Clear(TFT_BLACK);                // 全屏填充

TFTLCD_ShowString(0, 0, "Hello TFT", 16, TFT_WHITE);
TFTLCD_Printf(0, 40, 12, TFT_YELLOW, "ADC: %d mV", adc_val);
TFTLCD_BackLight(1);                     // 背光开关
```

| 函数 | 说明 |
|------|------|
| `TFTLCD_Init()` | 自动检测驱动 IC，适配分辨率，默认竖屏 |
| `TFTLCD_Clear(color)` | 全屏填充单色 |
| `TFTLCD_DisplayDir(dir)` | 切换显示方向 (0=竖屏, 1=横屏) |
| `TFTLCD_SetCursor(x, y)` | 设置光标位置 |
| `TFTLCD_SetWindow(xs, ys, xe, ye)` | 设置填充窗口 |
| `TFTLCD_DrawPoint(x, y, color)` | 画点 |
| `TFTLCD_Fill(xs, ys, xe, ye, color)` | 矩形填充 |
| `TFTLCD_DrawLine(x1, y1, x2, y2, color)` | Bresenham 直线 |
| `TFTLCD_DrawRect(x, y, w, h, color)` | 空心矩形 |
| `TFTLCD_DrawFillRect(x, y, w, h, color)` | 实心矩形 |
| `TFTLCD_DrawCircle(cx, cy, r, color)` | 空心圆 |
| `TFTLCD_DrawFillCircle(cx, cy, r, color)` | 实心圆 |
| `TFTLCD_ShowChar(x, y, ch, size, color)` | 字符 (size: 8/12/16，整数缩放) |
| `TFTLCD_ShowString(x, y, str, size, color)` | 字符串，自动换行 |
| `TFTLCD_ShowNum(x, y, num, len, size, color)` | 数字（高位补空格） |
| `TFTLCD_Printf(x, y, size, color, fmt, ...)` | 格式化打印（底层 `vsnprintf`） |
| `TFTLCD_BackLight(on)` | 背光开关（PB15，1=开） |
| `TFTLCD_SetBackColor(bg)` | 设置全局背景色并清屏 |

**RGB565 预定义颜色**

`TFT_WHITE`, `TFT_BLACK`, `TFT_RED`, `TFT_GREEN`, `TFT_BLUE`, `TFT_YELLOW`, `TFT_CYAN`, `TFT_MAGENTA`, `TFT_GRAY`, `TFT_ORANGE`, `TFT_PURPLE`, `TFT_DARKBLUE`, `TFT_LIGHTBLUE`, `TFT_LIGHTGREEN`, `TFT_BROWN`

**XPT2046 触摸（已合并到 tft_lcd.h）**

XPT2046 电阻触摸控制器，软件 SPI。PEN 中断检测触摸状态，5 次采样取平均，四点 Y 分段 + 线性 X 校准。全局实例：`touch`。

```c
Touch_Init();
while (1) {
    if (Touch_Scan()) {
        uint16_t x, y;
        Touch_GetXY(&x, &y);   // 已映射到 LCD 分辨率
    }
}
```

| 函数 | 说明 |
|------|------|
| `Touch_Init()` | 初始化，清零状态 |
| `Touch_Scan()` | 扫描，返回 1=检测到新触摸数据 |
| `Touch_GetXY(x, y)` | 获取校准后坐标 |
| `Touch_IsPressed()` | 当前是否正在触摸 |

### GUI — 触屏控件

基于 TFT LCD + Touch 的轻量 GUI 组件，包含按钮、复选框、水平滑块。所有控件由触摸坐标驱动，在 `Touch_Scan()` 检测到触摸后分发事件。

**Button — 按钮**

3D 边框效果，触摸按下/松开切换视觉凹陷。

```c
Button btn = {
    .x = 20, .y = 60, .w = 120, .h = 40,
    .text = "LED0", .color = TFT_WHITE, .bg_color = TFT_BLUE,
    .on_click = on_led0_click,
};
Button_Draw(&btn);
// 在触摸扫描循环中:
if (Touch_Scan()) { Touch_GetXY(&x, &y); Button_Check(&btn, x, y); }
```

| 函数 | 说明 |
|------|------|
| `Button_Draw(btn)` | 绘制按钮（含 3D 边框） |
| `Button_Check(btn, tx, ty)` | 检测触摸坐标，命中时自动绘制凹陷效果并回调 `on_click` |

**CheckBox — 复选框**

20×20 方框 + 右侧文字，点击切换选中/取消状态。

```c
CheckBox cb = {
    .x = 20, .y = 120, .text = "Flow LED",
    .color = TFT_WHITE, .bg_color = TFT_BLACK,
    .on_change = on_flow_change,       // 回调参数: checked (0/1)
};
CheckBox_Draw(&cb);
// 触摸分发: CheckBox_Check(&cb, x, y);
```

| 函数 | 说明 |
|------|------|
| `CheckBox_Draw(cb)` | 绘制复选框 |
| `CheckBox_Check(cb, tx, ty)` | 检测触摸，命中时翻转状态并回调 `on_change(checked)` |
| `CheckBox_SetChecked(cb, val)` | 外部设置选中状态并重绘 |

**Slider — 滑块**

0~100 水平滑块，触摸拖动实时更新。

```c
Slider s = {
    .x = 20, .y = 160, .w = 200,
    .color = TFT_RED, .bg_color = TFT_GRAY,
    .on_change = on_brightness_change,  // 回调参数: value (0~100)
};
Slider_Draw(&s);
// 触摸分发: Slider_Check(&s, x, y);
```

| 函数 | 说明 |
|------|------|
| `Slider_Draw(s)` | 绘制滑块（滑槽 + 填充条 + 圆钮） |
| `Slider_Check(s, tx, ty)` | 检测触摸拖动，更新 value 并回调 `on_change(value)` |
| `Slider_SetValue(s, val)` | 外部设置值 (0~100) 并重绘 |

### I2C_Bus — I2C 总线层

不写死设备地址的 I2C 总线封装，支持寄存器读/写和无寄存器读/写，带设备探测功能。全局实例：`i2c1`。

```c
I2C_Bus_Init(&i2c1);
if (I2C_Bus_Probe(&i2c1, 0x50)) { /* 设备在线 */ }

uint8_t buf[8];
I2C_Bus_MemRead(&i2c1, 0x50, 0x00, buf, 8);   // 从寄存器 0x00 起读 8 字节
I2C_Bus_MemWrite(&i2c1, 0x50, 0x00, buf, 8);  // 从寄存器 0x00 起写 8 字节
```

| 函数 | 说明 |
|------|------|
| `I2C_Bus_Init(bus)` | 初始化总线 |
| `I2C_Bus_Probe(bus, dev)` | 扫描设备，返回 0=无响应, 1=在线 |
| `I2C_Bus_MemRead(bus, dev, reg, buf, len)` | 寄存器读（1 字节寄存器地址） |
| `I2C_Bus_MemWrite(bus, dev, reg, buf, len)` | 寄存器写 |
| `I2C_Bus_Read(bus, dev, buf, len)` | 无寄存器地址读 |
| `I2C_Bus_Write(bus, dev, buf, len)` | 无寄存器地址写 |

### OneWire — 单总线层

Dallas OneWire 底层协议封装，不写死引脚，传入结构体即可驱动任意 GPIO。精确微秒延时（关中断），支持复位、位读写、字节读写。全局实例：`ow1`。

```c
OneWire_Init(&ow1);
if (OneWire_Reset(&ow1)) {           // 检测设备是否存在
    OneWire_WriteByte(&ow1, 0xCC);   // SKIP ROM
    OneWire_WriteByte(&ow1, 0x44);   // 启动转换
}
```

| 函数 | 说明 |
|------|------|
| `OneWire_Init(ow)` | 初始化 GPIO |
| `OneWire_Reset(ow)` | 复位脉冲，返回 1=有设备应答 |
| `OneWire_WriteBit(ow, bit)` | 写一个位 |
| `OneWire_ReadBit(ow)` | 读一个位 |
| `OneWire_WriteByte(ow, byte)` | 写一个字节 |
| `OneWire_ReadByte(ow)` | 读一个字节 |

### DS18B20 — 温度传感器

基于 OneWire 总线的数字温度传感器，精度 12 位 (0.0625°C)。全局实例：`ds18b20` (绑定 `ow1`)。

```c
DS18B20_Init(&ds18b20);
DS18B20_StartConvert(&ds18b20);      // 启动温度转换
HAL_Delay(750);                       // 等待转换完成（最长 750ms）
float t = DS18B20_ReadTemp(&ds18b20); // 读取温度 °C
```

| 函数 | 说明 |
|------|------|
| `DS18B20_Init(s)` | 初始化传感器 |
| `DS18B20_StartConvert(s)` | 启动温度转换（需等待 750ms 后读取） |
| `DS18B20_ReadTemp(s)` | 读取温度值 (°C)，返回 float |

### DHT11 — 温湿度传感器

自定义单总线协议的温湿度传感器，单次读取返回湿度和温度。全局实例：`dht11`。

```c
DHT11_Init(&dht11);
if (DHT11_Read(&dht11)) {            // 返回 1=成功
    uint8_t hum = dht11.humidity;    // 湿度 %RH
    uint8_t tmp = dht11.temperature; // 温度 °C
}
```

| 函数 | 说明 |
|------|------|
| `DHT11_Init(dht)` | 初始化 GPIO |
| `DHT11_Read(dht)` | 读取一次，返回 1=成功，数据存入 `humidity` / `temperature` |

### W25QXX — SPI Flash

W25Q128 (16 MB) SPI NOR Flash 驱动。支持读 ID、任意地址读取、扇区擦除 (4 KB)、页写入 (256 B)、整片擦除、掉电/唤醒。含 SPI 读写底层。全局实例：`w25q128`。

```c
W25QXX_Init(&w25q128);
uint16_t id = W25QXX_ReadID(&w25q128);    // 0xEF17

uint8_t buf[256];
W25QXX_Read(&w25q128, 0x000000, buf, 256);  // 读 256 字节

W25QXX_EraseSector(&w25q128, 0x000000);     // 擦除扇区 0
W25QXX_WritePage(&w25q128, 0x000000, buf, 256); // 写一页
W25QXX_Write(&w25q128, 0x000000, buf, 1024);    // 自动跨页写入
```

| 函数 | 说明 |
|------|------|
| `W25QXX_Init(f)` | 初始化 SPI + CS 引脚 |
| `W25QXX_ReadID(f)` | 读取芯片 ID (W25Q128 = 0xEF17) |
| `W25QXX_Read(f, addr, buf, len)` | 任意地址读取 |
| `W25QXX_EraseSector(f, addr)` | 擦除 4 KB 扇区 |
| `W25QXX_WritePage(f, addr, buf, len)` | 页写入（≤256 字节，不可跨页） |
| `W25QXX_Write(f, addr, buf, len)` | 自动处理跨页/擦除的写入 |
| `W25QXX_EraseAll(f)` | 整片擦除 |
| `W25QXX_PowerDown(f)` / `WakeUp(f)` | 掉电/唤醒 |

预定义宏：`W25QXX_SECTOR_SIZE` (4096), `W25QXX_BLOCK_SIZE` (65536), `W25QXX_CAPACITY` (16 MB)

### FontFlash — 中文字库

从 SPI Flash 中读取 GBK 编码的中文字模并绘制到 TFT LCD。支持 16×16 和 24×24 两种字号。字库通过 Python 工具 (`Tools/gbk_font_tool.py`) 经串口烧录到 Flash。同时支持 UTF-8 字符串直接绘制（内部转为 GBK 再查表）。

```c
FontFlash_Init();

// GBK 编码字符串
uint8_t gbk_str[] = {0xBF, 0xAA, 0xCA, 0xBC, 0x00};  // "开心"
FontFlash_ShowString16(0, 0, gbk_str, TFT_RED, TFT_BLACK);

// UTF-8 字符串（自动处理中英混排）
FontFlash_ShowString_UTF8(0, 40, "你好世界ABC", 16, TFT_WHITE);
```

| 函数 | 说明 |
|------|------|
| `FontFlash_Init()` | 初始化 W25QXX（由内部调用） |
| `FontFlash_ShowChar16(x, y, hi, lo, color, bg)` | 16×16 单字绘制（GBK 编码） |
| `FontFlash_ShowChar24(x, y, hi, lo, color, bg)` | 24×24 单字绘制 |
| `FontFlash_ShowString16(x, y, gbk_str, color, bg)` | GBK 字符串绘制 |
| `FontFlash_ShowString24(x, y, gbk_str, color, bg)` | GBK 字符串绘制 (24×24) |
| `FontFlash_ShowString_UTF8(x, y, utf8_str, size, color)` | UTF-8 字符串，size≤16→16×16, size≥24→24×24 |
| `FontFlash_RecvProc(ser)` | 串口接收字库数据（主循环中调用） |
| `FontFlash_GetInfo(total, ok, bad)` | 获取烧录统计信息 |

烧录字库流程：PC 端运行 `python Tools/gbk_font_tool.py` → 串口发送 → MCU 在主循环调用 `FontFlash_RecvProc(&serial1)` 接收写入 Flash。

### Servo — 舵机驱动

基于 HW_Timer (TIM6) 的软件 PWM 驱动，不占用硬件 PWM 通道。可挂任意空闲 GPIO，支持 0~180° 角度。20ms 周期 (50Hz)，脉宽 0.5~2.5ms 对应 0~180°。全局实例：`servo1`。

```c
Servo_Init(&servo1);
Servo_SetAngle(&servo1, 90);   // 转到 90°

while (1) {
    Servo_Update(&servo1);     // 必须在主循环中高频调用，生成 PWM 波形
}
```

| 函数 | 说明 |
|------|------|
| `Servo_Init(s)` | 初始化 GPIO 为推挽输出 |
| `Servo_SetAngle(s, angle)` | 设置目标角度 0~180 |
| `Servo_Update(s)` | 主循环中调用，位脉冲生成 PWM 波形 |

> **注意**: `Servo_Update()` 用 `HwTimer_DelayUs` 阻塞产生 ±1μs 级精度的脉宽，会短暂阻塞其他任务。

### Protocol — 通信协议

MCU 间串口通信协议，帧格式：`SOF_H(0xAA) | SOF_L(0x55) | LEN | MSG_TYPE | FUNC_CODE | DATA[0..N] | XOR`。支持 5 种消息类型（系统、控制、查询、上报、数据透传），状态机解析，收完完整帧后回调。

```c
Protocol proto;
Proto_Init(&proto, on_frame);             // on_frame 为完整帧回调

while (1) {
    while (Serial_Available(&serial1))
        Proto_Feed(&proto, Serial_ReadByte(&serial1));
}

// 发送帧
uint8_t data[] = {0x01, 0x00};           // LED0 亮, LED1 灭
Proto_SendFrame(&serial1, MSG_CONTROL, FUNC_CTRL_LED, data, 2);
```

**消息类型**

| 类型 | 值 | 说明 |
|------|------|------|
| `MSG_SYSTEM` | 0x00 | 系统：PING/ACK/ERROR/RESET |
| `MSG_CONTROL` | 0x01 | 主→从：控制外设动作（LED、数码管等） |
| `MSG_QUERY` | 0x02 | 主→从：查询/读取 |
| `MSG_REPORT` | 0x03 | 从→主：数据上报 |
| `MSG_DATA` | 0x04 | 双向：数据块透传 |

**API**

| 函数 | 说明 |
|------|------|
| `Proto_Init(p, cb)` | 初始化协议实例，注册帧回调 |
| `Proto_Feed(p, byte)` | 喂入一个字节，内部状态机解析 |
| `Proto_SendFrame(s, type, func, data, len)` | 构造并发送一帧（含 SOF/XOR） |

**预定义控制结构体**：`ProtoCtrlLed` (LED0/1/2 + 流水灯间隔), `ProtoCtrlSmg` (数码管单/全图模式)

## 引脚分配

| 功能 | 引脚 | 说明 |
|------|------|------|
| LED0 | PF9 | 低电平点亮 |
| LED1 | PF10 | 低电平点亮 |
| BUZZER | PF8 | 高电平有效 |
| KEY0 | PE4 | 按下低电平 |
| KEY1 | PE3 | 按下低电平 |
| KEY2 | PE2 | 按下低电平 |
| KEY_UP | PA0 | 按下高电平 |
| LIGHT_SENSOR | PF7 | 暗=高电平, 亮=低电平 |
| TOUCH_KEY | PA5 | 电容触摸 |
| WIRE_DQ1 | PG9 | OneWire 总线 (DS18B20 / DHT11) |
| USART1_TX / RX | PA9 / PA10 | 串口 1 (115200 8N1) |
| USART3_TX / RX | PB10 / PB11 | 串口 3 |
| W25Q128_CS | PB14 | SPI Flash 片选 |
| LCD_BL | PB15 | TFT 背光控制 |
| T_CS / T_SCLK / T_MOSI / T_MISO / T_PEN | PC13 / PB0 / PF11 / PB2 / PB1 | XPT2046 触摸 |
| OLED_CS / DC / RW / RD / RST | PB7 / PD6 / PA4 / PD7 / PG15 | OLED 8080 控制线 |
| OLED_D[0:7] | PC6~9, PC11, PB6, PE5, PE6 | OLED 8080 数据线 |
| FSMC_D[0:15] | PD14/15, PD0/1, PE7~15, PD8~10 | TFT LCD 数据线 |
| FSMC 控制 | PD4/5/7/11/12, PE0/1 | TFT LCD FSMC (RD/WR/RS/CS) |
