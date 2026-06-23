//
// 软件 PWM 舵机驱动（基于 HW_TIMER，不占用硬件 PWM 通道）
//
#ifndef SERVO_H
#define SERVO_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct {
    uint16_t       pin;
    GPIO_TypeDef  *port;
    uint8_t        angle;        // 当前角度 0~180
} Servo;
extern Servo servo1;

void Servo_Init(Servo *s);
void Servo_SetAngle(Servo *s, uint8_t angle);
void Servo_Update(Servo *s);    // 主循环中调用，生成 PWM 波形

#endif
