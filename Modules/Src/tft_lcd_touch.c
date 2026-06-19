//
// Created by 35156 on 2026/6/19.
// XPT2046 电阻触摸，软件 SPI
//

#include "tft_lcd_touch.h"
#include "tft_lcd.h"

TouchData touch;

#define CMD_RDX  0xD0   // 读 X（官方配置）
#define CMD_RDY  0x90   // 读 Y（官方配置）

/* ── 引脚操作 ── */
#define TCS_L()   HAL_GPIO_WritePin(T_CS_GPIO_Port,   T_CS_Pin,   GPIO_PIN_RESET)
#define TCS_H()   HAL_GPIO_WritePin(T_CS_GPIO_Port,   T_CS_Pin,   GPIO_PIN_SET)
#define CLK_H()   HAL_GPIO_WritePin(T_SCLK_GPIO_Port, T_SCLK_Pin, GPIO_PIN_SET)
#define CLK_L()   HAL_GPIO_WritePin(T_SCLK_GPIO_Port, T_SCLK_Pin, GPIO_PIN_RESET)
#define MOSI_H()  HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_SET)
#define MOSI_L()  HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_RESET)
#define MISO_RD() HAL_GPIO_ReadPin(T_MISO_GPIO_Port,  T_MISO_Pin)
#define PEN_RD()  HAL_GPIO_ReadPin(T_PEN_GPIO_Port,   T_PEN_Pin)

/* ── 软件 SPI 读写 ── */
static uint16_t SPI_Read16(uint8_t cmd)
{
    uint16_t val = 0;
    TCS_L();
    // 写 8 位命令：数据在 CLK 上升沿被采样 → 先设 MOSI，再拉高 CLK
    for (uint8_t i = 0; i < 8; i++) {
        if (cmd & 0x80) MOSI_H(); else MOSI_L();
        CLK_H();
        CLK_L();
        cmd <<= 1;
    }
    // 等 XPT2046 完成采样（1 个 busy clock）
    CLK_H(); CLK_L();
    // 读 12 位数据：主机在 CLK 上升沿采样 MISO
    for (uint8_t i = 0; i < 12; i++) {
        CLK_H();
        val <<= 1;
        if (MISO_RD()) val |= 1;
        CLK_L();
    }
    TCS_H();
    return val;
}

/* ── 多次采样取平均 ── */
static uint16_t Read_Avg(uint8_t cmd, uint8_t times)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < times; i++) sum += SPI_Read16(cmd);
    return (uint16_t)(sum / times);
}

/* ── 初始化 ── */
void Touch_Init(void)
{
    touch.x = touch.y = 0;
    touch.pressed = 0;
}

/* ── 扫描一次 ── */
uint8_t Touch_Scan(void)
{
    if (PEN_RD() == GPIO_PIN_SET) {    // PEN=高=未触摸
        touch.pressed = 0;
        return 0;
    }

    uint16_t x = Read_Avg(CMD_RDX, 5);
    uint16_t y = Read_Avg(CMD_RDY, 5);

    // 检查是否还在触摸（防止抬起瞬间读到乱值）
    if (PEN_RD() == GPIO_PIN_SET) {
        touch.pressed = 0;
        return 0;
    }

    touch.x = x;
    touch.y = y;
    touch.pressed = 1;
    return 1;
}

/* ── 获取坐标（五点 Y 校准，四段线性） ── */
void Touch_GetXY(uint16_t *x, uint16_t *y)
{
    #define RAW_X_MIN  730
    #define RAW_X_MAX  3405
    // Y 校准点: raw → LCD(应为竖屏)  Y=0,120,240,360,479
    #define RY0   3516   // Y=0
    #define RY1   2759   // Y=120
    #define RY2   2008   // Y=240
    #define RY3   1253   // Y=360
    #define RY4   1058   // Y=479

    // X: 线性
    int32_t lx = (int32_t)(RAW_X_MAX - touch.x) * (tftlcd.width - 1) / (RAW_X_MAX - RAW_X_MIN);

    // Y: 四段线性
    int32_t ly;
    if (touch.y >= RY1) {
        ly = (int32_t)(RY0 - touch.y) * (120 - 0) / (RY0 - RY1);             // 0~120
    } else if (touch.y >= RY2) {
        ly = 120 + (int32_t)(RY1 - touch.y) * (240 - 120) / (RY1 - RY2);     // 120~240
    } else if (touch.y >= RY3) {
        ly = 240 + (int32_t)(RY2 - touch.y) * (360 - 240) / (RY2 - RY3);     // 240~360
    } else {
        ly = 360 + (int32_t)(RY3 - touch.y) * (479 - 360) / (RY3 - RY4);     // 360~479
    }
    if (lx < 0) lx = 0; if (lx >= (int32_t)tftlcd.width)  lx = tftlcd.width - 1;
    if (ly < 0) ly = 0; if (ly >= (int32_t)tftlcd.height) ly = tftlcd.height - 1;
    *x = lx;
    *y = ly;
}

uint8_t Touch_IsPressed(void)
{
    return touch.pressed;
}
