//
// Created by 35156 on 2026/6/19.
// ATK-MD0096 0.96" OLED (SSD1306) 8080 并口驱动
//

#include "oled.h"
#include "oled_font.h"

/* ── 8080 写一个字节 ── */
static void oled_write_byte(uint8_t dat, uint8_t cmd)
{
    // 拼数据到四个端口
    uint16_t d = dat & 0x0F;
    GPIOC->ODR &= ~(0x0F << 6);
    GPIOC->ODR |= d << 6;                         // D[3:0] → PC[9:6]

    GPIOC->ODR &= ~(1 << 11);
    GPIOC->ODR |= ((dat >> 4) & 1) << 11;         // D4 → PC11

    GPIOB->ODR &= ~(1 << 6);
    GPIOB->ODR |= ((dat >> 5) & 1) << 6;          // D5 → PB6

    GPIOE->ODR &= ~(0x03 << 5);
    GPIOE->ODR |= ((dat >> 6) & 1) << 5;          // D6 → PE5
    GPIOE->ODR |= ((dat >> 7) & 1) << 6;          // D7 → PE6

    // DC: 0=命令, 1=数据
    HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin,
        cmd ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // CS=0 → WR=0 → WR=1 锁存
    HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OLED_RW_GPIO_Port, OLED_RW_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OLED_RW_GPIO_Port, OLED_RW_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET);
}

static void oled_write_cmd(uint8_t cmd)  { oled_write_byte(cmd, 0); }
static void oled_write_data(uint8_t dat) { oled_write_byte(dat, 1); }

/* ════════════════════════════ 帧缓存 ════════════════════════════ */

static uint8_t g_gram[OLED_HEIGHT / 8][OLED_WIDTH];

void OLED_Update(void)
{
    for (uint8_t p = 0; p < OLED_HEIGHT / 8; p++) {
        oled_write_cmd(0xB0 + p);
        oled_write_cmd(0x00);
        oled_write_cmd(0x10);
        for (uint8_t c = 0; c < OLED_WIDTH; c++)
            oled_write_data(g_gram[p][c]);
    }
}

void OLED_Clear(void)
{
    for (uint8_t p = 0; p < OLED_HEIGHT / 8; p++)
        for (uint8_t c = 0; c < OLED_WIDTH; c++)
            g_gram[p][c] = 0;
}

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t dot)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    if (dot)
        g_gram[y / 8][x] |=  (1 << (y % 8));
    else
        g_gram[y / 8][x] &= ~(1 << (y % 8));
}

/* ════════════════════════════ 初始化 ════════════════════════════ */

void OLED_Init(void)
{
    // 硬件复位
    HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    // 初始电平
    HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_RW_GPIO_Port, OLED_RW_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_RD_GPIO_Port, OLED_RD_Pin, GPIO_PIN_SET);

    // SSD1306 寄存器序列
    oled_write_cmd(0xAE);
    oled_write_cmd(0xD5); oled_write_cmd(0x80);
    oled_write_cmd(0xA8); oled_write_cmd(0x3F);
    oled_write_cmd(0xD3); oled_write_cmd(0x00);
    oled_write_cmd(0x40);
    oled_write_cmd(0x8D); oled_write_cmd(0x14);
    oled_write_cmd(0x20); oled_write_cmd(0x02);
    oled_write_cmd(0xA1);
    oled_write_cmd(0xC8);
    oled_write_cmd(0xDA); oled_write_cmd(0x12);
    oled_write_cmd(0x81); oled_write_cmd(0xCF);
    oled_write_cmd(0xD9); oled_write_cmd(0xF1);
    oled_write_cmd(0xDB); oled_write_cmd(0x30);
    oled_write_cmd(0xA4);
    oled_write_cmd(0xA6);
    oled_write_cmd(0xAF);

    OLED_Clear();
    OLED_Update();
}

/* ════════════════════════════ 图形绘制 ════════════════════════════ */

