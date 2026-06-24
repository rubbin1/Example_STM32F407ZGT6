//
// Created by 35156 on 2026/6/17.
//

#include "adc_dev.h"
#include "adc.h"

#define ADC_TIMEOUT_MS  10
#define VREF_MV         3300
#define ADC_RES_12BIT   4096

ADC_CH adc1_ch5 = {
    .hadc = &hadc1,
    .channel = ADC_CHANNEL_5,
};

static uint16_t ReadOnce(ADC_CH *ch)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = ch->channel;
    sConfig.Rank          = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

    HAL_ADC_ConfigChannel(ch->hadc, &sConfig);
    HAL_ADC_Start(ch->hadc);
    HAL_ADC_PollForConversion(ch->hadc, ADC_TIMEOUT_MS);
    uint16_t val = HAL_ADC_GetValue(ch->hadc);
    HAL_ADC_Stop(ch->hadc);
    return val;
}

void ADC_Init(ADC_CH *ch)
{
    ch->raw         = ReadOnce(ch);
    ch->voltage_mv  = (uint32_t)ch->raw * VREF_MV / ADC_RES_12BIT;
    ch->avg_raw     = ch->raw;
}

uint16_t ADC_ReadRaw(ADC_CH *ch)
{
    ch->raw = ReadOnce(ch);
    return ch->raw;
}

uint16_t ADC_ReadVoltage(ADC_CH *ch)
{
    ch->raw        = ReadOnce(ch);
    ch->voltage_mv = (uint32_t)ch->raw * VREF_MV / ADC_RES_12BIT;
    return ch->voltage_mv;
}

uint16_t ADC_ReadAvg(ADC_CH *ch, uint8_t times)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < times; i++) {
        sum += ReadOnce(ch);
    }
    ch->avg_raw = (uint16_t)(sum / times);
    return ch->avg_raw;
}
