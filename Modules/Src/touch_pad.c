//
// Created by 35156 on 2026/6/17.
//

#include "touch_pad.h"
#include "main.h"

#define DISCHARGE_MS   5       // 放电时间
#define TOUCH_FACTOR   1.4f    // 触摸阈值倍数

TOUCH_PAD touch_pad = {
    .pin = TOUCH_KEY_Pin,
    .port = TOUCH_KEY_GPIO_Port,
};

static void Pin_Out_High(TOUCH_PAD *tp)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = tp->pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(tp->port, &g);
    HAL_GPIO_WritePin(tp->port, tp->pin, GPIO_PIN_SET);
}

static void Pin_Out_Low(TOUCH_PAD *tp)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = tp->pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(tp->port, &g);
    HAL_GPIO_WritePin(tp->port, tp->pin, GPIO_PIN_RESET);
}

static void Pin_In_Float(TOUCH_PAD *tp)
{
    GPIO_InitTypeDef g = {0};
    g.Pin  = tp->pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(tp->port, &g);
}

// 放电 → 浮空输入 → 电容靠引脚泄漏电流慢速充电 → 计充电时间
static uint16_t Measure_Charge(TOUCH_PAD *tp)
{
    uint16_t count = 0;

    // 1. 彻底放电
    Pin_Out_Low(tp);
    HAL_Delay(DISCHARGE_MS);

    // 2. 浮空输入，电容通过引脚保护二极管极慢充电
    Pin_In_Float(tp);

    // 3. 计数直到引脚变高
    while (HAL_GPIO_ReadPin(tp->port, tp->pin) == GPIO_PIN_RESET) {
        count++;
        if (count > 0xFFFF) break;
    }
    return count;
}

static uint16_t Measure_Avg(TOUCH_PAD *tp, uint8_t times)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < times; i++) {
        sum += Measure_Charge(tp);
    }
    return (uint16_t)(sum / times);
}

void TouchPad_Init(TOUCH_PAD *tp)
{
    // 预充电 → 放电，建立初始状态
    Pin_Out_High(tp);
    HAL_Delay(DISCHARGE_MS);
    Pin_Out_Low(tp);
    HAL_Delay(DISCHARGE_MS);

    // 采样建立基准
    tp->baseline = Measure_Avg(tp, 10);
    tp->pressed = 0;
}

void TouchPad_Scan(TOUCH_PAD *tp)
{
    uint16_t val = Measure_Charge(tp);

    if (val > (uint16_t)(tp->baseline * TOUCH_FACTOR) && val > tp->baseline + 10) {
        tp->pressed = 1;
        tp->baseline = (tp->baseline * 7 + val) / 8;
    } else {
        tp->pressed = 0;
        tp->baseline = (tp->baseline * 15 + val) / 16;
    }
}

uint8_t TouchPad_IsPressed(TOUCH_PAD *tp)
{
    return tp->pressed;
}
