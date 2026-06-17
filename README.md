# Example STM32F407ZGT6

基于正点原子探索者V3开发板的 STM32F407ZGT6 裸机项目，使用 STM32CubeMX HAL 库 + CMake 构建，采用结构体+函数指针的模块化封装风格。

## 硬件平台

| 项目 | 说明 |
|------|------|
| 开发板 | 正点原子 探索者V3 |
| MCU | STM32F407ZGT6 (Cortex-M4F, 168 MHz) |
| 调试器 | DAPLink |
| 时钟 | HSI (16 MHz) → PLL → SYSCLK 168 MHz |

## 构建与烧录

```bash
# 配置 (ARM GCC)
cmake --preset gcc-debug

# 编译
cmake --build build/gcc-debug

# 烧录 (DAPLink / OpenOCD)
openocd -f daplink.cfg -f target/stm32f4x.cfg -c "program build/gcc-debug/Example_STM32F407ZGT6.elf verify reset exit"
```

工具链支持 ARM GCC 和 ST ARM Clang，详见 `cmake/` 目录下的工具链文件。

## 目录结构

```
├── Core/               # CubeMX 生成的 HAL 初始化代码
│   ├── Inc/            # 头文件 (main.h, gpio.h, adc.h, usart.h, ...)
│   └── Src/            # 源文件 (main.c, gpio.c, adc.c, usart.c, ...)
├── Drivers/            # CMSIS + STM32F4xx HAL 驱动库
├── Modules/            # 自定义模块封装
│   ├── Inc/            # 模块头文件
│   └── Src/            # 模块源文件
├── cmake/              # CMake 工具链文件
├── Docs/               # 开发板文档 (不纳入版本管理)
├── CMakeLists.txt      # 顶层构建文件
└── daplink.cfg         # OpenOCD DAPLink 配置
```

## 模块使用指南

所有模块以结构体封装外设状态，函数接收结构体指针操作。核心依赖链：**SoftTimer**（底层）← 所有其他模块。

---

### SoftTimer — 软件定时器

基于 `HAL_GetTick()` 的轻量级毫秒定时器，无中断/无硬件占用。

**API**

| 函数 | 说明 |
|------|------|
| `SoftTimer_Start(t, ms)` | 设置间隔并记录起始时刻 |
| `SoftTimer_Reset(t)` | 重置起始时刻，不改变间隔 |
| `SoftTimer_Expired(t)` | 到期检查，**到期后自动重置**并返回 true |
| `SoftTimer_Elapsed(t)` | 仅检查到期，**不重置** |

> **关键区别**: `Expired` 适合周期性任务（到点自动续期），`Elapsed` 适合一次性判断（查完不管）。

**例1：LED 每 500ms 闪烁一次**

```c
SOFT_TIMER blink_timer;
SoftTimer_Start(&blink_timer, 500);

while (1) {
    if (SoftTimer_Expired(&blink_timer)) {
        Led_Toggle(&led0);  // 每 500ms 翻转一次，自动续期
    }
}
```

**例2：同时运行多个定时器**

```c
SOFT_TIMER led_timer, beep_timer, print_timer;
SoftTimer_Start(&led_timer, 200);   // LED 每 200ms 闪
SoftTimer_Start(&beep_timer, 1000); // 蜂鸣器每 1s 响一次
SoftTimer_Start(&print_timer, 3000); // 每 3s 打印一次

while (1) {
    if (SoftTimer_Expired(&led_timer))  Led_Toggle(&led0);
    if (SoftTimer_Expired(&beep_timer)) Buzzer_Beep(&buzzer, 100);
    if (SoftTimer_Expired(&print_timer)) Serial_Println(&serial1, "alive");
}
```

**例3：Expired vs Elapsed 的典型用法**

```c
// 场景：按键按下后，LED 亮 2 秒再灭
SOFT_TIMER hold;
uint8_t led_lit = 0;

while (1) {
    Key_Scan(&key0);

    if (key0.event == KEY_EVENT_SHORT && !led_lit) {
        Led_On(&led0);
        SoftTimer_Start(&hold, 2000);
        led_lit = 1;
    }

    // 用 Elapsed（不重置）：只需判断一次"时间到了没"
    if (led_lit && SoftTimer_Elapsed(&hold)) {
        Led_Off(&led0);
        led_lit = 0;
    }
}
```

---

