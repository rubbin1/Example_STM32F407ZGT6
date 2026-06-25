//
// 外设控制页 — LED / 蜂鸣器
//
#include "app_peripheral.h"
#include "gui_button.h"
#include "tft_lcd.h"
#include "led.h"
#include "buzzer.h"
#include "font_flash.h"
#include <stdio.h>
#include "page_manager.h"

/* ── LED0 按钮回调 ── */
static void Btn_Led0_Click(void)
{
    Led_Toggle(&led0);
}

/* ── LED1 按钮回调 ── */
static void Btn_Led1_Click(void)
{
    Led_Toggle(&led1);
}

/* ── 流水灯 按键回调 ── */
static void Btn_Flow_Click(void)
{
    led_flow.flag = (led_flow.flag == LED_FLOWING) ? LED_NO_FLOWING : LED_FLOWING;
    SoftTimer_Start(&led_flow.timer, 500);

    if (led_flow.flag == LED_NO_FLOWING){
        Led_OffAll();
    }
}

/* ── 蜂鸣器按钮 ── */
static void Btn_Beep_Click(void)
{
    Buzzer_Beep(&buzzer, 500);
}

/* ── 子页面控件定义 ── */
//返回键
static Button btn_back = {
    .x = 10, .y = 10, .w = 80, .h = 20,
    .text = "Back", .color = TFT_WHITE, .bg_color = TFT_BLACK,
    .on_click = NULL,
};

//LED子页面
static Button btn_led_ctrl = {
    .x = 10, .y = 40, .w = 300, .h = 40,
    .text = "LED", .color = TFT_WHITE, .bg_color = TFT_BLACK,
    .on_click = NULL,
};

//蜂鸣器子页面
static Button btn_buzzer_ctrl = {
    .x = 10, .y = 100, .w = 300, .h = 40,
    .text = "Buzzer", .color = TFT_WHITE, .bg_color = TFT_BLACK,
    .on_click = NULL,
};

/* ── 控件定义 ── */
static Button btn_led0 = {
    .x = 10, .y = 100, .w = 140, .h = 40,
    .text = " LED0 ", .color = TFT_WHITE, .bg_color = TFT_RED,
    .on_click = Btn_Led0_Click,
};

static Button btn_led1 = {
    .x = 10, .y = 190, .w = 140, .h = 40,
    .text = " LED1 ", .color = TFT_WHITE, .bg_color = TFT_GREEN,
    .on_click = Btn_Led1_Click,
};

static Button btn_flow = {
    .x = 10, .y = 280, .w = 140, .h = 40,
    .text = "Flow LED", .color = TFT_WHITE, .bg_color = TFT_BLUE,
    .on_click = Btn_Flow_Click,
};

static Button btn_beep = {
    .x = 10, .y = 100, .w = 140, .h = 40,
    .text = " Buzzer ", .color = TFT_WHITE, .bg_color = TFT_BLACK,
    .on_click = Btn_Beep_Click,
};

static SubPage sub_page = SUB_MAIN;

/* ════════════════════════════ 清除内容区 ════════════════════════════*/

static void ClearContent(void)
{
    TFTLCD_Fill(0, 0, tftlcd.width - 1, tftlcd.height - NAV_H - 1, TFT_BLACK);
}

/* ════════════════════════════ 对外接口 ════════════════════════════ */

void App_Peripheral_Hardware_Init(void)
{
    ClearContent();
    tft_bg_color = TFT_BLACK;

    switch (sub_page) {
    case SUB_MAIN:
        FontFlash_ShowString_UTF8(10, 10, "硬件外设控制", 24, TFT_WHITE);
        Button_Draw(&btn_led_ctrl);
        Button_Draw(&btn_buzzer_ctrl);
        break;
    case SUB_LED_CTRL:
        FontFlash_ShowString_UTF8(150, 5, "LED控制", 24, TFT_WHITE);
        Button_Draw(&btn_back);
        Button_Draw(&btn_led0);
        Button_Draw(&btn_led1);
        Button_Draw(&btn_flow);
        break;
    case SUB_BUZZER_CTRL:
        FontFlash_ShowString_UTF8(150, 5, "蜂鸣器控制", 24, TFT_WHITE);
        Button_Draw(&btn_back);
        Button_Draw(&btn_beep);
        break;
    }
}

void App_Peripheral_Hardware_Loop(uint16_t x, uint16_t y, uint8_t pressed)
{
    switch (sub_page){
    case SUB_MAIN:
        if (Button_Check(&btn_led_ctrl, x, y)){
            sub_page = SUB_LED_CTRL;
            App_Peripheral_Hardware_Init();
        }
        if (Button_Check(&btn_buzzer_ctrl, x, y)){
            sub_page = SUB_BUZZER_CTRL;
            App_Peripheral_Hardware_Init();
        }
        break;
    case SUB_LED_CTRL:
        if (Button_Check(&btn_back, x, y)){
            sub_page = SUB_MAIN;
            App_Peripheral_Hardware_Init();
        }
        if (pressed){
            Button_Check(&btn_led0, x, y);
            Button_Check(&btn_led1, x, y);
            Button_Check(&btn_flow, x, y);
        }
        break;
    case SUB_BUZZER_CTRL:
        if (Button_Check(&btn_back, x, y)){
            sub_page = SUB_MAIN;
            App_Peripheral_Hardware_Init();
        }
        if (pressed){
            Button_Check(&btn_beep, x, y);
        }
        break;
    }
    Led_Flow();
    Buzzer_Loop(&buzzer);
}
