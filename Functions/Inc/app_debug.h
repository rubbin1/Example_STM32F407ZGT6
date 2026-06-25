#ifndef APP_DEBUG_H
#define APP_DEBUG_H
#include <stdint.h>

void App_Debug_Init(void);
void App_Debug_Loop(uint16_t x, uint16_t y, uint8_t pressed);

// 全局 printf，任何地方调用，自动显示在 Debug 页
void Debug_Printf(const char *fmt, ...);
void Debug_Clear(void);

#endif
