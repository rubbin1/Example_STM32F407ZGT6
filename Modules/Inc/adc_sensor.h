//
// Created by 35156 on 2026/6/17.
//

#ifndef EXAMPLE_STM32F407ZGT6_ADC_SENSOR_H
#define EXAMPLE_STM32F407ZGT6_ADC_SENSOR_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct
{
    ADC_HandleTypeDef *hadc;
    uint32_t channel;           // ADC_CHANNEL_x
    uint16_t raw;               // 最近一次原始值
    uint16_t voltage_mv;        // 最近一次电压值(mV)
    uint16_t avg_raw;           // 滑动平均值
} ADC_CH;
extern ADC_CH adc1_ch5;

void ADC_Init(ADC_CH *ch);
uint16_t ADC_ReadRaw(ADC_CH *ch);
uint16_t ADC_ReadVoltage(ADC_CH *ch);
uint16_t ADC_ReadAvg(ADC_CH *ch, uint8_t times);

#endif //EXAMPLE_STM32F407ZGT6_ADC_SENSOR_H
