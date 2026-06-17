#ifndef KEY_PRESS_H
#define KEY_PRESS_H

#include "main.h"
#include "soft_timer.h"

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_SHORT,   // 短按：按下后在长按阈值前释放
    KEY_EVENT_LONG,    // 长按：按下超过长按阈值
    KEY_EVENT_REPEAT,  // 连发：长按期间周期性触发
} KeyEvent;

typedef struct
{
    uint16_t   pin;
    GPIO_TypeDef *port;
    uint8_t    current_state;       // 当前稳定电平：0=松开，1=按下
    uint8_t    last_state;          // 上一次读取的原始电平
    SOFT_TIMER debounce;            // 消抖定时器 20ms
    KeyEvent   event;               // 最近一次按键事件
    uint8_t    long_pressed;        // 是否已触发过长按
    SOFT_TIMER long_press;          // 长按判定 100ms
    SOFT_TIMER repeat;              // 长按连发 200ms
} Key_TypeDef;

extern Key_TypeDef key0;
extern Key_TypeDef key1;
extern Key_TypeDef key2;
extern Key_TypeDef key_up;

extern Key_TypeDef *key_map[];
extern const uint8_t KEY_COUNT;

void Key_Scan(Key_TypeDef *key);

#endif //KEY_PRESS_H