### LED — 板载 LED 控制

控制 LED0 (PF9) 和 LED1 (PF10)，低电平点亮。全局实例：`led0`, `led1`。

**API**

| 函数 | 说明 |
|------|------|
| `Leds_Init()` | 初始化，所有 LED 熄灭 |
| `Led_On(led)` / `Led_Off(led)` | 点亮 / 熄灭单个 LED |
| `Led_Toggle(led)` | 翻转单个 LED 状态 |
| `Led_OnAll()` / `Led_OffAll()` / `Led_ToggleAll()` | 点亮 / 熄灭 / 翻转全部 LED |
| `Led_Flow_Init(ms)` | 初始化流水灯，设置切换间隔 |
| `Led_Flow()` | 流水灯状态机，需在主循环中周期性调用 |

**例1：基本开关**

```c
Leds_Init();

Led_On(&led0);      // LED0 亮
HAL_Delay(500);
Led_Off(&led0);     // LED0 灭

Led_Toggle(&led1);  // LED1 翻转（灭→亮）
HAL_Delay(500);
Led_Toggle(&led1);  // LED1 再翻转（亮→灭）
```

**例2：SOS 求救信号（三短三长三短）**

```c
void sos_blink(void) {
    // 短闪 3 次
    for (int i = 0; i < 3; i++) {
        Led_On(&led0);  HAL_Delay(200);
        Led_Off(&led0); HAL_Delay(200);
    }
    HAL_Delay(300);
    // 长闪 3 次
    for (int i = 0; i < 3; i++) {
        Led_On(&led0);  HAL_Delay(600);
        Led_Off(&led0); HAL_Delay(200);
    }
    HAL_Delay(300);
    // 短闪 3 次
    for (int i = 0; i < 3; i++) {
        Led_On(&led0);  HAL_Delay(200);
        Led_Off(&led0); HAL_Delay(200);
    }
}

// 在主循环中通过按键触发
Key_Scan(&key0);
if (key0.event == KEY_EVENT_SHORT) sos_blink();
```

**例3：流水灯**

```c
Leds_Init();
Led_Flow_Init(300);  // 每 300ms 切换到下一个 LED

while (1) {
    Led_Flow();  // 周期性调用即可，内部自动管理状态
}
```

---

### Buzzer — 蜂鸣器

控制 PF8，高电平鸣响。全局实例：`buzzer`。

**API**

| 函数 | 说明 |
|------|------|
| `Buzzer_Init(bzr)` | 初始化，默认关闭 |
| `Buzzer_On(bzr)` / `Buzzer_Off(bzr)` | 打开 / 关闭 |
| `Buzzer_Toggle(bzr)` | 翻转状态 |
| `Buzzer_Beep(bzr, ms)` | 鸣响指定毫秒后**自动关闭** |
| `Buzzer_Loop(bzr)` | 后台处理，检查是否到时间该关（**必须在主循环调用**） |

> **注意**: 调用 `Buzzer_Beep()` 后，必须持续调用 `Buzzer_Loop()` 才能在到期时自动关闭蜂鸣器。

**例1：按键短按蜂鸣反馈**

```c
Buzzer_Init(&buzzer);

while (1) {
    Key_Scan(&key0);
    if (key0.event == KEY_EVENT_SHORT) {
        Buzzer_Beep(&buzzer, 80);  // 短促的 80ms 提示音
    }

    Buzzer_Loop(&buzzer);  // 必须调用，否则蜂鸣器不会自动停
}
```

**例2：报警音 —— 长按触发，直到松开才停**

```c
Buzzer_Init(&buzzer);

while (1) {
    Key_Scan(&key0);

    // 长按期间持续响
    if (key0.current_state == 1) {
        Buzzer_On(&buzzer);
    } else {
        Buzzer_Off(&buzzer);
    }
}
```

**例3：组合场景 —— 按键反馈 + 光敏报警**

```c
Buzzer_Init(&buzzer);
LightSensor_Init(&light_sensor);

while (1) {
    LightSensor_Scan(&light_sensor);
    Key_Scan(&key0);

    // 短按：提示音反馈
    if (key0.event == KEY_EVENT_SHORT) {
        Buzzer_Beep(&buzzer, 100);
    }

    // 暗环境持续蜂鸣报警
    if (LightSensor_IsDark(&light_sensor)) {
        Buzzer_On(&buzzer);
    } else {
        Buzzer_Off(&buzzer);
    }

    Buzzer_Loop(&buzzer);  // 处理 Beep 的自动关闭
}
```

