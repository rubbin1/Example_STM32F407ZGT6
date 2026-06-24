//
// Created by 35156 on 2026/6/24.
// I2C 总线层 — 不写死设备地址
//

#ifndef I2C_BUS_H
#define I2C_BUS_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct {
    I2C_HandleTypeDef *hi2c;
} I2C_BUS;

extern I2C_BUS i2c1;

void I2C_Bus_Init(I2C_BUS *bus);

/* 扫描设备：返回 0=无响应，1=在线 */
uint8_t I2C_Bus_Probe(I2C_BUS *bus, uint8_t dev_addr);

/* 寄存器读/写 (1字节寄存器地址) */
void I2C_Bus_MemRead(I2C_BUS *bus, uint8_t dev, uint8_t reg,
                     uint8_t *buf, uint16_t len);
void I2C_Bus_MemWrite(I2C_BUS *bus, uint8_t dev, uint8_t reg,
                      const uint8_t *buf, uint16_t len);

/* 无寄存器地址的读/写 */
void I2C_Bus_Read(I2C_BUS *bus, uint8_t dev, uint8_t *buf, uint16_t len);
void I2C_Bus_Write(I2C_BUS *bus, uint8_t dev, const uint8_t *buf, uint16_t len);

#endif //I2C_BUS_H
