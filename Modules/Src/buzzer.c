//
// Created by 35156 on 2026/6/2.
//

#include "buzzer.h"
#include "main.h"
#include "soft_timer.h"

BUZZER buzzer = {
    .pin = BUZZER_Pin,
    .port = BUZZER_GPIO_Port,
    .active_level = GPIO_PIN_SET,
};

void Buzzer_Init(BUZZER *bzr)
{
    Buzzer_Off(bzr);
}

void Buzzer_On(BUZZER *bzr)
{
    HAL_GPIO_WritePin(bzr->port, bzr->pin, bzr->active_level);
    bzr->beeping = 1;
}

void Buzzer_Off(BUZZER *bzr)
{
    HAL_GPIO_WritePin(bzr->port, bzr->pin, !bzr->active_level);
    bzr->beeping = 0;
    bzr->timer.interval = 0;
}

void Buzzer_Toggle(BUZZER *bzr)
{
    HAL_GPIO_TogglePin(bzr->port, bzr->pin);
    bzr->beeping = !bzr->beeping;
}

void Buzzer_Beep(BUZZER *bzr, uint32_t duration_ms)
{
    Buzzer_On(bzr);
    SoftTimer_Start(&bzr->timer, duration_ms);
}

void Buzzer_Loop(BUZZER *bzr)
{
    if (bzr->beeping && bzr->timer.interval > 0 && SoftTimer_Elapsed(&bzr->timer)) {
        Buzzer_Off(bzr);
    }
}
