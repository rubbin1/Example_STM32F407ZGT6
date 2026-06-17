//
// Created by 35156 on 2026/6/2.
//

#ifndef EXAMPLE_STM32F407ZGT6_LIGHT_SENSOR_H
#define EXAMPLE_STM32F407ZGT6_LIGHT_SENSOR_H
#include <stdint.h>

#include "stm32f407xx.h"

typedef enum
{
    DARK,
    LIGHT,
} LIGHT_FLAG;

typedef struct
{
    uint16_t pin;
    GPIO_TypeDef *port;
    uint8_t dark_level;         // 什么电平算"暗"：0=低电平暗，1=高电平暗
    LIGHT_FLAG light_flag;
} LIGHT_SENSOR;
extern LIGHT_SENSOR light_sensor;

void LightSensor_Init(LIGHT_SENSOR *lts);
void LightSensor_Scan(LIGHT_SENSOR *lts);
uint8_t LightSensor_IsDark(LIGHT_SENSOR *lts);
uint8_t LightSensor_IsLight(LIGHT_SENSOR *lts);

#endif //EXAMPLE_STM32F407ZGT6_LIGHT_SENSOR_H
