//
// Created by 35156 on 2026/6/17.
//

#ifndef EXAMPLE_STM32F407ZGT6_TOUCH_PAD_H
#define EXAMPLE_STM32F407ZGT6_TOUCH_PAD_H
#include <stdint.h>
#include "stm32f407xx.h"

typedef struct
{
    uint16_t pin;
    GPIO_TypeDef *port;
    uint16_t baseline;          // 未触摸时的充电时间基准
    uint8_t pressed;            // 当前是否按下
} TOUCH_PAD;
extern TOUCH_PAD touch_pad;

void TouchPad_Init(TOUCH_PAD *tp);
void TouchPad_Scan(TOUCH_PAD *tp);
uint8_t TouchPad_IsPressed(TOUCH_PAD *tp);

#endif //EXAMPLE_STM32F407ZGT6_TOUCH_PAD_H
