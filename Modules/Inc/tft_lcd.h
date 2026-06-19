//
// Created by 35156 on 2026/6/19.
// TFT LCD (MCU屏) FSMC 驱动
//

#ifndef TFTLCD_H
#define TFTLCD_H
#include <stdint.h>
#include "main.h"

/* ── RGB565 颜色 ── */
#define TFT_WHITE       0xFFFF
#define TFT_BLACK       0x0000
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_GRAY        0x8430
#define TFT_BROWN       0xBC40
#define TFT_DARKBLUE    0x01CF
#define TFT_LIGHTBLUE   0x7D7C
#define TFT_ORANGE      0xFC00
#define TFT_PURPLE      0x8010
#define TFT_LIGHTGREEN  0x841F

/* ── LCD 信息结构体 ── */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t id;            // 驱动芯片 ID
    uint8_t  dir;           // 显示方向 0=竖屏, 1=横屏
} TFTLCD_Dev;
extern TFTLCD_Dev tftlcd;

/* ── 全局背景色（ShowString 等自动用此色填背景） ── */
extern uint16_t tft_bg_color;    // 背景色（默认白）

void TFTLCD_SetBackColor(uint16_t bg);

/* ── 基础 API ── */
void TFTLCD_Init(void);
void TFTLCD_Clear(uint16_t color);
void TFTLCD_DisplayDir(uint8_t dir);     // 旋转屏幕 0=竖 1=横

void TFTLCD_SetCursor(uint16_t x, uint16_t y);
void TFTLCD_SetWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);
void TFTLCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void TFTLCD_Fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color);

/* ── 图形 ── */
void TFTLCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void TFTLCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void TFTLCD_DrawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void TFTLCD_DrawCircle(uint16_t cx, uint16_t cy, uint8_t r, uint16_t color);
void TFTLCD_DrawFillCircle(uint16_t cx, uint16_t cy, uint8_t r, uint16_t color);

/* ── 文字（背景用 tft_bg_color，前景传参） ── */
void TFTLCD_ShowChar(uint16_t x, uint16_t y, char ch, uint8_t size, uint16_t color);
void TFTLCD_ShowString(uint16_t x, uint16_t y, const char *str, uint8_t size, uint16_t color);
void TFTLCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void TFTLCD_Printf(uint16_t x, uint16_t y, uint8_t size, uint16_t color, const char *fmt, ...);

void TFTLCD_BackLight(uint8_t on);

#endif //TFTLCD_H
