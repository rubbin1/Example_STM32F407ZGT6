//
// 软件 PWM 舵机驱动（DWT 微秒精确定时）
// 50Hz 周期=20ms, 脉宽 500~2500us 映射到 0°~180°
//
#include "servo_180.h"
#include "main.h"

#define PERIOD_MS      20
#define PULSE_MIN_US   500    // 0°
#define PULSE_MAX_US   2500   // 180°

static uint32_t g_next = 0;

Servo servo1 = {0};

static void DWT_Delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

void Servo_Init(Servo *s)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    GPIO_InitTypeDef g = {0};
    g.Pin   = s->pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(s->port, &g);
    HAL_GPIO_WritePin(s->port, s->pin, GPIO_PIN_RESET);

    s->angle = 90;
    g_next   = HAL_GetTick();
}

void Servo_SetAngle(Servo *s, uint8_t angle)
{
    if (angle > 180) angle = 180;
    s->angle = angle;
}

void Servo_Update(Servo *s)
{
    uint32_t now = HAL_GetTick();
    if (now < g_next) return;

    uint32_t pulse_us = PULSE_MIN_US + (uint32_t)s->angle * (PULSE_MAX_US - PULSE_MIN_US) / 180;

    HAL_GPIO_WritePin(s->port, s->pin, GPIO_PIN_SET);
    DWT_Delay_us(pulse_us);
    HAL_GPIO_WritePin(s->port, s->pin, GPIO_PIN_RESET);

    g_next = now + PERIOD_MS;
}
