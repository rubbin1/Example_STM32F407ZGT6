//
// Created by 35156 on 2026/6/24.
// Dallas OneWire 总线层
//

#include "onewire.h"
#include "main.h"

/* ── 微秒延时 (168MHz 下 ~5 周期/循环) ── */
static void delay_us(uint16_t us)
{
    uint32_t ticks = us * 42;  // 168MHz / 4 ≈ 42 循环/μs
    while (ticks--) { __NOP(); }
}

/* ── 引脚模式切换 ── */
static void ow_out(ONEWIRE *ow)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = ow->pin;
    g.Mode  = GPIO_MODE_OUTPUT_OD;   // 开漏: 写0=拉低, 写1=释放(上拉)
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ow->port, &g);
}

static void ow_in(ONEWIRE *ow)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = ow->pin;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(ow->port, &g);
}

ONEWIRE ow1 = {
    .port = WIRE_DQ1_GPIO_Port,
    .pin  = WIRE_DQ1_Pin,
};

void OneWire_Init(ONEWIRE *ow)
{
    ow_out(ow);
    HAL_GPIO_WritePin(ow->port, ow->pin, GPIO_PIN_SET);  // 释放总线
}

/* ── 复位 + 检测存在脉冲 ── */
uint8_t OneWire_Reset(ONEWIRE *ow)
{
    uint8_t presence;

    __disable_irq();
    ow_out(ow);
    HAL_GPIO_WritePin(ow->port, ow->pin, GPIO_PIN_RESET);
    delay_us(480);
    HAL_GPIO_WritePin(ow->port, ow->pin, GPIO_PIN_SET);  // 释放
    delay_us(70);
    ow_in(ow);
    presence = (HAL_GPIO_ReadPin(ow->port, ow->pin) == GPIO_PIN_RESET);
    delay_us(410);
    __enable_irq();
    return presence;
}

/* ── 写一个 bit ── */
void OneWire_WriteBit(ONEWIRE *ow, uint8_t bit)
{
    __disable_irq();
    ow_out(ow);
    HAL_GPIO_WritePin(ow->port, ow->pin, GPIO_PIN_RESET);
    delay_us(2);  // 拉低 > 1μs

    if (bit) {
        HAL_GPIO_WritePin(ow->port, ow->pin, GPIO_PIN_SET);  // 释放
        delay_us(60);
    } else {
        delay_us(60);
        HAL_GPIO_WritePin(ow->port, ow->pin, GPIO_PIN_SET);  // 释放
        delay_us(2);
    }
    __enable_irq();
}

/* ── 读一个 bit ── */
uint8_t OneWire_ReadBit(ONEWIRE *ow)
{
    uint8_t bit;
    __disable_irq();
    ow_out(ow);
    HAL_GPIO_WritePin(ow->port, ow->pin, GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(ow->port, ow->pin, GPIO_PIN_SET);  // 释放
    delay_us(1);
    ow_in(ow);
    bit = (HAL_GPIO_ReadPin(ow->port, ow->pin) == GPIO_PIN_SET);
    delay_us(57);
    __enable_irq();
    return bit;
}

/* ── 写/读 字节 (LSB first) ── */
void OneWire_WriteByte(ONEWIRE *ow, uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        OneWire_WriteBit(ow, byte & 0x01);
        byte >>= 1;
    }
}

uint8_t OneWire_ReadByte(ONEWIRE *ow)
{
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++) {
        byte >>= 1;
        if (OneWire_ReadBit(ow)) byte |= 0x80;
    }
    return byte;
}
