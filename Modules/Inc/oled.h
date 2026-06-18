//
// Created by 35156 on 2026/6/19.
// ATK-MD0096 0.96" OLED (SSD1306) 8080 并口驱动
//

#ifndef OLED_H
#define OLED_H
#include <stdint.h>
#include "main.h"

#define OLED_WIDTH   128
#define OLED_HEIGHT  64

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t dot);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);

/* ── 图形绘制 ── */
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void OLED_DrawFillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r);
void OLED_DrawFillCircle(uint8_t x, uint8_t y, uint8_t r);

#endif //OLED_H
