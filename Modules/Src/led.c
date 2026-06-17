//
// Created by 35156 on 2026/5/24.
//

#include "led.h"
#include "main.h"

LED led0 = {
    .pin = LED0_Pin,
    .port = LED0_GPIO_Port,
    .active_level = GPIO_PIN_RESET,
};
LED led1 = {
    .pin = LED1_Pin,
    .port = LED1_GPIO_Port,
    .active_level = GPIO_PIN_RESET,
};

LED *led_map[] = {
    &led0,
    &led1,
};
uint8_t const led_map_size = sizeof(led_map) / sizeof(led_map[0]);

//所有LED的初始化
void Leds_Init(void)
{
    for (uint8_t i = 0; i < led_map_size; i++){
        Led_Off(led_map[i]);
    }
}

//下面写LED的功能函数
void Led_On(LED *led)
{
    HAL_GPIO_WritePin(led->port, led->pin, led->active_level);
}

void Led_Off(LED *led)
{
    HAL_GPIO_WritePin(led->port, led->pin, !led->active_level);
}

void Led_OnAll(void)
{
    for (uint8_t i = 0; i < led_map_size; i++){
        Led_On(led_map[i]);
    }
}

void Led_OffAll(void)
{
    for (uint8_t i = 0; i < led_map_size; i++){
        Led_Off(led_map[i]);
    }
}

void Led_Toggle(LED *led)
{
    HAL_GPIO_TogglePin(led->port, led->pin);
}

void Led_ToggleAll(void)
{
    for (uint8_t i = 0; i < led_map_size; i++){
        Led_Toggle(led_map[i]);
    }
}

LED_FLOW led_flow = {0};

void Led_Flow_Init(int time_interval)
{
    led_flow.led = &led0;
    SoftTimer_Start(&led_flow.timer, time_interval);
    led_flow.flag = LED_NO_FLOWING;
}

void Led_Flow(void)
{
    static uint8_t current_index = 0;

    if (led_flow.flag != LED_FLOWING) {
        return;
    }

    if (SoftTimer_Expired(&led_flow.timer)) {
        Led_Off(led_map[current_index]);
        current_index = (current_index + 1) % led_map_size;
        Led_On(led_map[current_index]);
    }
}