// Bresenham 直线
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    int16_t dx = (x2 > x1) ? x2 - x1 : x1 - x2;
    int16_t dy = (y2 > y1) ? y2 - y1 : y1 - y2;
    int8_t  sx = (x1 < x2) ? 1 : -1;
    int8_t  sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t e2;

    while (1) {
        OLED_DrawPoint(x1, y1, 1);
        if (x1 == x2 && y1 == y2) break;
        e2 = err << 1;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    OLED_DrawLine(x,     y,     x + w, y);
    OLED_DrawLine(x + w, y,     x + w, y + h);
    OLED_DrawLine(x + w, y + h, x,     y + h);
    OLED_DrawLine(x,     y + h, x,     y);
}

void OLED_DrawFillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t r = y; r <= y + h; r++)
        OLED_DrawLine(x, r, x + w, r);
}

// Bresenham 圆
void OLED_DrawCircle(uint8_t cx, uint8_t cy, uint8_t r)
{
    int16_t a = 0, b = r, d = 3 - (r << 1);
    while (a <= b) {
        OLED_DrawPoint(cx + a, cy + b, 1);
        OLED_DrawPoint(cx - a, cy + b, 1);
        OLED_DrawPoint(cx + a, cy - b, 1);
        OLED_DrawPoint(cx - a, cy - b, 1);
        OLED_DrawPoint(cx + b, cy + a, 1);
        OLED_DrawPoint(cx - b, cy + a, 1);
        OLED_DrawPoint(cx + b, cy - a, 1);
        OLED_DrawPoint(cx - b, cy - a, 1);
        a++;
        if (d < 0) { d += 4 * a + 6; }
        else       { d += 10 + 4 * (a - b); b--; }
    }
}

void OLED_DrawFillCircle(uint8_t cx, uint8_t cy, uint8_t r)
{
    int16_t a = 0, b = r, d = 3 - (r << 1);
    while (a <= b) {
        for (int16_t i = cx - b; i <= cx + b; i++) {
            OLED_DrawPoint(i, cy + a, 1);
            OLED_DrawPoint(i, cy - a, 1);
        }
        for (int16_t i = cx - a; i <= cx + a; i++) {
            OLED_DrawPoint(i, cy + b, 1);
            OLED_DrawPoint(i, cy - b, 1);
        }
        a++;
        if (d < 0) { d += 4 * a + 6; }
        else       { d += 10 + 4 * (a - b); b--; }
    }
}

/* ════════════════════════════ 6×8 字库 ════════════════════════════ */

