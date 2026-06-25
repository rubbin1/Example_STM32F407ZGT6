#ifndef APP_PERIPHERAL_H
#define APP_PERIPHERAL_H

#include <stdint.h>

void App_Peripheral_Hardware_Init(void);
void App_Peripheral_Hardware_Loop(uint16_t x, uint16_t y, uint8_t pressed);

typedef enum
{
    SUB_MAIN,
    SUB_LED_CTRL,
    SUB_BUZZER_CTRL,
}SubPage;

#endif
