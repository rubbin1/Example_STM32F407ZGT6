//
// Debug 页面 — printf 式屏幕日志
//
#include "app_debug.h"
#include "tft_lcd.h"
#include "page_manager.h"
#include "font_flash.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define DBG_LINES     30
#define DBG_LINE_LEN  50
#define FONT_SIZE     16
#define VISIBLE_LINES ((tftlcd.height - NAV_H - 1) / FONT_SIZE)

static char buf[DBG_LINES][DBG_LINE_LEN];
static uint8_t head = 0;    // 下一条写入行
static uint8_t count = 0;   // 总行数（最大 DBG_LINES）

/* ── 绘制当前可见行 ── */
static void DrawLines(void)
{
    uint16_t y = 0;
    // 计算显示起始行
    uint8_t start = (count <= VISIBLE_LINES) ? 0 : (count - VISIBLE_LINES);
    for (uint8_t i = start; i < count; i++) {
        uint8_t idx = (head - count + i + DBG_LINES) % DBG_LINES;
        TFTLCD_ShowString(0, y, buf[idx], FONT_SIZE, TFT_WHITE);
        y += FONT_SIZE;
        if (y >= tftlcd.height - NAV_H) break;
    }
}

/* ════════════════════════════ 对外接口 ════════════════════════════ */

void App_Debug_Init(void)
{
    TFTLCD_Fill(0, 0, tftlcd.width - 1, tftlcd.height - NAV_H - 1, TFT_BLACK);
    tft_bg_color = TFT_BLACK;
    FontFlash_ShowString_UTF8(10, 10, "Debug与测试界面", 24, TFT_WHITE);
    // 标题占用 34px，正文从 y=36 开始
    uint16_t banner_h = 36;
    // 清除标题以下区域
    TFTLCD_Fill(0, banner_h, tftlcd.width - 1, tftlcd.height - NAV_H - 1, TFT_BLACK);

    DrawLines();
}

void App_Debug_Loop(uint16_t x, uint16_t y, uint8_t pressed)
{
    (void)x; (void)y; (void)pressed;
    // 没有交互，纯显示
}

void Debug_Printf(const char *fmt, ...)
{
    // 永远写入缓冲区
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf[head], DBG_LINE_LEN, fmt, args);
    va_end(args);

    head = (head + 1) % DBG_LINES;
    if (count < DBG_LINES) count++;

    // 当前在 Debug 页才刷新屏幕
    if (PageManager_IsPage(PAGE_DEBUG)) {
        uint16_t banner_h = 36;
        TFTLCD_Fill(0, banner_h, tftlcd.width - 1, tftlcd.height - NAV_H - 1, TFT_BLACK);
        DrawLines();
    }
}

void Debug_Clear(void)
{
    head = 0;
    count = 0;
    memset(buf, 0, sizeof(buf));
    uint16_t banner_h = 26;
    TFTLCD_Fill(0, banner_h, tftlcd.width - 1, tftlcd.height - NAV_H - 1, TFT_BLACK);
}