---

### Key_Press — 按键事件

4 个按键 (KEY0 PE4, KEY1 PE3, KEY2 PE2, KEY_UP PA0)，软件消抖 + 事件解码。

| 事件 | 含义 |
|------|------|
| `KEY_EVENT_NONE` | 无事件 |
| `KEY_EVENT_SHORT` | 短按（按下后在 500ms 内释放） |
| `KEY_EVENT_LONG` | 长按（按下超过 500ms，触发一次） |
| `KEY_EVENT_REPEAT` | 连发（长按期间每 200ms 触发一次） |

全局实例：`key0`, `key1`, `key2`, `key_up`。

**例1：轮询所有按键**

```c
// key_map[] 是全局数组，包含全部 4 个按键，KEY_COUNT = 4
while (1) {
    for (int i = 0; i < KEY_COUNT; i++) {
        Key_Scan(key_map[i]);

        switch (key_map[i]->event) {
            case KEY_EVENT_SHORT:
                // 按键 i 被短按
                break;
            case KEY_EVENT_LONG:
                // 按键 i 被长按
                break;
            case KEY_EVENT_REPEAT:
                // 按键 i 正在连发
                break;
            default:
                break;
        }
    }
}
```

**例2：单按键多功能 —— 短按切换 LED，长按蜂鸣**

```c
while (1) {
    Key_Scan(&key0);

    switch (key0.event) {
        case KEY_EVENT_SHORT:
            Led_Toggle(&led0);
            break;
        case KEY_EVENT_LONG:
            Buzzer_Beep(&buzzer, 500);
            break;
        case KEY_EVENT_REPEAT:
            // 长按期间的连发：控制亮度增减等
            break;
        default:
            break;
    }

    Buzzer_Loop(&buzzer);
}
```

**例3：按键组合 —— 双键同时按下**

```c
while (1) {
    Key_Scan(&key0);
    Key_Scan(&key1);

    // 两个键同时处于按下状态 = 组合键
    if (key0.current_state == 1 && key1.current_state == 1) {
        Led_OnAll();
        Buzzer_On(&buzzer);
    } else {
        Led_OffAll();
        Buzzer_Off(&buzzer);
    }
}
```

---

### Light_Sensor — 光敏传感器

读取 PF7 数字量，判断环境亮暗。暗=高电平，亮=低电平。全局实例：`light_sensor`。

**API**

| 函数 | 说明 |
|------|------|
| `LightSensor_Init(lts)` | 初始化并首次扫描 |
| `LightSensor_Scan(lts)` | 读取引脚电平，更新内部标志 |
| `LightSensor_IsDark(lts)` | 当前是否为暗环境 |
| `LightSensor_IsLight(lts)` | 当前是否为亮环境 |

> **注意**: 必须在主循环中周期性调用 `LightSensor_Scan()` 才能获取最新状态。`IsDark`/`IsLight` 只是读标志，不会主动扫描。

**例1：夜间自动亮灯**

```c
LightSensor_Init(&light_sensor);

while (1) {
    LightSensor_Scan(&light_sensor);

    if (LightSensor_IsDark(&light_sensor)) {
        Led_On(&led0);   // 暗环境自动开灯
    } else {
        Led_Off(&led0);  // 亮环境关灯
    }

    HAL_Delay(100);  // 100ms 扫描一次即可
}
```

**例2：状态变化时打印通知（带去抖）**

```c
LightSensor_Init(&light_sensor);
Serial_Init(&serial1);

LIGHT_FLAG last_state = light_sensor.light_flag;

while (1) {
    LightSensor_Scan(&light_sensor);

    if (light_sensor.light_flag != last_state) {
        last_state = light_sensor.light_flag;

        if (LightSensor_IsDark(&light_sensor)) {
            Serial_Println(&serial1, "-> Dark");
        } else {
            Serial_Println(&serial1, "-> Light");
        }
    }
}
```

---

### Touch_Pad — 电容触摸

RC 充电延迟法实现电容触摸感应 (PA5)，无需 ADC。自适应基线校准：未触摸时缓慢跟踪漂移，触摸时快速适应。全局实例：`touch_pad`。

**API**

