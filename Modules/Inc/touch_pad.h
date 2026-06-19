//
// Created by 35156 on 2026/6/17.
//

#ifndef EXAMPLE_STM32F407ZGT6_TOUCH_PAD_H
#define EXAMPLE_STM32F407ZGT6_TOUCH_PAD_H
#include <stdint.h>
#include "stm32f407xx.h"
#include "soft_timer.h"

typedef enum {
    TPAD_EVENT_NONE = 0,
    TPAD_EVENT_SHORT,    // 短按（1s 内松开）
    TPAD_EVENT_LONG,     // 长按（超过 1s）
} TPAD_Event;

typedef struct
{
    uint16_t    pin;
    GPIO_TypeDef *port;
    uint16_t    baseline;
    uint8_t     pressing;           // 当前是否正在触摸
    uint8_t     debounce;
    uint8_t     release_cnt;        // 连续未触摸计数（防噪声误判松手）
    TPAD_Event  event;             // 最近事件
    SOFT_TIMER  hold_timer;        // 长按计时器（1s）
    uint8_t     long_triggered;    // 本次触摸是否已触发过长按
} TOUCH_PAD;
extern TOUCH_PAD touch_pad;

void    TouchPad_Init(TOUCH_PAD *tp);
void    TouchPad_Scan(TOUCH_PAD *tp);
uint8_t TouchPad_IsShortPressed(TOUCH_PAD *tp);   // 读取并清除 SHORT 事件
uint8_t TouchPad_IsLongPressed(TOUCH_PAD *tp);    // 读取并清除 LONG 事件

#endif //EXAMPLE_STM32F407ZGT6_TOUCH_PAD_H
