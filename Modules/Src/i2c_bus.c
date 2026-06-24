//
// Created by 35156 on 2026/6/24.
// I2C 总线层
//

#include "i2c_bus.h"
#include "i2c.h"

#define I2C_TIMEOUT 50

I2C_BUS i2c1 = {
    .hi2c = &hi2c1,
};

void I2C_Bus_Init(I2C_BUS *bus)
{
    /* HAL I2C 已在 MX_I2C1_Init 中初始化 */
    (void)bus;
}

uint8_t I2C_Bus_Probe(I2C_BUS *bus, uint8_t dev_addr)
{
    return (HAL_I2C_IsDeviceReady(bus->hi2c, dev_addr << 1, 2, I2C_TIMEOUT) == HAL_OK);
}

void I2C_Bus_MemRead(I2C_BUS *bus, uint8_t dev, uint8_t reg,
                     uint8_t *buf, uint16_t len)
{
    HAL_I2C_Mem_Read(bus->hi2c, dev << 1, reg, I2C_MEMADD_SIZE_8BIT,
                     buf, len, I2C_TIMEOUT);
}

void I2C_Bus_MemWrite(I2C_BUS *bus, uint8_t dev, uint8_t reg,
                      const uint8_t *buf, uint16_t len)
{
    HAL_I2C_Mem_Write(bus->hi2c, dev << 1, reg, I2C_MEMADD_SIZE_8BIT,
                      (uint8_t *)buf, len, I2C_TIMEOUT);
}

void I2C_Bus_Read(I2C_BUS *bus, uint8_t dev, uint8_t *buf, uint16_t len)
{
    HAL_I2C_Master_Receive(bus->hi2c, dev << 1, buf, len, I2C_TIMEOUT);
}

void I2C_Bus_Write(I2C_BUS *bus, uint8_t dev, const uint8_t *buf, uint16_t len)
{
    HAL_I2C_Master_Transmit(bus->hi2c, dev << 1, (uint8_t *)buf, len, I2C_TIMEOUT);
}
