//
// Created by 35156 on 2026/6/19.
// TFT LCD (MCU屏) FSMC 驱动
// 支持 ILI9341 / ST7789 / NT35310 等
//

#include "tft_lcd.h"
#include "fsmc.h"
#include <string.h>

/* ── FSMC 内存映射 ──
 * FSMC_NE4 基地址 = 0x6C00 0000
 * RS 接 FSMC_A6 → 偏移 = 2^6 * 2 = 0x80
 * LCD_BASE = 0x6C00 0000 | (0x80 - 2) = 0x6C00 007E
 *   LCD->LCD_REG (RS=0, A6=0): 写寄存器索引  → 地址 0x6C00 007E
 *   LCD->LCD_RAM (RS=1, A6=1): 写/读数据     → 地址 0x6C00 0080
 */
typedef struct {
    volatile uint16_t REG;
    volatile uint16_t RAM;
} LCD_TypeDef;

#define LCD_BASE  ((uint32_t)(0x60000000 | (0x4000000 * 3) | (((1 << 6) * 2) - 2)))
#define LCD       ((LCD_TypeDef *)LCD_BASE)

TFTLCD_Dev tftlcd;

/* ── 全局背景色 ── */
uint16_t tft_bg_color;
void TFTLCD_SetBackColor(uint16_t bg) {
    TFTLCD_Clear(bg);   // 全屏换背景
    tft_bg_color = bg;
}

/* ── 底层读写 ── */
static void LCD_WR_REG(uint16_t reg)  { LCD->REG = reg; }
static void LCD_WR_DATA(uint16_t dat) { LCD->RAM = dat; }
static void LCD_WriteReg(uint16_t reg, uint16_t val) { LCD_WR_REG(reg); LCD_WR_DATA(val); }
static uint16_t LCD_RD_DATA(void)     { uint16_t r; r = LCD->RAM; return r; }

static uint16_t LCD_ReadID(void)
{
    uint16_t id;
    LCD_WR_REG(0xD3);
    id  = LCD_RD_DATA();    // dummy
    id  = LCD_RD_DATA();    // 0x00
    id  = LCD_RD_DATA();    // 0x93
    id <<= 8;
    id |= LCD_RD_DATA();    // 0x41
    return id;
}

void TFTLCD_BackLight(uint8_t on)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ════════════════════════════ ILI9341 初始化 ════════════════════════════ */

static void ILI9341_Init(void)
{
    LCD_WR_REG(0xCF); LCD_WR_DATA(0x00); LCD_WR_DATA(0xC1); LCD_WR_DATA(0x30);
    LCD_WR_REG(0xED); LCD_WR_DATA(0x64); LCD_WR_DATA(0x03); LCD_WR_DATA(0x12); LCD_WR_DATA(0x81);
    LCD_WR_REG(0xE8); LCD_WR_DATA(0x85); LCD_WR_DATA(0x10); LCD_WR_DATA(0x7A);
    LCD_WR_REG(0xCB); LCD_WR_DATA(0x39); LCD_WR_DATA(0x2C); LCD_WR_DATA(0x00); LCD_WR_DATA(0x34); LCD_WR_DATA(0x02);
    LCD_WR_REG(0xF7); LCD_WR_DATA(0x20);
    LCD_WR_REG(0xEA); LCD_WR_DATA(0x00); LCD_WR_DATA(0x00);
    LCD_WR_REG(0xC0); LCD_WR_DATA(0x1B);
    LCD_WR_REG(0xC1); LCD_WR_DATA(0x01);
    LCD_WR_REG(0xC5); LCD_WR_DATA(0x30); LCD_WR_DATA(0x30);
    LCD_WR_REG(0xC7); LCD_WR_DATA(0xB7);
    LCD_WR_REG(0x36); LCD_WR_DATA(0x48);
    LCD_WR_REG(0x3A); LCD_WR_DATA(0x55);
    LCD_WR_REG(0xB1); LCD_WR_DATA(0x00); LCD_WR_DATA(0x1A);
    LCD_WR_REG(0xB6); LCD_WR_DATA(0x0A); LCD_WR_DATA(0xA2);
    LCD_WR_REG(0xF2); LCD_WR_DATA(0x00);
    LCD_WR_REG(0x26); LCD_WR_DATA(0x01);
    LCD_WR_REG(0xE0); LCD_WR_DATA(0x0F); LCD_WR_DATA(0x2A); LCD_WR_DATA(0x28);
    LCD_WR_DATA(0x08); LCD_WR_DATA(0x0E); LCD_WR_DATA(0x08); LCD_WR_DATA(0x54);
    LCD_WR_DATA(0xA9); LCD_WR_DATA(0x43); LCD_WR_DATA(0x0A); LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x00); LCD_WR_DATA(0x00); LCD_WR_DATA(0x00); LCD_WR_DATA(0x00);
    LCD_WR_REG(0xE1); LCD_WR_DATA(0x00); LCD_WR_DATA(0x15); LCD_WR_DATA(0x17);
    LCD_WR_DATA(0x07); LCD_WR_DATA(0x11); LCD_WR_DATA(0x06); LCD_WR_DATA(0x2B);
    LCD_WR_DATA(0x56); LCD_WR_DATA(0x3C); LCD_WR_DATA(0x05); LCD_WR_DATA(0x10);
    LCD_WR_DATA(0x0F); LCD_WR_DATA(0x3F); LCD_WR_DATA(0x3F); LCD_WR_DATA(0x0F);
    LCD_WR_REG(0x2B); LCD_WR_DATA(0x00); LCD_WR_DATA(0x00); LCD_WR_DATA(0x01); LCD_WR_DATA(0x3F);
    LCD_WR_REG(0x2A); LCD_WR_DATA(0x00); LCD_WR_DATA(0x00); LCD_WR_DATA(0x00); LCD_WR_DATA(0xEF);
    LCD_WR_REG(0x11); HAL_Delay(120);
    LCD_WR_REG(0x29);
}

