//
// Created by 35156 on 2026/6/17.
//

#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

#define SERIAL_RX_BUF_SIZE 512

typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t  rx_buf[SERIAL_RX_BUF_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
} Serial_t;

extern Serial_t serial1;
extern Serial_t serial3;

void Serial_Init(Serial_t *s);
void Serial_SendByte(Serial_t *s, uint8_t byte);
void Serial_SendData(Serial_t *s, const uint8_t *data, uint16_t len);
void Serial_SendString(Serial_t *s, const char *str);
void Serial_PrintU32(Serial_t *s, uint32_t val);
void Serial_Println(Serial_t *s, const char *str);

uint16_t Serial_Available(Serial_t *s);
uint8_t  Serial_ReadByte(Serial_t *s);

#endif //SERIAL_H
