//
// Created by 35156 on 2026/6/19.
// TFT 触摸按钮
//

#include "../Inc/gui_button.h"
#include "tft_lcd.h"
#include <string.h>

void Button_Draw(Button *btn)
{
    // 底色
    TFTLCD_Fill(btn->x, btn->y, btn->x + btn->w, btn->y + btn->h, btn->bg_color);
    // 3D 边框效果
    uint16_t top_color  = btn->bg_color + 0x0841;  // 稍亮
    uint16_t bot_color  = btn->bg_color - 0x0841;  // 稍暗
    if (btn->pressed) {
        // 按下：交换明暗边框 = 凹陷效果
        uint16_t t = top_color; top_color = bot_color; bot_color = t;
    }
    TFTLCD_DrawLine(btn->x, btn->y, btn->x + btn->w, btn->y, top_color);           // 上边亮
    TFTLCD_DrawLine(btn->x, btn->y, btn->x, btn->y + btn->h, top_color);           // 左边亮
    TFTLCD_DrawLine(btn->x, btn->y + btn->h, btn->x + btn->w, btn->y + btn->h, bot_color); // 下边暗
    TFTLCD_DrawLine(btn->x + btn->w, btn->y, btn->x + btn->w, btn->y + btn->h, bot_color); // 右边暗
    // 文字居中
    uint8_t  tw = (16 >= 16) ? 12 : 6;
    uint16_t tx = btn->x + (btn->w - tw * strlen(btn->text)) / 2;
    uint16_t ty = btn->y + (btn->h - 16) / 2;
    TFTLCD_ShowString(tx, ty, btn->text, 16, btn->color);
}

uint8_t Button_Check(Button *btn, uint16_t tx, uint16_t ty)
{
    uint8_t hit = (tx >= btn->x && tx <= btn->x + btn->w &&
                   ty >= btn->y && ty <= btn->y + btn->h);

    if (hit && !btn->pressed) {
        btn->pressed = 1;
        Button_Draw(btn);
        if (btn->on_click) btn->on_click();
        return 1;
    }
    if (!hit && btn->pressed) {
        btn->pressed = 0;
        Button_Draw(btn);
    }
    return 0;
}

/* ════════════════════════════ 复选框 ════════════════════════════ */

void CheckBox_Draw(CheckBox *cb)
{
    // 方框 20×20
    TFTLCD_Fill(cb->x, cb->y, cb->x + 20, cb->y + 20, cb->bg_color);
    TFTLCD_DrawRect(cb->x, cb->y, 20, 20, cb->color);
    if (cb->checked) {
        // 画勾
        TFTLCD_DrawLine(cb->x + 4, cb->y + 10, cb->x + 8,  cb->y + 16, cb->color);
        TFTLCD_DrawLine(cb->x + 8, cb->y + 16, cb->x + 16, cb->y + 4,  cb->color);
    }
    // 文字在方框右边
    TFTLCD_ShowString(cb->x + 26, cb->y + 2, cb->text, 16, cb->color);
}

uint8_t CheckBox_Check(CheckBox *cb, uint16_t tx, uint16_t ty)
{
    uint8_t hit = (tx >= cb->x && tx <= cb->x + 20 + 26 + 12 * 6 &&  // 含文字区域
                   ty >= cb->y && ty <= cb->y + 20);

    if (hit && !cb->pressed) {
        cb->pressed = 1;
        cb->checked = !cb->checked;
        CheckBox_Draw(cb);
        if (cb->on_change) cb->on_change(cb->checked);
        return 1;
    }
    if (!hit) cb->pressed = 0;
    return 0;
}

void CheckBox_SetChecked(CheckBox *cb, uint8_t val)
{
    cb->checked = val ? 1 : 0;
    CheckBox_Draw(cb);
}

/* ════════════════════════════ 水平滑块 ════════════════════════════ */

void Slider_Draw(Slider *s)
{
    // 滑槽背景
    TFTLCD_Fill(s->x, s->y + 8, s->x + s->w, s->y + 12, s->bg_color);
    TFTLCD_DrawRect(s->x, s->y + 8, s->w, 4, s->color);

    // 填充条
    uint16_t fx = (uint32_t)s->value * s->w / 100;
    if (fx > 0) TFTLCD_Fill(s->x, s->y + 8, s->x + fx, s->y + 12, s->color);

    // 滑块圆钮
    TFTLCD_DrawFillCircle(s->x + fx, s->y + 10, 6, s->color);
}

uint8_t Slider_Check(Slider *s, uint16_t tx, uint16_t ty)
{
    uint8_t hit = (tx >= s->x && tx <= s->x + s->w &&
                   ty >= s->y && ty <= s->y + 20);

    if (hit) {
        s->dragging = 1;
        uint8_t v = (uint32_t)(tx - s->x) * 100 / s->w;
        if (v > 100) v = 100;
        if (v != s->value) {
            s->value = v;
            Slider_Draw(s);
            if (s->on_change) s->on_change(s->value);
        }
        return 1;
    }
    s->dragging = 0;
    return 0;
}

void Slider_SetValue(Slider *s, uint8_t val)
{
    if (val > 100) val = 100;
    s->value = val;
    Slider_Draw(s);
}
