//
// Created by 35156 on 2026/6/17.
// 电容触摸按键 — 基于 ADC_Sensor
//

#ifndef EXAMPLE_STM32F407ZGT6_TOUCH_PAD_H
#define EXAMPLE_STM32F407ZGT6_TOUCH_PAD_H
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "soft_timer.h"
#include "adc_dev.h"

typedef enum {
    TPAD_EVENT_NONE = 0,
    TPAD_EVENT_SHORT,
    TPAD_EVENT_LONG,
} TPAD_Event;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    ADC_CH       *adc;               // 绑定到 ADC 通道
    uint16_t      baseline;          // 基准 ADC 值
    uint8_t       pressing;
    uint8_t       debounce;
    uint8_t       release_cnt;
    TPAD_Event    event;
    SOFT_TIMER    hold_timer;
    uint8_t       long_triggered;
} TOUCH_PAD;

extern TOUCH_PAD touch_pad;

void    TouchPad_Init(TOUCH_PAD *tp);
void    TouchPad_Scan(TOUCH_PAD *tp);
uint8_t TouchPad_IsShortPressed(TOUCH_PAD *tp);
uint8_t TouchPad_IsLongPressed(TOUCH_PAD *tp);

#endif
