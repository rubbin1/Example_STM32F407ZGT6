//
// Created by 35156 on 26-4-9.
//

#include "key_press.h"

#define DEBOUNCE_MS    20
#define LONG_PRESS_MS  500
#define REPEAT_MS      200

Key_TypeDef key0 = {
    .pin = KEY0_Pin,
    .port = KEY0_GPIO_Port,
};
Key_TypeDef key1 = {
    .pin = KEY1_Pin,
    .port = KEY1_GPIO_Port,
};
Key_TypeDef key2 = {
    .pin = KEY2_Pin,
    .port = KEY2_GPIO_Port,
};
Key_TypeDef key_up = {
    .pin = KEY_UP_Pin,
    .port = KEY_UP_GPIO_Port,
};

Key_TypeDef *key_map[] = {&key0, &key1, &key2, &key_up};
const uint8_t KEY_COUNT = sizeof(key_map) / sizeof(key_map[0]);

void Key_Scan(Key_TypeDef *key)
{
    int level = HAL_GPIO_ReadPin(key->port, key->pin);

    // 电平变化（或首次调用）→ 重置消抖计时
    if (level != key->last_state || key->debounce.interval == 0) {
        key->last_state = level;
        SoftTimer_Start(&key->debounce, DEBOUNCE_MS);
        return;
    }

    // 消抖未完成
    if (!SoftTimer_Elapsed(&key->debounce)) {
        return;
    }

    key->event = KEY_EVENT_NONE;

    // 稳定低电平 → 按键处于按下状态
    if (level == GPIO_PIN_RESET) {
        // 刚按下（上升沿之后的首个稳定低电平）
        if (key->current_state == 0) {
            key->current_state = 1;
            key->event = KEY_EVENT_SHORT;
            key->long_pressed = 0;
            SoftTimer_Start(&key->long_press, LONG_PRESS_MS);
            SoftTimer_Start(&key->repeat, REPEAT_MS);
        }
        // 持续按下中
        else {
            if (!key->long_pressed && SoftTimer_Expired(&key->long_press)) {
                key->long_pressed = 1;
                key->event = KEY_EVENT_LONG;
                SoftTimer_Reset(&key->repeat);
            }
            else if (key->long_pressed && SoftTimer_Expired(&key->repeat)) {
                key->event = KEY_EVENT_REPEAT;
            }
        }
    }
    // 稳定高电平 → 按键松开
    else {
        key->current_state = 0;
    }
}
