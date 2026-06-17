//
// Created by 35156 on 2026/6/2.
//

#include "light_sensor.h"
#include "main.h"

LIGHT_SENSOR light_sensor = {
    .pin = LIGHT_SENSOR_Pin,
    .port = LIGHT_SENSOR_GPIO_Port,
    .dark_level = GPIO_PIN_SET,     // 高电平 = 暗（无光时阻值大，引脚被拉高）
    .light_flag = LIGHT,
};

void LightSensor_Init(LIGHT_SENSOR *lts)
{
    LightSensor_Scan(lts);
}

void LightSensor_Scan(LIGHT_SENSOR *lts)
{
    int level = HAL_GPIO_ReadPin(lts->port, lts->pin);
    lts->light_flag = (level == lts->dark_level) ? DARK : LIGHT;
}

uint8_t LightSensor_IsDark(LIGHT_SENSOR *lts)
{
    return lts->light_flag == DARK;
}

uint8_t LightSensor_IsLight(LIGHT_SENSOR *lts)
{
    return lts->light_flag == LIGHT;
}
