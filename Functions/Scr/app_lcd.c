//
// LCD 触摸演示应用
//
#include "tft_lcd.h"
#include "tft_lcd_touch.h"
#include "gui_button.h"
#include "led.h"
#include <stdio.h>

/* ── LED0 按钮回调 ── */
static void Btn_Led0_Click(void) {
    Led_Toggle(&led0);
}

/* ── 流水灯复选框回调 ── */
static void Cb_Flow_Change(uint8_t checked) {
    led_flow.flag = checked ? LED_FLOWING : LED_NO_FLOWING;
    if (!checked) Led_OffAll();
}

/* ── 控件定义 ── */
static Button btn_led0 = {
    .x = 20, .y = 60, .w = 120, .h = 40,
    .text = "LED0", .color = TFT_WHITE, .bg_color = TFT_BLUE,
    .on_click = Btn_Led0_Click,
};

static CheckBox cb_flow = {
    .x = 20, .y = 120, .text = "Flow LED",
    .color = TFT_WHITE, .bg_color = TFT_BLACK,
    .on_change = Cb_Flow_Change,
};

/* ════════════════════════════ 对外接口 ════════════════════════════ */

void App_LCD_Init(void)
{
    TFTLCD_SetBackColor(TFT_BLACK);
    TFTLCD_ShowString(20, 20, "LCD Touch Control LEDs", 16, TFT_YELLOW);
    Button_Draw(&btn_led0);
    CheckBox_Draw(&cb_flow);
}

void App_LCD_Loop(void)
{
    if (!Touch_Scan()) return;

    uint16_t x, y;
    Touch_GetXY(&x, &y);

    // 分发触摸事件
    Button_Check(&btn_led0, x, y);
    CheckBox_Check(&cb_flow, x, y);
}
