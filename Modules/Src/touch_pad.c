//
// Created by 35156 on 2026/6/17.
// 电容触摸按键 — ADC 采样版
//

#include "touch_pad.h"
#include "main.h"

#define DISCHARGE_MS    5
#define CHARGE_US       100
#define TOUCH_THRESH    80      // 触摸时 ADC 下降阈值
#define LONG_PRESS_MS   1000

TOUCH_PAD touch_pad = {
    .port = TOUCH_KEY_GPIO_Port,
    .pin  = TOUCH_KEY_Pin,
    .adc  = &adc1_ch5,
};

static void pad_discharge(TOUCH_PAD *tp)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = tp->pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(tp->port, &g);
    HAL_GPIO_WritePin(tp->port, tp->pin, GPIO_PIN_RESET);
    HAL_Delay(DISCHARGE_MS);
}

static void pad_charge(TOUCH_PAD *tp)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = tp->pin;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(tp->port, &g);
    /* 等待固定充电时间 */
    for (volatile uint16_t i = 0; i < CHARGE_US * 10; i++) { __NOP(); }
}

static uint16_t pad_read(TOUCH_PAD *tp)
{
    pad_discharge(tp);
    pad_charge(tp);
    return ADC_ReadRaw(tp->adc);
}

void TouchPad_Init(TOUCH_PAD *tp)
{
    ADC_Init(tp->adc);
    tp->baseline = 0;
    for (uint8_t i = 0; i < 10; i++) {
        tp->baseline += pad_read(tp);
    }
    tp->baseline /= 10;
    tp->pressing = 0;
    tp->debounce = 0;
    tp->release_cnt = 0;
    tp->event = TPAD_EVENT_NONE;
}

void TouchPad_Scan(TOUCH_PAD *tp)
{
    uint16_t val = pad_read(tp);
    uint8_t  touched = (val + TOUCH_THRESH < tp->baseline);

    tp->event = TPAD_EVENT_NONE;

    if (touched) {
        tp->release_cnt = 0;
        tp->baseline = (tp->baseline * 7 + val) / 8;

        if (tp->debounce > 0) return;

        if (!tp->pressing) {
            tp->pressing = 1;
            tp->long_triggered = 0;
            SoftTimer_Start(&tp->hold_timer, LONG_PRESS_MS);
        } else if (!tp->long_triggered && SoftTimer_Expired(&tp->hold_timer)) {
            tp->long_triggered = 1;
            tp->event = TPAD_EVENT_LONG;
        }
    } else {
        if (tp->pressing) {
            tp->release_cnt++;
            if (tp->release_cnt >= 3) {
                if (!tp->long_triggered)
                    tp->event = TPAD_EVENT_SHORT;
                tp->pressing = 0;
                tp->debounce = 5;
            }
        } else if (tp->debounce > 0) {
            tp->debounce--;
        }
        tp->baseline = (tp->baseline * 15 + val) / 16;
    }
}

uint8_t TouchPad_IsShortPressed(TOUCH_PAD *tp)
{
    if (tp->event == TPAD_EVENT_SHORT) {
        tp->event = TPAD_EVENT_NONE;
        return 1;
    }
    return 0;
}

uint8_t TouchPad_IsLongPressed(TOUCH_PAD *tp)
{
    if (tp->event == TPAD_EVENT_LONG) {
        tp->event = TPAD_EVENT_NONE;
        return 1;
    }
    return 0;
}
