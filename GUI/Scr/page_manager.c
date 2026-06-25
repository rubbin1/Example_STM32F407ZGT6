#include "../Inc/page_manager.h"
#include "tft_lcd.h"
#include "font_flash.h"
#include "app_peripheral.h"
#include "app_debug.h"
#include <string.h>

/* ── 16x16 图标位图 (MSB per byte, 32 bytes each) ── */

// 文件管理器（文件夹 + 内部文件）
static const uint8_t icon_folder[] = {
    0x00,0x00, 0x1C,0x00, 0x7F,0xFE, 0x40,0x02,
    0x40,0x02, 0x40,0x02, 0x5F,0xFA, 0x50,0x0A,
    0x4F,0xEA, 0x47,0xFA, 0x5F,0xFA, 0x40,0x02,
    0x40,0x02, 0x40,0x02, 0x7F,0xFE, 0x00,0x00,
};

// 硬件外设控制（芯片 IC）
static const uint8_t icon_hardware[] = {
    0x0A,0xA0, 0x0A,0xA0, 0x00,0x00, 0x1F,0xF8,
    0x10,0x08, 0xF0,0x0F, 0x10,0x08, 0xF0,0x0F,
    0x10,0x08, 0xF0,0x0F, 0x10,0x08, 0xF0,0x0F,
    0x1F,0xF8, 0x0A,0xA0, 0x0A,0xA0, 0x00,0x00,
};

// 传感器数据（折线图 + 坐标轴 + 数据标记）
static const uint8_t icon_sensor[] = {
    0x00,0x00, 0x00,0x00, 0x40,0x0C, 0x40,0x0C,
    0x40,0x1C, 0x40,0x20, 0x40,0x40, 0x40,0x80,
    0x41,0x00, 0x42,0x00, 0x44,0x00, 0x48,0x00,
    0x50,0x00, 0x60,0x00, 0x7F,0xFF, 0x00,0x00,
};

// 调试/错误排查（小虫子 Bug）
static const uint8_t icon_debug[] = {
    0x00,0x00, 0x10,0x08, 0x2C,0x34, 0x7C,0x3E,
    0x5E,0x7A, 0x3E,0x7C, 0x1F,0xF8, 0x0F,0xF0,
    0x07,0xE0, 0x06,0x60, 0x02,0x40, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

static void draw_icon(uint16_t x, uint16_t y, const uint8_t *bm,
                      uint16_t color, uint16_t bg)
{
    for (uint8_t row = 0; row < 16; row++) {
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t byte = bm[row * 2 + col / 8];
            uint8_t bit  = (byte >> (7 - (col % 8))) & 1;
            TFTLCD_DrawPoint(x + col, y + row, bit ? color : bg);
        }
    }
}

/* ── 占位页面 ── */

static void File_Init(void)
{
    FontFlash_ShowString_UTF8(10, 10, "文件管理器", 24, TFT_WHITE);
}

static void File_Loop(uint16_t x, uint16_t y, uint8_t pressed) { (void)x; (void)y; (void)pressed; }

static void Peripheral_Init(void)  { App_Peripheral_Hardware_Init(); }
static void Peripheral_Loop(uint16_t x, uint16_t y, uint8_t pressed)  { App_Peripheral_Hardware_Loop(x, y, pressed); }

static void Sensor_Init(void)
{
    FontFlash_ShowString_UTF8(10, 10, "传感器数据", 24, TFT_WHITE);
}

static void Sensor_Loop(uint16_t x, uint16_t y, uint8_t pressed) { (void)x; (void)y; (void)pressed; }

static void Debug_Init(void)  { App_Debug_Init(); }
static void Debug_Loop(uint16_t x, uint16_t y, uint8_t pressed)  { App_Debug_Loop(x, y, pressed); }

/* ── 页面注册 ── */

static Page pages[PAGE_COUNT] = {
    {"File",     File_Init,       File_Loop},
    {"Periph",   Peripheral_Init, Peripheral_Loop},
    {"Sensor",   Sensor_Init,     Sensor_Loop},
    {"Debug",    Debug_Init,       Debug_Loop},
};

static const uint8_t *icons[PAGE_COUNT] = {
    icon_folder, icon_hardware, icon_sensor, icon_debug,
};

static PageID cur_page = PAGE_FILE;

/* ── 绘制底部导航栏 ── */
static void DrawNav(void)
{
    uint16_t nav_y = tftlcd.height - NAV_H;

    TFTLCD_DrawLine(0, nav_y, tftlcd.width - 1, nav_y, TFT_WHITE);

    for (uint8_t i = 0; i < PAGE_COUNT; i++) {
        uint16_t bx = i * BTN_W;
        uint16_t bg = (i == cur_page) ? TFT_BLUE : TFT_DARKBLUE;
        uint16_t fg = (i == cur_page) ? TFT_WHITE : TFT_GRAY;
        TFTLCD_Fill(bx, nav_y + 1, bx + BTN_W - 1, tftlcd.height - 1, bg);

        // 图标居中 16x16
        draw_icon(bx + (BTN_W - 16) / 2, nav_y + 17, icons[i], fg, bg);
    }
}

/* ════════════════════════════ 对外接口 ════════════════════════════ */

void PageManager_Init(void)
{
    TFTLCD_Clear(TFT_BLACK);
    DrawNav();
    pages[cur_page].init();
}

void PageManager_Loop(void)
{
    uint8_t pressed = Touch_Scan();
    uint16_t tx = 0, ty = 0;
    if (pressed) Touch_GetXY(&tx, &ty);

    // 检查是否点击了导航栏
    if (pressed && ty >= tftlcd.height - NAV_H) {
        uint8_t idx = tx / BTN_W;
        if (idx < PAGE_COUNT && idx != cur_page) {
            cur_page = idx;
            TFTLCD_Clear(TFT_BLACK);
            DrawNav();
            pages[cur_page].init();
        }
    }

    pages[cur_page].loop(tx, ty, pressed);
}

uint8_t PageManager_IsPage(PageID id)
{
    return cur_page == id;
}
