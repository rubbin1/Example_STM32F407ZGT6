//
// Created by 35156 on 2026/6/17.
//

#include "serial.h"
#include "main.h"
#include "usart.h"

Serial_t serial1 = {
    .huart = &huart1,
};

Serial_t serial3 = {
    .huart = &huart3,
};

void Serial_Init(Serial_t *s)
{
    s->rx_head = 0;
    s->rx_tail = 0;
    HAL_UART_Receive_IT(s->huart, &s->rx_buf[s->rx_head], 1);
}

void Serial_SendByte(Serial_t *s, uint8_t byte)
{
    HAL_UART_Transmit(s->huart, &byte, 1, HAL_MAX_DELAY);
}

void Serial_SendData(Serial_t *s, const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(s->huart, (uint8_t *)data, len, HAL_MAX_DELAY);
}

void Serial_SendString(Serial_t *s, const char *str)
{
    while (*str) {
        Serial_SendByte(s, *str++);
    }
}

void Serial_PrintU32(Serial_t *s, uint32_t val)
{
    char buf[12];
    uint8_t pos = 11;
    buf[pos] = '\0';
    if (val == 0) {
        buf[--pos] = '0';
    } else {
        while (val > 0) {
            buf[--pos] = '0' + (val % 10);
            val /= 10;
        }
    }
    Serial_SendString(s, &buf[pos]);
}

void Serial_Println(Serial_t *s, const char *str)
{
    Serial_SendString(s, str);
    Serial_SendString(s, "\r\n");
}

uint16_t Serial_Available(Serial_t *s)
{
    uint16_t tail = s->rx_tail;
    if (s->rx_head >= tail) {
        return s->rx_head - tail;
    }
    return SERIAL_RX_BUF_SIZE - tail + s->rx_head;
}

uint8_t Serial_ReadByte(Serial_t *s)
{
    uint8_t byte = s->rx_buf[s->rx_tail];
    s->rx_tail = (s->rx_tail + 1) % SERIAL_RX_BUF_SIZE;
    return byte;
}

static Serial_t *serial_by_huart(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) return &serial1;
    if (huart == &huart3) return &serial3;
    return NULL;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    Serial_t *s = serial_by_huart(huart);
    if (!s) return;

    uint16_t next = (s->rx_head + 1) % SERIAL_RX_BUF_SIZE;
    if (next != s->rx_tail) {
        s->rx_head = next;
    }
    HAL_UART_Receive_IT(huart, &s->rx_buf[s->rx_head], 1);
}