/* ════════════════════════════ ST7789 初始化 ════════════════════════════ */

static void ST7789_Init(void)
{
    LCD_WR_REG(0x11);    // Sleep out
    HAL_Delay(120);
    LCD_WR_REG(0x36); LCD_WR_DATA(0x00);          // MADCTL
    LCD_WR_REG(0x3A); LCD_WR_DATA(0x05);          // 16bit/pixel
    LCD_WR_REG(0xB2); LCD_WR_DATA(0x0C); LCD_WR_DATA(0x0C); LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x33); LCD_WR_DATA(0x33);
    LCD_WR_REG(0xB7); LCD_WR_DATA(0x35);          // VCOM
    LCD_WR_REG(0xBB); LCD_WR_DATA(0x2B);          // VCOM
    LCD_WR_REG(0xC0); LCD_WR_DATA(0x2C);          // LCM
    LCD_WR_REG(0xC2); LCD_WR_DATA(0x01); LCD_WR_DATA(0xFF);
    LCD_WR_REG(0xC3); LCD_WR_DATA(0x11);          // VRH
    LCD_WR_REG(0xC4); LCD_WR_DATA(0x20);          // VDV
    LCD_WR_REG(0xC6); LCD_WR_DATA(0x0F);          // Frame Rate
    LCD_WR_REG(0xD0); LCD_WR_DATA(0xA4); LCD_WR_DATA(0xA1);
    LCD_WR_REG(0xE0); LCD_WR_DATA(0xD0); LCD_WR_DATA(0x00); LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x0E); LCD_WR_DATA(0x15); LCD_WR_DATA(0x0D); LCD_WR_DATA(0x37);
    LCD_WR_DATA(0x43); LCD_WR_DATA(0x47); LCD_WR_DATA(0x09); LCD_WR_DATA(0x15);
    LCD_WR_DATA(0x12); LCD_WR_DATA(0x16); LCD_WR_DATA(0x19);
    LCD_WR_REG(0xE1); LCD_WR_DATA(0xD0); LCD_WR_DATA(0x00); LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x0D); LCD_WR_DATA(0x0C); LCD_WR_DATA(0x06); LCD_WR_DATA(0x2D);
    LCD_WR_DATA(0x44); LCD_WR_DATA(0x40); LCD_WR_DATA(0x0E); LCD_WR_DATA(0x1C);
    LCD_WR_DATA(0x18); LCD_WR_DATA(0x16); LCD_WR_DATA(0x19);
    LCD_WR_REG(0x21);    // Inversion ON
    LCD_WR_REG(0x29);    // Display ON
}

