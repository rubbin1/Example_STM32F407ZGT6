//
// Created by 35156 on 2026/6/18.
//

#include "hw_timer.h"
#include "main.h"

HW_TIMER hwtim6 = {
    .htim = &htim6,
};

void HwTimer_Init(HW_TIMER *t)
{
    uint32_t clk = HAL_RCC_GetPCLK1Freq() * 2;  // TIM 在 APB1，x2
    t->freq_hz = clk / (t->htim->Init.Prescaler + 1);
}

void HwTimer_DelayUs(HW_TIMER *t, uint16_t us)
{
    uint16_t ticks = (uint16_t)(((uint32_t)us * t->freq_hz) / 1000000);
    if (ticks == 0) ticks = 1;

    __HAL_TIM_SET_COUNTER(t->htim, 0);
    HwTimer_Start(t);
    while (__HAL_TIM_GET_COUNTER(t->htim) < ticks);
    HwTimer_Stop(t);
}

void HwTimer_DelayMs(HW_TIMER *t, uint16_t ms)
{
    while (ms--) {
        HwTimer_DelayUs(t, 1000);
    }
}

void HwTimer_Start(HW_TIMER *t)
{
    HAL_TIM_Base_Start(t->htim);
}

void HwTimer_Stop(HW_TIMER *t)
{
    HAL_TIM_Base_Stop(t->htim);
}

void HwTimer_Reset(HW_TIMER *t)
{
    __HAL_TIM_SET_COUNTER(t->htim, 0);
}

uint16_t HwTimer_GetCount(HW_TIMER *t)
{
    return __HAL_TIM_GET_COUNTER(t->htim);
}
