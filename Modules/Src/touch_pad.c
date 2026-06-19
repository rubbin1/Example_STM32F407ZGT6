//
// Created by 35156 on 2026/6/17.
// TIM6 精确定时版，支持短按/长按
//

#include "touch_pad.h"
#include "main.h"
#include "hw_timer.h"

#define DISCHARGE_MS   5
#define TOUCH_FACTOR   1.4f
#define LONG_PRESS_MS  1000

TOUCH_PAD touch_pad = {
    .pin = TOUCH_KEY_Pin,
    .port = TOUCH_KEY_GPIO_Port,
};

static void Pin_Out_High(TOUCH_PAD *tp)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = tp->pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(tp->port, &g);
    HAL_GPIO_WritePin(tp->port, tp->pin, GPIO_PIN_SET);
}

static void Pin_Out_Low(TOUCH_PAD *tp)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = tp->pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(tp->port, &g);
    HAL_GPIO_WritePin(tp->port, tp->pin, GPIO_PIN_RESET);
}

static void Pin_In_Float(TOUCH_PAD *tp)
{
    GPIO_InitTypeDef g = {0};
    g.Pin  = tp->pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(tp->port, &g);
}

static uint16_t Measure_Charge(TOUCH_PAD *tp)
{
    Pin_Out_Low(tp);
    HAL_Delay(DISCHARGE_MS);
    Pin_In_Float(tp);

    HwTimer_Reset(&hwtim6);
    HwTimer_Start(&hwtim6);
    while (HAL_GPIO_ReadPin(tp->port, tp->pin) == GPIO_PIN_RESET) {
        if (HwTimer_GetCount(&hwtim6) > 900) break;
    }
    uint16_t cnt = HwTimer_GetCount(&hwtim6);
    HwTimer_Stop(&hwtim6);
    return cnt;
}

static uint16_t Measure_Avg(TOUCH_PAD *tp, uint8_t times)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < times; i++)
        sum += Measure_Charge(tp);
    return (uint16_t)(sum / times);
}

void TouchPad_Init(TOUCH_PAD *tp)
{
    Pin_Out_High(tp);
    HAL_Delay(DISCHARGE_MS);
    Pin_Out_Low(tp);
    HAL_Delay(DISCHARGE_MS);

    tp->baseline = Measure_Avg(tp, 10);
    tp->pressing = 0;
    tp->debounce = 0;
    tp->event = TPAD_EVENT_NONE;
}

void TouchPad_Scan(TOUCH_PAD *tp)
{
    uint16_t val = Measure_Charge(tp);
    uint8_t  touched = (val > (uint16_t)(tp->baseline * TOUCH_FACTOR) && val > tp->baseline + 10);

    tp->event = TPAD_EVENT_NONE;

    if (touched) {
        tp->release_cnt = 0;                                          // 清零松手计数
        tp->baseline = (tp->baseline * 7 + val) / 8;

        if (tp->debounce > 0) return;                                 // 冷却中，不产生事件

        if (!tp->pressing) {                                          // 刚触摸
            tp->pressing = 1;
            tp->long_triggered = 0;
            SoftTimer_Start(&tp->hold_timer, LONG_PRESS_MS);
        } else if (!tp->long_triggered && SoftTimer_Expired(&tp->hold_timer)) {
            tp->long_triggered = 1;                                   // 长按
            tp->event = TPAD_EVENT_LONG;
        }
    } else {
        if (tp->pressing) {
            tp->release_cnt++;
            if (tp->release_cnt >= 3) {                               // 连续3次未触摸才判松手
                if (!tp->long_triggered)
                    tp->event = TPAD_EVENT_SHORT;                     // 短按
                tp->pressing = 0;
                tp->debounce = 5;                                     // 冷却
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
