//
// Created by 35156 on 2026/5/25.
//

#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

typedef struct {
    uint32_t start;
    uint32_t interval;
} SOFT_TIMER;

// 启动/重设定时器，设置间隔并记录起始时间
void SoftTimer_Start(SOFT_TIMER *t, uint32_t interval_ms);

// 检查定时器是否到期，到期自动重置并返回true
bool SoftTimer_Expired(SOFT_TIMER *t);

// 仅重置起始时间，不改变间隔
void SoftTimer_Reset(SOFT_TIMER *t);

// 仅检查是否到期，不重置（用于反复查询同一事件的场景）
bool SoftTimer_Elapsed(SOFT_TIMER *t);

#endif // SOFT_TIMER_H
