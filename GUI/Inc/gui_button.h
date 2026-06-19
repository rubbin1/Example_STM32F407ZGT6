//
// Created by 35156 on 2026/6/19.
// TFT 触摸 GUI 组件：按钮 / 复选框 / 滑块
//

#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H
#include <stdint.h>
#include "main.h"

/* ── 回调类型 ── */
typedef void (*BtnCallback)(void);
typedef void (*CheckCallback)(uint8_t checked);
typedef void (*SliderCallback)(uint8_t value);

/* ── 按钮 ── */
typedef struct {
    uint16_t x, y, w, h;
    const char *text;
    uint16_t color;
    uint16_t bg_color;
    uint8_t  pressed;
    BtnCallback on_click;
} Button;
void Button_Draw(Button *btn);
uint8_t Button_Check(Button *btn, uint16_t tx, uint16_t ty);

/* ── 复选框 ── */
typedef struct {
    uint16_t x, y;
    const char *text;
    uint16_t color;
    uint16_t bg_color;
    uint8_t  checked;             // 0=未选, 1=已选
    uint8_t  pressed;             // 内部消抖
    CheckCallback on_change;      // 状态改变时回调
} CheckBox;
void CheckBox_Draw(CheckBox *cb);
uint8_t CheckBox_Check(CheckBox *cb, uint16_t tx, uint16_t ty);
void CheckBox_SetChecked(CheckBox *cb, uint8_t val);  // 外部设置状态

/* ── 水平滑块 ── */
typedef struct {
    uint16_t x, y, w;             // 位置和宽度（高度固定 20px）
    uint16_t color;               // 填充色
    uint16_t bg_color;            // 底色
    uint8_t  value;               // 0~100
    uint8_t  dragging;            // 内部：是否拖动中
    SliderCallback on_change;     // 值改变时回调
} Slider;
void Slider_Draw(Slider *s);
uint8_t Slider_Check(Slider *s, uint16_t tx, uint16_t ty);
void Slider_SetValue(Slider *s, uint8_t val);           // 外部设置值

#endif //GUI_BUTTON_H