/* ════════════════════════════ 初始化 ════════════════════════════ */

static uint16_t ReadID_D3(void) {
    LCD_WR_REG(0xD3);
    uint16_t id = LCD_RD_DATA();
    id = LCD_RD_DATA();
    id = LCD_RD_DATA();
    id <<= 8;
    id |= LCD_RD_DATA();
    return id;
}

static uint16_t ReadID_04(void) {
    LCD_WR_REG(0x04);
    uint16_t id = LCD_RD_DATA();
    id = LCD_RD_DATA();
    id = LCD_RD_DATA();
    id <<= 8;
    id |= LCD_RD_DATA();
    return id;
}

void TFTLCD_Init(void)
{
    TFTLCD_BackLight(1);
    MX_FSMC_Init();

    /* ── ID 检测链（与官方 lcd.c 一致） ── */
    tftlcd.id = ReadID_D3();

    if (tftlcd.id != 0x9341) {
        tftlcd.id = ReadID_04();
        if (tftlcd.id == 0x8552) tftlcd.id = 0x7789;

        if (tftlcd.id != 0x7789) {
            tftlcd.id = LCD_ReadID();  // 0xD3 again
            if (tftlcd.id != 0x5310 && tftlcd.id != 0x7796) {
                // NT35510 / ILI9806 / SSD1963
                LCD_WriteReg(0xF000, 0x0055);
                LCD_WriteReg(0xF001, 0x00AA);
                LCD_WriteReg(0xF002, 0x0052);
                LCD_WriteReg(0xF003, 0x0008);
                LCD_WriteReg(0xF004, 0x0001);
                LCD_WR_REG(0xC500);
                tftlcd.id = LCD_RD_DATA(); tftlcd.id <<= 8;
                LCD_WR_REG(0xC501);
                tftlcd.id |= LCD_RD_DATA();
                HAL_Delay(5);
                if (tftlcd.id != 0x5510) {
                    tftlcd.id = ReadID_D3();
                    if (tftlcd.id != 0x9806) {
                        LCD_WR_REG(0xA1);
                        tftlcd.id = LCD_RD_DATA();
                        tftlcd.id = LCD_RD_DATA();
                        tftlcd.id <<= 8;
                        tftlcd.id |= LCD_RD_DATA();
                        if (tftlcd.id == 0x5761) tftlcd.id = 0x1963;
                    }
                }
            }
        }
    }

    /* ── 按 ID 初始化 + 设分辨率 ── */
    if (tftlcd.id == 0x9341) {
        ILI9341_Init();
        tftlcd.width = 240; tftlcd.height = 320;
    } else if (tftlcd.id == 0x7789) {
        ST7789_Init();
        tftlcd.width = 240; tftlcd.height = 320;
    } else if (tftlcd.id == 0x5310) {
        ILI9341_Init();
        tftlcd.width = 320; tftlcd.height = 480;
    } else if (tftlcd.id == 0x7796) {
        /* ST7796 初始化 (3.5寸 320×480) */
        LCD_WR_REG(0x11); HAL_Delay(120);
        LCD_WR_REG(0x36); LCD_WR_DATA(0x08);
        LCD_WR_REG(0x3A); LCD_WR_DATA(0x55);
        LCD_WR_REG(0xF0); LCD_WR_DATA(0xC3);
        LCD_WR_REG(0xF0); LCD_WR_DATA(0x96);
        LCD_WR_REG(0xB4); LCD_WR_DATA(0x01);
        LCD_WR_REG(0xB6); LCD_WR_DATA(0x0A); LCD_WR_DATA(0xA2);
        LCD_WR_REG(0xB7); LCD_WR_DATA(0xC6);
        LCD_WR_REG(0xB9); LCD_WR_DATA(0x02); LCD_WR_DATA(0xE0);
        LCD_WR_REG(0xC0); LCD_WR_DATA(0x80); LCD_WR_DATA(0x16);
        LCD_WR_REG(0xC1); LCD_WR_DATA(0x19);
        LCD_WR_REG(0xC2); LCD_WR_DATA(0xA7);
        LCD_WR_REG(0xC5); LCD_WR_DATA(0x16);
        LCD_WR_REG(0xE8); LCD_WR_DATA(0x40); LCD_WR_DATA(0x8A); LCD_WR_DATA(0x00);
        LCD_WR_DATA(0x00); LCD_WR_DATA(0x29); LCD_WR_DATA(0x19);
        LCD_WR_DATA(0xA5); LCD_WR_DATA(0x33);
        LCD_WR_REG(0xE0); LCD_WR_DATA(0xF0); LCD_WR_DATA(0x07); LCD_WR_DATA(0x0D);
        LCD_WR_DATA(0x04); LCD_WR_DATA(0x05); LCD_WR_DATA(0x14); LCD_WR_DATA(0x36);
        LCD_WR_DATA(0x54); LCD_WR_DATA(0x4C); LCD_WR_DATA(0x38); LCD_WR_DATA(0x13);
        LCD_WR_DATA(0x14); LCD_WR_DATA(0x2E); LCD_WR_DATA(0x34);
        LCD_WR_REG(0xE1); LCD_WR_DATA(0xF0); LCD_WR_DATA(0x10); LCD_WR_DATA(0x14);
        LCD_WR_DATA(0x0E); LCD_WR_DATA(0x0C); LCD_WR_DATA(0x08); LCD_WR_DATA(0x35);
        LCD_WR_DATA(0x44); LCD_WR_DATA(0x4C); LCD_WR_DATA(0x26); LCD_WR_DATA(0x10);
        LCD_WR_DATA(0x12); LCD_WR_DATA(0x2C); LCD_WR_DATA(0x32);
        LCD_WR_REG(0xF0); LCD_WR_DATA(0x3C);
        LCD_WR_REG(0xF0); LCD_WR_DATA(0x69);
        HAL_Delay(120);
        LCD_WR_REG(0x21);   // Inversion ON
        LCD_WR_REG(0x29);   // Display ON
        tftlcd.width = 320; tftlcd.height = 480;
    } else if (tftlcd.id == 0x5510 || tftlcd.id == 0x9806) {
        ILI9341_Init();
        tftlcd.width = 480; tftlcd.height = 800;
    } else if (tftlcd.id == 0x1963) {
        tftlcd.width = 800; tftlcd.height = 480;
    } else {
        tftlcd.width = 320; tftlcd.height = 480;  // 默认
    }

    tftlcd.dir = 0;
    TFTLCD_Clear(TFT_BLACK);   // ST7796 反相，BLACK=白屏
}

