//
// Created by 35156 on 2026/6/24.
// DS18B20 温度传感器
//

#include "ds18b20.h"

#define CMD_SKIP_ROM    0xCC
#define CMD_CONVERT_T   0x44
#define CMD_READ_SP     0xBE

DS18B20 ds18b20 = {
    .bus = &ow1,
};

void DS18B20_Init(DS18B20 *s)
{
    OneWire_Init(s->bus);
    s->temp_c = 0.0f;
}

uint8_t DS18B20_StartConvert(DS18B20 *s)
{
    if (!OneWire_Reset(s->bus)) return 0;
    OneWire_WriteByte(s->bus, CMD_SKIP_ROM);
    OneWire_WriteByte(s->bus, CMD_CONVERT_T);
    return 1;
}

float DS18B20_ReadTemp(DS18B20 *s)
{
    uint8_t sp[9];

    if (!OneWire_Reset(s->bus)) return s->temp_c;

    OneWire_WriteByte(s->bus, CMD_SKIP_ROM);
    OneWire_WriteByte(s->bus, CMD_READ_SP);

    for (uint8_t i = 0; i < 9; i++) {
        sp[i] = OneWire_ReadByte(s->bus);
    }

    int16_t raw = ((int16_t)sp[1] << 8) | sp[0];
    s->temp_c = raw / 16.0f;
    return s->temp_c;
}
