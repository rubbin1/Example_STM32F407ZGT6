//
// Created by 35156 on 2026/6/19.
// XPT2046 电阻触摸驱动
//

#ifndef TOUCH_H
#define TOUCH_H
#include <stdint.h>
#include "main.h"

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t  pressed;   // 0=未触摸, 1=触摸中
} TouchData;
extern TouchData touch;

void    Touch_Init(void);
uint8_t Touch_Scan(void);                    // 返回 1=有新数据
void    Touch_GetXY(uint16_t *x, uint16_t *y);
uint8_t Touch_IsPressed(void);

#endif //TOUCH_H