/* ════════════════════════════ 窗口与画点 ════════════════════════════ */

void TFTLCD_SetCursor(uint16_t x, uint16_t y)
{
    LCD_WR_REG(0x2A); LCD_WR_DATA(x >> 8); LCD_WR_DATA(x & 0xFF);
    LCD_WR_DATA(x >> 8); LCD_WR_DATA(x & 0xFF);
    LCD_WR_REG(0x2B); LCD_WR_DATA(y >> 8); LCD_WR_DATA(y & 0xFF);
    LCD_WR_DATA(y >> 8); LCD_WR_DATA(y & 0xFF);
    LCD_WR_REG(0x2C);
}

void TFTLCD_SetWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    LCD_WR_REG(0x2A);
    LCD_WR_DATA(xs >> 8); LCD_WR_DATA(xs & 0xFF);
    LCD_WR_DATA(xe >> 8); LCD_WR_DATA(xe & 0xFF);
    LCD_WR_REG(0x2B);
    LCD_WR_DATA(ys >> 8); LCD_WR_DATA(ys & 0xFF);
    LCD_WR_DATA(ye >> 8); LCD_WR_DATA(ye & 0xFF);
    LCD_WR_REG(0x2C);
}

void TFTLCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= tftlcd.width || y >= tftlcd.height) return;
    TFTLCD_SetCursor(x, y);
    LCD_WR_DATA(color);
}

void TFTLCD_Clear(uint16_t color)
{
    tft_bg_color = color;
    TFTLCD_SetWindow(0, 0, tftlcd.width - 1, tftlcd.height - 1);
    for (uint32_t i = 0; i < (uint32_t)tftlcd.width * tftlcd.height; i++)
        LCD_WR_DATA(color);
}

void TFTLCD_Fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color)
{
    if (xs > xe || ys > ye) return;
    TFTLCD_SetWindow(xs, ys, xe, ye);
    uint32_t n = (uint32_t)(xe - xs + 1) * (ye - ys + 1);
    while (n--) LCD_WR_DATA(color);
}

