//
// Created by 35156 on 2026/6/2.
//

#ifndef EXAMPLE_STM32F407ZGT6_BUZZER_H
#define EXAMPLE_STM32F407ZGT6_BUZZER_H
#include <stdint.h>

#include "stm32f407xx.h"
#include "soft_timer.h"

typedef struct
{
    uint16_t pin;
    GPIO_TypeDef *port;
    uint8_t active_level;       // 鸣响电平，1=高电平响，0=低电平响
    SOFT_TIMER timer;           // 控制响铃时长
    uint8_t beeping;            // 是否正在鸣响
} BUZZER;
extern BUZZER buzzer;

void Buzzer_Init(BUZZER *bzr);
void Buzzer_On(BUZZER *bzr);
void Buzzer_Off(BUZZER *bzr);
void Buzzer_Toggle(BUZZER *bzr);
void Buzzer_Beep(BUZZER *bzr, uint32_t duration_ms);   // 响指定毫秒后自动停
void Buzzer_Loop(BUZZER *bzr);                          // 主循环中调用，检查是否该停

#endif //EXAMPLE_STM32F407ZGT6_BUZZER_H
