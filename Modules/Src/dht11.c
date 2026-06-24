//
// Created by 35156 on 2026/6/24.
// DHT11 温湿度传感器 (自定义单总线协议)
//

#include "dht11.h"
#include "main.h"

DHT11 dht11 = {
    .port = WIRE_DQ1_GPIO_Port,
    .pin  = WIRE_DQ1_Pin,
};

static void delay_us(uint16_t us)
{
    uint32_t ticks = us * 42;
    while (ticks--) { __NOP(); }
}

static void pin_out(DHT11 *dht)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = dht->pin;
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(dht->port, &g);
}

static void pin_in(DHT11 *dht)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = dht->pin;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(dht->port, &g);
}

static uint8_t read_byte(DHT11 *dht)
{
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++) {
        /* 等待低电平结束 */
        uint16_t timeout = 0;
        while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_RESET && ++timeout < 200);
        /* 等待高电平开始，延时 30μs 后采样 */
        delay_us(30);
        byte <<= 1;
        if (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET) byte |= 1;
        /* 等待高电平结束 */
        timeout = 0;
        while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET && ++timeout < 200);
    }
    return byte;
}

void DHT11_Init(DHT11 *dht)
{
    pin_out(dht);
    HAL_GPIO_WritePin(dht->port, dht->pin, GPIO_PIN_SET);
    dht->humidity = 0;
    dht->temperature = 0;
}

uint8_t DHT11_Read(DHT11 *dht)
{
    uint8_t buf[5];

    /* 主机起始信号: 拉低 18ms，拉高 30μs */
    __disable_irq();
    pin_out(dht);
    HAL_GPIO_WritePin(dht->port, dht->pin, GPIO_PIN_RESET);
    delay_us(18000);
    HAL_GPIO_WritePin(dht->port, dht->pin, GPIO_PIN_SET);
    delay_us(30);
    pin_in(dht);
    /* 等待 DHT11 应答 */
    uint16_t wait = 0;
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET && ++wait < 200);
    if (wait >= 200) { __enable_irq(); return 0; }
    wait = 0;
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_RESET && ++wait < 200);
    if (wait >= 200) { __enable_irq(); return 0; }
    wait = 0;
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET && ++wait < 200);
    if (wait >= 200) { __enable_irq(); return 0; }

    /* 读 5 字节 */
    for (uint8_t i = 0; i < 5; i++) {
        buf[i] = read_byte(dht);
    }
    __enable_irq();

    /* 校验 */
    if (buf[4] != ((buf[0] + buf[1] + buf[2] + buf[3]) & 0xFF))
        return 0;

    dht->humidity    = buf[0];
    dht->temperature = buf[2];
    return 1;
}