/* ════════════════════════════ 图形 ════════════════════════════ */

void TFTLCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int16_t dx = (x2 > x1) ? x2 - x1 : x1 - x2;
    int16_t dy = (y2 > y1) ? y2 - y1 : y1 - y2;
    int8_t  sx = (x1 < x2) ? 1 : -1;
    int8_t  sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy, e2;
    while (1) {
        TFTLCD_DrawPoint(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = err << 1;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

void TFTLCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    TFTLCD_DrawLine(x, y, x + w, y, color);
    TFTLCD_DrawLine(x + w, y, x + w, y + h, color);
    TFTLCD_DrawLine(x + w, y + h, x, y + h, color);
    TFTLCD_DrawLine(x, y + h, x, y, color);
}

void TFTLCD_DrawCircle(uint16_t cx, uint16_t cy, uint8_t r, uint16_t color)
{
    int16_t a = 0, b = r, d = 3 - (r << 1);
    while (a <= b) {
        TFTLCD_DrawPoint(cx + a, cy + b, color);
        TFTLCD_DrawPoint(cx - a, cy + b, color);
        TFTLCD_DrawPoint(cx + a, cy - b, color);
        TFTLCD_DrawPoint(cx - a, cy - b, color);
        TFTLCD_DrawPoint(cx + b, cy + a, color);
        TFTLCD_DrawPoint(cx - b, cy + a, color);
        TFTLCD_DrawPoint(cx + b, cy - a, color);
        TFTLCD_DrawPoint(cx - b, cy - a, color);
        a++;
        if (d < 0) { d += 4 * a + 6; }
        else       { d += 10 + 4 * (a - b); b--; }
    }
}

void TFTLCD_DrawFillCircle(uint16_t cx, uint16_t cy, uint8_t r, uint16_t color)
{
    int16_t a = 0, b = r, d = 3 - (r << 1);
    while (a <= b) {
        for (int16_t i = cx - b; i <= cx + b; i++) {
            TFTLCD_DrawPoint(i, cy + a, color);
            TFTLCD_DrawPoint(i, cy - a, color);
        }
        for (int16_t i = cx - a; i <= cx + a; i++) {
            TFTLCD_DrawPoint(i, cy + b, color);
            TFTLCD_DrawPoint(i, cy - b, color);
        }
        a++;
        if (d < 0) { d += 4 * a + 6; }
        else       { d += 10 + 4 * (a - b); b--; }
    }
}

void TFTLCD_DrawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    TFTLCD_Fill(x, y, x + w, y + h, color);
}

void TFTLCD_DisplayDir(uint8_t dir)
{
    tftlcd.dir = dir;
    if (dir == 0) {
        tftlcd.width  = (tftlcd.id == 0x5510) ? 480 : (tftlcd.id == 0x5310 || tftlcd.id == 0x7796) ? 320 : 240;
        tftlcd.height = (tftlcd.id == 0x5510) ? 800 : (tftlcd.id == 0x5310 || tftlcd.id == 0x7796) ? 480 : 320;
    } else {
        tftlcd.width  = (tftlcd.id == 0x5510) ? 800 : (tftlcd.id == 0x5310 || tftlcd.id == 0x7796) ? 480 : 320;
        tftlcd.height = (tftlcd.id == 0x5510) ? 480 : (tftlcd.id == 0x5310 || tftlcd.id == 0x7796) ? 320 : 240;
    }
}

/* ════════════════════════════ 6×8 字库 ════════════════════════════ */