| 函数 | 说明 |
|------|------|
| `TouchPad_Init(tp)` | 初始化，10 次采样建立充电基线（需约 1 秒） |
| `TouchPad_Scan(tp)` | 测充电时间，与基线比较，更新按下状态 |
| `TouchPad_IsPressed(tp)` | 当前是否被触摸 |

> **注意**: `TouchPad_Init()` 需要约 1 秒建立基线，建议在上电初始化阶段调用。`TouchPad_Scan()` 需周期性调用（建议每 20-50ms 一次）。

**例1：触摸开关灯**

```c
TouchPad_Init(&touch_pad);

while (1) {
    TouchPad_Scan(&touch_pad);

    if (TouchPad_IsPressed(&touch_pad)) {
        Led_On(&led0);
    } else {
        Led_Off(&led0);
    }

    HAL_Delay(30);  // 30ms 扫描间隔
}
```

**例2：触摸翻转 —— 摸一下开，再摸一下关**

```c
TouchPad_Init(&touch_pad);
uint8_t last_pressed = 0;

while (1) {
    TouchPad_Scan(&touch_pad);

    uint8_t now = TouchPad_IsPressed(&touch_pad);
    // 上升沿检测：从未触摸变为触摸
    if (now && !last_pressed) {
        Led_Toggle(&led0);
        Buzzer_Beep(&buzzer, 50);  // 触摸反馈音
    }
    last_pressed = now;

    Buzzer_Loop(&buzzer);
    HAL_Delay(30);
}
```

---

### Serial — 串口收发

USART1 (PA9 TX / PA10 RX, 115200 8N1)，中断接收 + 256 字节环形缓冲区。全局实例：`serial1` (绑定 `huart1`)。

**API**

| 函数 | 说明 |
|------|------|
| `Serial_Init(s)` | 清空缓冲区，启动中断接收 |
| `Serial_SendByte(s, byte)` | 发送单字节（阻塞） |
| `Serial_SendData(s, data, len)` | 发送字节数组（阻塞） |
| `Serial_SendString(s, str)` | 发送字符串（阻塞） |
| `Serial_PrintU32(s, val)` | 打印 32 位无符号整数 |
| `Serial_Println(s, str)` | 发送字符串 + `\r\n` 换行 |
| `Serial_Available(s)` | 返回环形缓冲区中可读的字节数 |
| `Serial_ReadByte(s)` | 从缓冲区读取一个字节 |

**例1：回显 (Echo)**

```c
Serial_Init(&serial1);

while (1) {
    // 把收到的字节原样发回去
    while (Serial_Available(&serial1)) {
        uint8_t ch = Serial_ReadByte(&serial1);
        Serial_SendByte(&serial1, ch);
    }
}
```

**例2：简单命令行解析**

```c
Serial_Init(&serial1);
Serial_Println(&serial1, "System ready.");

while (1) {
    // 收到换行符时处理一条命令
    static char cmd_buf[64];
    static int  cmd_idx = 0;

    while (Serial_Available(&serial1)) {
        uint8_t ch = Serial_ReadByte(&serial1);

        if (ch == '\r' || ch == '\n') {
            cmd_buf[cmd_idx] = '\0';
            cmd_idx = 0;

            // 解析命令
            if      (strcmp(cmd_buf, "led on")  == 0) Led_On(&led0);
            else if (strcmp(cmd_buf, "led off") == 0) Led_Off(&led0);
            else if (strcmp(cmd_buf, "beep")    == 0) Buzzer_Beep(&buzzer, 200);
            else Serial_Println(&serial1, "unknown cmd");

            Serial_SendString(&serial1, "> ");
        } else if (cmd_idx < sizeof(cmd_buf) - 1) {
            cmd_buf[cmd_idx++] = ch;
        }
    }

    Buzzer_Loop(&buzzer);
}
```

**例3：带格式的传感器数据输出**

```c
Serial_Init(&serial1);
LightSensor_Init(&light_sensor);

while (1) {
    LightSensor_Scan(&light_sensor);

    Serial_SendString(&serial1, "[LIGHT] ");
    Serial_SendString(&serial1,
        LightSensor_IsDark(&light_sensor) ? "Dark" : "Light");
    Serial_Println(&serial1, "");

    HAL_Delay(1000);
}
```

输出效果：
```
[LIGHT] Light
[LIGHT] Light
[LIGHT] Dark
```

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
