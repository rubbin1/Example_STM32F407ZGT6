#include "../Inc/page_manager.h"
#include "tft_lcd.h"
#include "font_flash.h"
#include "app_peripheral.h"
#include <string.h>

/* ── 16x16 图标位图 (MSB per byte, 32 bytes each) ── */

static const uint8_t icon_folder[] = {
    0x00,0x00, 0x7F,0x00, 0x40,0xFE, 0x40,0x01,
    0x40,0x01, 0x40,0x01, 0x40,0x01, 0x40,0x01,
    0x40,0x01, 0x40,0x01, 0x40,0x01, 0x40,0x01,
    0x7F,0xFE, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

static const uint8_t icon_gear[] = {
    0x00,0x00, 0x0F,0xF0, 0x10,0x08, 0x19,0x98,
    0x12,0x48, 0x24,0x24, 0x24,0x24, 0x24,0x24,
    0x24,0x24, 0x24,0x24, 0x12,0x48, 0x19,0x98,
    0x10,0x08, 0x0F,0xF0, 0x00,0x00, 0x00,0x00,
};

static const uint8_t icon_thermo[] = {
    0x00,0x00, 0x06,0x00, 0x0F,0x00, 0x06,0x00,
    0x06,0x00, 0x06,0x00, 0x06,0x00, 0x06,0x00,
    0x06,0x00, 0x0F,0x00, 0x13,0x00, 0x20,0xC0,
    0x40,0x40, 0x40,0x40, 0x3F,0x00, 0x00,0x00,
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

static void File_Loop(void) {}

static void Peripheral_Init(void)  { App_Peripheral_Init(); }
static void Peripheral_Loop(void)  { App_Peripheral_Loop(); }

static void Sensor_Init(void)
{
    FontFlash_ShowString_UTF8(10, 10, "传感器数据", 24, TFT_WHITE);
}

static void Sensor_Loop(void) {}

/* ── 页面注册 ── */

static Page pages[PAGE_COUNT] = {
    {"File",     File_Init,       File_Loop},
    {"Periph",   Peripheral_Init, Peripheral_Loop},
    {"Sensor",   Sensor_Init,     Sensor_Loop},
};

static const uint8_t *icons[PAGE_COUNT] = {
    icon_folder, icon_gear, icon_thermo,
};

static PageID cur_page = PAGE_SENSOR;

#define NAV_H      35
#define BTN_W      (tftlcd.width / PAGE_COUNT)

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
        draw_icon(bx + (BTN_W - 16) / 2, nav_y + 2, icons[i], fg, bg);

        // 文字
        uint8_t tw = strlen(pages[i].label) * 12;
        TFTLCD_ShowString(bx + (BTN_W - tw) / 2, nav_y + 20,
                          pages[i].label, 16, fg);
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
    if (!Touch_Scan()) return;

    uint16_t tx, ty;
    Touch_GetXY(&tx, &ty);

    // 检查是否点击了导航栏
    if (ty >= tftlcd.height - NAV_H) {
        uint8_t idx = tx / BTN_W;
        if (idx < PAGE_COUNT && idx != cur_page) {
            cur_page = idx;
            TFTLCD_Clear(TFT_BLACK);
            DrawNav();
            pages[cur_page].init();
        }
        return;
    }

    // 传给当前页面
    pages[cur_page].loop();
}
