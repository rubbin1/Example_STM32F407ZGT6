//
// Created by 35156 on 2026/6/24.
// Dallas OneWire 总线层 — 不写死引脚
//

#ifndef ONEWIRE_H
#define ONEWIRE_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t     pin;
} ONEWIRE;

extern ONEWIRE ow1;

void    OneWire_Init(ONEWIRE *ow);
uint8_t OneWire_Reset(ONEWIRE *ow);
void    OneWire_WriteBit(ONEWIRE *ow, uint8_t bit);
uint8_t OneWire_ReadBit(ONEWIRE *ow);
void    OneWire_WriteByte(ONEWIRE *ow, uint8_t byte);
uint8_t OneWire_ReadByte(ONEWIRE *ow);

#endif //ONEWIRE_H
