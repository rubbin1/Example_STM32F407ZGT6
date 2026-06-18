//
// Created by 35156 on 2026/5/24.
//

#ifndef EXAMPLE_LED_H
#define EXAMPLE_LED_H
#include <stdint.h>
#include "stm32f407xx.h"
#include "soft_timer.h"

//LED结构体
typedef struct
{
    uint16_t pin;
    GPIO_TypeDef *port;
    GPIO_PinState active_level;
}LED;
extern LED led0, led1;
//所有LED的初始化
void Leds_Init(void);
//一些简单的、常用的的led功能函数
void Led_On(LED *led);
void Led_Off(LED *led);
void Led_OnAll(void);
void Led_OffAll(void);
void Led_Toggle(LED *led);
void Led_ToggleAll(void);

//LEDmap和数组长度
extern LED *led_map[];
extern const uint8_t led_map_size;

//流水灯标志位
typedef enum
{
    LED_FLOWING = 1,
    LED_NO_FLOWING = 0,
}LED_FLOW_FLAG;
//流水灯结构体
typedef struct
{
    LED *led;
    SOFT_TIMER timer;
    LED_FLOW_FLAG flag;
}LED_FLOW;
extern LED_FLOW led_flow;
//流水灯初始化函数
void Led_Flow_Init(int time_interval);
void Led_Flow_On(void);
void Led_Flow_Off(void);
void Led_Flow(void);

#endif //EXAMPLE_LED_H
