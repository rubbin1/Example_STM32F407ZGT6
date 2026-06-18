//
// Created by 35156 on 2026/6/18.
//

#ifndef HW_TIMER_H
#define HW_TIMER_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t           freq_hz;   // 计数器时钟频率
} HW_TIMER;

extern HW_TIMER hwtim6;

void HwTimer_Init(HW_TIMER *t);
void HwTimer_DelayUs(HW_TIMER *t, uint16_t us);
void HwTimer_DelayMs(HW_TIMER *t, uint16_t ms);
void HwTimer_Start(HW_TIMER *t);
void HwTimer_Stop(HW_TIMER *t);
void HwTimer_Reset(HW_TIMER *t);
uint16_t HwTimer_GetCount(HW_TIMER *t);

#endif //HW_TIMER_H