static const uint8_t font_6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, /* sp */
    {0x00,0x00,0x5F,0x00,0x00,0x00}, /* !   */
    {0x00,0x07,0x00,0x07,0x00,0x00}, /* "   */
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, /* #   */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, /* $   */
    {0x23,0x13,0x08,0x64,0x62,0x00}, /* %   */
    {0x36,0x49,0x56,0x20,0x50,0x00}, /* &   */
    {0x00,0x08,0x07,0x00,0x00,0x00}, /* '   */
    {0x00,0x1C,0x22,0x41,0x00,0x00}, /* (   */
    {0x00,0x41,0x22,0x1C,0x00,0x00}, /* )   */
    {0x2A,0x1C,0x7F,0x1C,0x2A,0x00}, /* *   */
    {0x08,0x08,0x3E,0x08,0x08,0x00}, /* +   */
    {0x00,0x80,0x70,0x00,0x00,0x00}, /* ,   */
    {0x08,0x08,0x08,0x08,0x08,0x00}, /* -   */
    {0x00,0x00,0x60,0x60,0x00,0x00}, /* .   */
    {0x20,0x10,0x08,0x04,0x02,0x00}, /* /   */
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, /* 0   */
    {0x00,0x42,0x7F,0x40,0x00,0x00}, /* 1   */
    {0x62,0x51,0x49,0x49,0x46,0x00}, /* 2   */
    {0x22,0x41,0x49,0x49,0x36,0x00}, /* 3   */
    {0x18,0x14,0x12,0x7F,0x10,0x00}, /* 4   */
    {0x27,0x45,0x45,0x45,0x39,0x00}, /* 5   */
    {0x3C,0x4A,0x49,0x49,0x31,0x00}, /* 6   */
    {0x41,0x21,0x11,0x09,0x07,0x00}, /* 7   */
    {0x36,0x49,0x49,0x49,0x36,0x00}, /* 8   */
    {0x46,0x49,0x49,0x29,0x1E,0x00}, /* 9   */
    {0x00,0x00,0x14,0x00,0x00,0x00}, /* :   */
    {0x00,0x80,0x68,0x00,0x00,0x00}, /* ;   */
    {0x08,0x14,0x22,0x41,0x00,0x00}, /* <   */
    {0x14,0x14,0x14,0x14,0x14,0x00}, /* =   */
    {0x00,0x41,0x22,0x14,0x08,0x00}, /* >   */
    {0x02,0x01,0x51,0x09,0x06,0x00}, /* ?   */
    {0x32,0x49,0x79,0x41,0x3E,0x00}, /* @   */
    {0x7E,0x09,0x09,0x09,0x7E,0x00}, /* A   */
    {0x7F,0x49,0x49,0x49,0x36,0x00}, /* B   */
    {0x3E,0x41,0x41,0x41,0x22,0x00}, /* C   */
    {0x7F,0x41,0x41,0x41,0x3E,0x00}, /* D   */
    {0x7F,0x49,0x49,0x49,0x41,0x00}, /* E   */
    {0x7F,0x09,0x09,0x09,0x01,0x00}, /* F   */
    {0x3E,0x41,0x49,0x49,0x3A,0x00}, /* G   */
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, /* H   */
    {0x00,0x41,0x7F,0x41,0x00,0x00}, /* I   */
    {0x30,0x40,0x40,0x40,0x3F,0x00}, /* J   */
    {0x7F,0x08,0x14,0x22,0x41,0x00}, /* K   */
    {0x7F,0x40,0x40,0x40,0x40,0x00}, /* L   */
    {0x7F,0x02,0x04,0x02,0x7F,0x00}, /* M   */
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, /* N   */
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, /* O   */
    {0x7F,0x09,0x09,0x09,0x06,0x00}, /* P   */
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, /* Q   */
    {0x7F,0x09,0x19,0x29,0x46,0x00}, /* R   */
    {0x26,0x49,0x49,0x49,0x32,0x00}, /* S   */
    {0x01,0x01,0x7F,0x01,0x01,0x00}, /* T   */
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, /* U   */
    {0x0F,0x30,0x40,0x30,0x0F,0x00}, /* V   */
    {0x3F,0x40,0x38,0x40,0x3F,0x00}, /* W   */
    {0x63,0x14,0x08,0x14,0x63,0x00}, /* X   */
    {0x03,0x04,0x78,0x04,0x03,0x00}, /* Y   */
    {0x61,0x51,0x49,0x45,0x43,0x00}, /* Z   */
    {0x00,0x7F,0x41,0x41,0x41,0x00}, /* [   */
    {0x02,0x04,0x08,0x10,0x20,0x00}, /* \   */
    {0x00,0x41,0x41,0x41,0x7F,0x00}, /* ]   */
    {0x04,0x02,0x01,0x02,0x04,0x00}, /* ^   */
    {0x80,0x80,0x80,0x80,0x80,0x00}, /* _   */
    {0x00,0x03,0x07,0x00,0x00,0x00}, /* `   */
    {0x20,0x54,0x54,0x54,0x78,0x00}, /* a   */
    {0x7F,0x48,0x44,0x44,0x38,0x00}, /* b   */
    {0x38,0x44,0x44,0x44,0x28,0x00}, /* c   */
    {0x38,0x44,0x44,0x48,0x7F,0x00}, /* d   */
    {0x38,0x54,0x54,0x54,0x18,0x00}, /* e   */
    {0x00,0x08,0x7E,0x09,0x02,0x00}, /* f   */
    {0x18,0xA4,0xA4,0xA4,0x7C,0x00}, /* g   */
    {0x7F,0x08,0x04,0x04,0x78,0x00}, /* h   */
    {0x00,0x44,0x7D,0x40,0x00,0x00}, /* i   */
    {0x00,0x80,0x84,0x7D,0x00,0x00}, /* j   */
    {0x7F,0x10,0x28,0x44,0x00,0x00}, /* k   */
    {0x00,0x41,0x7F,0x40,0x00,0x00}, /* l   */
    {0x7C,0x04,0x18,0x04,0x78,0x00}, /* m   */
    {0x7C,0x08,0x04,0x04,0x78,0x00}, /* n   */
    {0x38,0x44,0x44,0x44,0x38,0x00}, /* o   */
    {0xFC,0x24,0x24,0x24,0x18,0x00}, /* p   */
    {0x18,0x24,0x24,0x18,0xFC,0x00}, /* q   */
    {0x7C,0x08,0x04,0x04,0x08,0x00}, /* r   */
    {0x48,0x54,0x54,0x54,0x24,0x00}, /* s   */
    {0x04,0x04,0x3F,0x44,0x24,0x00}, /* t   */
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, /* u   */
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, /* v   */
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, /* w   */
    {0x44,0x28,0x10,0x28,0x44,0x00}, /* x   */
    {0x1C,0xA0,0xA0,0xA0,0x7C,0x00}, /* y   */
    {0x44,0x64,0x54,0x4C,0x44,0x00}, /* z   */
    {0x08,0x36,0x41,0x41,0x00,0x00}, /* {   */
    {0x00,0x00,0x77,0x00,0x00,0x00}, /* |   */
    {0x00,0x41,0x41,0x36,0x08,0x00}, /* }   */
    {0x08,0x04,0x08,0x10,0x08,0x00}, /* ~   */
};