static const uint8_t font_6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, /* sp */
    {0x00,0x00,0x5F,0x00,0x00,0x00}, /* !   */
    {0x00,0x07,0x00,0x07,0x00,0x00}, /* "   */
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, /* #   */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, /* $   */
    {0x23,0x13,0x08,0x64,0x62,0x00}, /* %   */
    {0x36,0x49,0x56,0x20,0x50,0x00}, /* &   */
    {0x00,0x08,0x07,0x00,0x00,0x00}, /* '   */
    {0x00,0x1C,0x22,0x41,0x00,0x00}, /* (   */
    {0x00,0x41,0x22,0x1C,0x00,0x00}, /* )   */
    {0x2A,0x1C,0x7F,0x1C,0x2A,0x00}, /* *   */
    {0x08,0x08,0x3E,0x08,0x08,0x00}, /* +   */
    {0x00,0x80,0x70,0x00,0x00,0x00}, /* ,   */
    {0x08,0x08,0x08,0x08,0x08,0x00}, /* -   */
    {0x00,0x00,0x60,0x60,0x00,0x00}, /* .   */
    {0x20,0x10,0x08,0x04,0x02,0x00}, /* /   */
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, /* 0   */
    {0x00,0x42,0x7F,0x40,0x00,0x00}, /* 1   */
    {0x62,0x51,0x49,0x49,0x46,0x00}, /* 2   */
    {0x22,0x41,0x49,0x49,0x36,0x00}, /* 3   */
    {0x18,0x14,0x12,0x7F,0x10,0x00}, /* 4   */
    {0x27,0x45,0x45,0x45,0x39,0x00}, /* 5   */
    {0x3C,0x4A,0x49,0x49,0x31,0x00}, /* 6   */
    {0x41,0x21,0x11,0x09,0x07,0x00}, /* 7   */
    {0x36,0x49,0x49,0x49,0x36,0x00}, /* 8   */
    {0x46,0x49,0x49,0x29,0x1E,0x00}, /* 9   */
    {0x00,0x00,0x14,0x00,0x00,0x00}, /* :   */
    {0x00,0x80,0x68,0x00,0x00,0x00}, /* ;   */
    {0x08,0x14,0x22,0x41,0x00,0x00}, /* <   */
    {0x14,0x14,0x14,0x14,0x14,0x00}, /* =   */
    {0x00,0x41,0x22,0x14,0x08,0x00}, /* >   */
    {0x02,0x01,0x51,0x09,0x06,0x00}, /* ?   */
    {0x32,0x49,0x79,0x41,0x3E,0x00}, /* @   */
    {0x7E,0x09,0x09,0x09,0x7E,0x00}, /* A   */
    {0x7F,0x49,0x49,0x49,0x36,0x00}, /* B   */
    {0x3E,0x41,0x41,0x41,0x22,0x00}, /* C   */
    {0x7F,0x41,0x41,0x41,0x3E,0x00}, /* D   */
    {0x7F,0x49,0x49,0x49,0x41,0x00}, /* E   */
    {0x7F,0x09,0x09,0x09,0x01,0x00}, /* F   */
    {0x3E,0x41,0x49,0x49,0x3A,0x00}, /* G   */
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, /* H   */
    {0x00,0x41,0x7F,0x41,0x00,0x00}, /* I   */
    {0x30,0x40,0x40,0x40,0x3F,0x00}, /* J   */
    {0x7F,0x08,0x14,0x22,0x41,0x00}, /* K   */
    {0x7F,0x40,0x40,0x40,0x40,0x00}, /* L   */
    {0x7F,0x02,0x04,0x02,0x7F,0x00}, /* M   */
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, /* N   */
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, /* O   */
    {0x7F,0x09,0x09,0x09,0x06,0x00}, /* P   */
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, /* Q   */
    {0x7F,0x09,0x19,0x29,0x46,0x00}, /* R   */
    {0x26,0x49,0x49,0x49,0x32,0x00}, /* S   */
    {0x01,0x01,0x7F,0x01,0x01,0x00}, /* T   */
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, /* U   */
    {0x0F,0x30,0x40,0x30,0x0F,0x00}, /* V   */
    {0x3F,0x40,0x38,0x40,0x3F,0x00}, /* W   */
    {0x63,0x14,0x08,0x14,0x63,0x00}, /* X   */
    {0x03,0x04,0x78,0x04,0x03,0x00}, /* Y   */
    {0x61,0x51,0x49,0x45,0x43,0x00}, /* Z   */
    {0x00,0x7F,0x41,0x41,0x41,0x00}, /* [   */
    {0x02,0x04,0x08,0x10,0x20,0x00}, /* \   */
    {0x00,0x41,0x41,0x41,0x7F,0x00}, /* ]   */
    {0x04,0x02,0x01,0x02,0x04,0x00}, /* ^   */
    {0x80,0x80,0x80,0x80,0x80,0x00}, /* _   */
    {0x00,0x03,0x07,0x00,0x00,0x00}, /* `   */
    {0x20,0x54,0x54,0x54,0x78,0x00}, /* a   */
    {0x7F,0x48,0x44,0x44,0x38,0x00}, /* b   */
    {0x38,0x44,0x44,0x44,0x28,0x00}, /* c   */
    {0x38,0x44,0x44,0x48,0x7F,0x00}, /* d   */
    {0x38,0x54,0x54,0x54,0x18,0x00}, /* e   */
    {0x00,0x08,0x7E,0x09,0x02,0x00}, /* f   */
    {0x18,0xA4,0xA4,0xA4,0x7C,0x00}, /* g   */
    {0x7F,0x08,0x04,0x04,0x78,0x00}, /* h   */
    {0x00,0x44,0x7D,0x40,0x00,0x00}, /* i   */
    {0x00,0x80,0x84,0x7D,0x00,0x00}, /* j   */
    {0x7F,0x10,0x28,0x44,0x00,0x00}, /* k   */
    {0x00,0x41,0x7F,0x40,0x00,0x00}, /* l   */
    {0x7C,0x04,0x18,0x04,0x78,0x00}, /* m   */
    {0x7C,0x08,0x04,0x04,0x78,0x00}, /* n   */
    {0x38,0x44,0x44,0x44,0x38,0x00}, /* o   */
    {0xFC,0x24,0x24,0x24,0x18,0x00}, /* p   */
    {0x18,0x24,0x24,0x18,0xFC,0x00}, /* q   */
    {0x7C,0x08,0x04,0x04,0x08,0x00}, /* r   */
    {0x48,0x54,0x54,0x54,0x24,0x00}, /* s   */
    {0x04,0x04,0x3F,0x44,0x24,0x00}, /* t   */
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, /* u   */
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, /* v   */
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, /* w   */
    {0x44,0x28,0x10,0x28,0x44,0x00}, /* x   */
    {0x1C,0xA0,0xA0,0xA0,0x7C,0x00}, /* y   */
    {0x44,0x64,0x54,0x4C,0x44,0x00}, /* z   */
    {0x08,0x36,0x41,0x41,0x00,0x00}, /* {   */
    {0x00,0x00,0x77,0x00,0x00,0x00}, /* |   */
    {0x00,0x41,0x41,0x36,0x08,0x00}, /* }   */
    {0x08,0x04,0x08,0x10,0x08,0x00}, /* ~   */
};

