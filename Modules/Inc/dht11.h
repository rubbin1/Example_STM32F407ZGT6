//
// Created by 35156 on 2026/6/24.
// DHT11 温湿度传感器 (自定义单总线)
//

#ifndef DHT11_H
#define DHT11_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t     pin;
    uint8_t      humidity;      // %RH
    uint8_t      temperature;   // °C
} DHT11;

extern DHT11 dht11;

void    DHT11_Init(DHT11 *dht);
uint8_t DHT11_Read(DHT11 *dht);   // 返回 1=成功

#endif //DHT11_H