/* ════════════════════════════ 字符显示 ════════════════════════════ */

void TFTLCD_ShowChar(uint16_t x, uint16_t y, char ch, uint8_t size, uint16_t color)
{
    if (ch < ' ' || ch > '~') ch = ' ';
    uint8_t idx = ch - ' ';

    uint8_t scale = (size >= 16) ? 2 : (size >= 12) ? 2 : 1;
    uint8_t disp_h = scale * 8;

    TFTLCD_Fill(x, y, x + 6 * scale - 1, y + disp_h - 1, tft_bg_color);

    for (uint8_t c = 0; c < 6; c++) {
        uint8_t d = font_6x8[idx][c];
        for (uint8_t r = 0; r < 8; r++) {
            if (d & (1 << r)) {
                if (scale == 1)
                    TFTLCD_DrawPoint(x + c, y + r, color);
                else
                    TFTLCD_Fill(x + c * scale, y + r * scale,
                                x + c * scale + scale - 1,
                                y + r * scale + scale - 1, color);
            }
        }
    }
}

void TFTLCD_ShowString(uint16_t x, uint16_t y, const char *str, uint8_t size, uint16_t color)
{
    uint8_t w = (size >= 16) ? 12 : (size >= 12) ? 12 : 6;
    while (*str) {
        if (x + w > tftlcd.width) { x = 0; y += size; }
        if (y + size > tftlcd.height) break;
        TFTLCD_ShowChar(x, y, *str, size, color);
        x += w;
        str++;
    }
}

void TFTLCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t w = (size >= 16) ? 12 : (size >= 12) ? 12 : 6;
    uint8_t first = 1;
    for (uint8_t i = 0; i < len; i++) {
        uint32_t p = 1; for (uint8_t j = 0; j < len - i - 1; j++) p *= 10;
        uint8_t digit = (num / p) % 10;
        if (first && i < len - 1 && digit == 0)
            TFTLCD_ShowChar(x + w * i, y, ' ', size, color);
        else {
            TFTLCD_ShowChar(x + w * i, y, '0' + digit, size, color);
            first = 0;
        }
    }
}

