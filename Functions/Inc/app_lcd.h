//
// LCD 触摸演示应用
//
#ifndef APP_LCD_H
#define APP_LCD_H
#include <stdint.h>

void App_LCD_Init(void);     // 初始化界面，画所有控件
void App_LCD_Loop(void);     // 主循环里调用：扫描触摸+分发事件

#endif
