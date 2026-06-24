//
// Created by 35156 on 2026/6/24.
// DS18B20 温度传感器
//

#ifndef DS18B20_H
#define DS18B20_H
#include <stdint.h>
#include "onewire.h"

typedef struct {
    ONEWIRE *bus;
    float    temp_c;
} DS18B20;

extern DS18B20 ds18b20;

void  DS18B20_Init(DS18B20 *s);
uint8_t DS18B20_StartConvert(DS18B20 *s);
float   DS18B20_ReadTemp(DS18B20 *s);

#endif //DS18B20_H