#include <stdio.h>
#include <stdarg.h>

void TFTLCD_Printf(uint16_t x, uint16_t y, uint8_t size, uint16_t color, const char *fmt, ...)
{
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    TFTLCD_ShowString(x, y, buf, size, color);
}

/* ════════════════════════════ XPT2046 触摸 (合并 tft_lcd_touch) ════════════════════════════ */

TouchData touch;

/* 引脚操作 (main.h 定义的宏) */
#define TCS_L()   HAL_GPIO_WritePin(T_CS_GPIO_Port,   T_CS_Pin,   GPIO_PIN_RESET)
#define TCS_H()   HAL_GPIO_WritePin(T_CS_GPIO_Port,   T_CS_Pin,   GPIO_PIN_SET)
#define TCLK_H()  HAL_GPIO_WritePin(T_SCLK_GPIO_Port, T_SCLK_Pin, GPIO_PIN_SET)
#define TCLK_L()  HAL_GPIO_WritePin(T_SCLK_GPIO_Port, T_SCLK_Pin, GPIO_PIN_RESET)
#define TMOSI_H() HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_SET)
#define TMOSI_L() HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_RESET)
#define TMISO()   HAL_GPIO_ReadPin(T_MISO_GPIO_Port,  T_MISO_Pin)
#define TPEN()    HAL_GPIO_ReadPin(T_PEN_GPIO_Port,   T_PEN_Pin)

static uint16_t tspi_read16(uint8_t cmd)
{
    uint16_t val = 0;
    TCS_L();
    for (uint8_t i = 0; i < 8; i++) {
        if (cmd & 0x80) TMOSI_H(); else TMOSI_L();
        TCLK_H(); TCLK_L();
        cmd <<= 1;
    }
    TCLK_H(); TCLK_L();  /* busy clock */
    for (uint8_t i = 0; i < 12; i++) {
        TCLK_H();
        val = (val << 1) | (TMISO() ? 1 : 0);
        TCLK_L();
    }
    TCS_H();
    return val;
}

static uint16_t tspi_read_avg(uint8_t cmd, uint8_t times)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < times; i++) sum += tspi_read16(cmd);
    return (uint16_t)(sum / times);
}

void Touch_Init(void)
{
    touch.x = touch.y = 0;
    touch.pressed = 0;
}

uint8_t Touch_Scan(void)
{
    if (TPEN()) { touch.pressed = 0; return 0; }

    uint16_t x = tspi_read_avg(0xD0, 5);
    uint16_t y = tspi_read_avg(0x90, 5);

    if (TPEN()) { touch.pressed = 0; return 0; }

    touch.x = x;
    touch.y = y;
    touch.pressed = 1;
    return 1;
}

void Touch_GetXY(uint16_t *x, uint16_t *y)
{
    enum {
        RX_MIN = 730,  RX_MAX = 3405,
        RY0 = 3516, RY1 = 2759, RY2 = 2008, RY3 = 1253, RY4 = 1058
    };
    int32_t lx = (int32_t)(RX_MAX - touch.y) * (tftlcd.width - 1) / (RX_MAX - RX_MIN);
    int32_t ly;
    if (touch.y >= RY1)      ly = (int32_t)(RY0 - touch.y) * 120 / (RY0 - RY1);
    else if (touch.y >= RY2) ly = 120 + (int32_t)(RY1 - touch.y) * 120 / (RY1 - RY2);
    else if (touch.y >= RY3) ly = 240 + (int32_t)(RY2 - touch.y) * 120 / (RY2 - RY3);
    else                     ly = 360 + (int32_t)(RY3 - touch.y) * 119 / (RY3 - RY4);
    if (lx < 0) lx = 0; if (lx >= (int32_t)tftlcd.width)  lx = tftlcd.width - 1;
    if (ly < 0) ly = 0; if (ly >= (int32_t)tftlcd.height) ly = tftlcd.height - 1;
    *x = (uint16_t)lx;
    *y = (uint16_t)ly;
}

uint8_t Touch_IsPressed(void)
{
    return touch.pressed;
}