/* ════════════════════════════ 字符显示 ════════════════════════════ */

void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    if (ch < ' ' || ch > '~') ch = ' ';
    uint8_t idx = ch - ' ';

    if (size == 8) {
        // 原生 6×8 字体
        for (uint8_t c = 0; c < 6; c++) {
            uint8_t d = font_6x8[idx][c];
            for (uint8_t r = 0; r < 8; r++) {
                if (d & (1 << r))
                    OLED_DrawPoint(x + c, y + r, 1);
            }
        }
    } else if (size == 12 || size == 16) {
        // 1206 (6×12) / 1608 (8×16)：PCtoLCD2002 位流格式
        // MSB 先，按列从上到下。满一列后当前字节剩余位丢弃，下字节起新列
        uint8_t  h = (size == 12) ? OLED_FONT_12_HEIGHT : OLED_FONT_16_HEIGHT;
        uint8_t  sz = (size == 12) ? OLED_FONT_12_SIZE : OLED_FONT_16_SIZE;
        const uint8_t *f = (size == 12)
            ? font_1206[idx]
            : font_1608[idx];

        uint8_t col = 0, row = 0;
        for (uint8_t i = 0; i < sz; i++) {
            uint8_t d = f[i];
            for (uint8_t b = 0; b < 8; b++) {
                if (d & 0x80)
                    OLED_DrawPoint(x + col, y + row, 1);
                d <<= 1;
                row++;
                if (row == h) { row = 0; col++; break; }  // ← break 丢弃本字节剩余位
            }
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    uint8_t w = (size == 16) ? 8 : 6;
    while (*str) {
        if (x + w > OLED_WIDTH) { x = 0; y += size; }
        if (y + size > OLED_HEIGHT) break;
        OLED_ShowChar(x, y, *str, size);
        x += w;
        str++;
    }
}

static uint32_t ipow10(uint8_t n)
{
    uint32_t r = 1;
    while (n--) r *= 10;
    return r;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t w = (size == 16) ? 8 : 6;
    uint8_t first = 1;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t digit = (num / ipow10(len - i - 1)) % 10;
        if (first && i < len - 1 && digit == 0)
            OLED_ShowChar(x + w * i, y, ' ', size);
        else {
            OLED_ShowChar(x + w * i, y, '0' + digit, size);
            first = 0;
        }
    }
}
