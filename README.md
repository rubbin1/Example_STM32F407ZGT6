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
│   ├── Inc/            # 头文件 (main.h, gpio.h, adc.h, usart.h, fsmc.h, ...)
│   └── Src/            # 源文件 (main.c, gpio.c, adc.c, usart.c, fsmc.c, ...)
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
├── cmake/              # CMake 工具链文件 (含 stm32cubemx 子目录)
├── Docs/               # 开发板文档 (不纳入版本管理)
├── CMakeLists.txt      # 顶层构建文件
└── daplink.cfg         # OpenOCD DAPLink 配置
```

## 已封装模块

所有模块以结构体封装外设状态，函数接收结构体指针操作。除 `hw_timer` 外，其余模块均依赖 `soft_timer` 提供定时能力。

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

USART1 (PA9/PA10, 115200 8N1) 的中断驱动串口，带 256 字节环形接收缓冲区。

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

### TFT Touch — 电阻触摸

XPT2046 触摸控制器，软件 SPI 通信。PEN 中断检测触摸状态，5 次采样取平均，四点 Y 分段 + 线性 X 校准。全局实例：`touch`。

```c
Touch_Init();

while (1) {
    if (Touch_Scan()) {                  // 返回 1 表示检测到触摸
        uint16_t x, y;
        Touch_GetXY(&x, &y);             // 获取校准后的 LCD 坐标
        // x,y 已经映射到 LCD 分辨率
    }
    if (Touch_IsPressed()) { /* 正在触摸中 */ }
}
```

| 函数 | 说明 |
|------|------|
| `Touch_Init()` | 初始化，清零状态 |
| `Touch_Scan()` | 扫描一次，返回 1=检测到新的触摸数据 |
| `Touch_GetXY(x, y)` | 获取校准后的坐标（已映射到 LCD 分辨率） |
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
| USART1_TX | PA9 | 115200 8N1 |
| USART1_RX | PA10 | 115200 8N1 |
