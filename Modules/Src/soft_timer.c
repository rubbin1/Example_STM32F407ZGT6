//
// Created by 35156 on 2026/5/25.
//

#include "soft_timer.h"

void SoftTimer_Start(SOFT_TIMER *t, uint32_t interval_ms)
{
    t->interval = interval_ms;
    t->start = HAL_GetTick();
}

void SoftTimer_Reset(SOFT_TIMER *t)
{
    t->start = HAL_GetTick();
}

bool SoftTimer_Expired(SOFT_TIMER *t)
{
    if (HAL_GetTick() - t->start >= t->interval) {
        t->start = HAL_GetTick();
        return true;
    }
    return false;
}

bool SoftTimer_Elapsed(SOFT_TIMER *t)
{
    return (HAL_GetTick() - t->start >= t->interval);
}