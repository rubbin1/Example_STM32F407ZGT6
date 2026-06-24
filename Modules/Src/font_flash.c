//
// Created by 35156 on 2026/6/24.
//

#include "font_flash.h"
#include "tft_lcd.h"

/* GBK 第二字节的有效个数: 0x40-0x7E(63) + 0x80-0xFE(127) = 190 */
#define GBK_HI_FIRST    0x81
#define GBK_HI_COUNT    126
#define GBK_LO_COUNT    190

void FontFlash_Init(void)
{
    /* W25Q128 已在 W25QXX_Init 中初始化 */
}

uint32_t FontFlash_GetOffset(uint8_t hi, uint8_t lo, uint16_t font_bytes)
{
    uint16_t hi_idx = hi - GBK_HI_FIRST;  // 0 ~ 125

    uint16_t lo_idx;
    if (lo <= 0x7E) {
        lo_idx = lo - 0x40;               // 0 ~ 62
    } else {
        lo_idx = lo - 0x41;               // 63 ~ 189 (跳过 0x7F 空隙)
    }

    return (uint32_t)(hi_idx * GBK_LO_COUNT + lo_idx) * font_bytes;
}

static void font_draw16(uint16_t x, uint16_t y, const uint8_t *font, uint16_t color, uint16_t bg)
{
    for (uint8_t row = 0; row < 16; row++) {
        uint8_t left  = font[row * 2];       // 第 0~7  列
        uint8_t right = font[row * 2 + 1];   // 第 8~15 列

        for (uint8_t col = 0; col < 8; col++) {
            TFTLCD_DrawPoint(x + col,     y + row, (left  & (0x80 >> col)) ? color : bg);
            TFTLCD_DrawPoint(x + col + 8, y + row, (right & (0x80 >> col)) ? color : bg);
        }
    }
}

static void font_draw24(uint16_t x, uint16_t y, const uint8_t *font, uint16_t color, uint16_t bg)
{
    for (uint8_t row = 0; row < 24; row++) {
        uint8_t b0 = font[row * 3];       // 第 0~7  列
        uint8_t b1 = font[row * 3 + 1];   // 第 8~15 列
        uint8_t b2 = font[row * 3 + 2];   // 第 16~23列

        for (uint8_t col = 0; col < 8; col++) {
            TFTLCD_DrawPoint(x + col,      y + row, (b0 & (0x80 >> col)) ? color : bg);
            TFTLCD_DrawPoint(x + col + 8,  y + row, (b1 & (0x80 >> col)) ? color : bg);
            TFTLCD_DrawPoint(x + col + 16, y + row, (b2 & (0x80 >> col)) ? color : bg);
        }
    }
}

void FontFlash_ShowChar16(uint16_t x, uint16_t y, uint8_t hi, uint8_t lo,
                          uint16_t color, uint16_t bg)
{
    uint32_t offset = FontFlash_GetOffset(hi, lo, FONT16_BYTES);
    uint8_t font[FONT16_BYTES];
    W25QXX_Read(&w25q128, FONT16_ADDR + offset, font, FONT16_BYTES);
    font_draw16(x, y, font, color, bg);
}

void FontFlash_ShowChar24(uint16_t x, uint16_t y, uint8_t hi, uint8_t lo,
                          uint16_t color, uint16_t bg)
{
    uint32_t offset = FontFlash_GetOffset(hi, lo, FONT24_BYTES);
    uint8_t font[FONT24_BYTES];
    W25QXX_Read(&w25q128, FONT24_ADDR + offset, font, FONT24_BYTES);
    font_draw24(x, y, font, color, bg);
}

void FontFlash_ShowString16(uint16_t x, uint16_t y, const uint8_t *gbk_str,
                            uint16_t color, uint16_t bg)
{
    uint16_t cx = x, cy = y;
    while (*gbk_str) {
        if (*gbk_str < 0x80) {
            TFTLCD_ShowChar(cx, cy, *gbk_str, 8, color);
            cx += 8;
            gbk_str++;
        } else {
            uint8_t hi = *gbk_str++;
            uint8_t lo = *gbk_str++;
            FontFlash_ShowChar16(cx, cy, hi, lo, color, bg);
            cx += 16;
        }
        /* 换行 */
        if (cx + 16 > tftlcd.width) {
            cx = x;
            cy += 16;
        }
    }
}

void FontFlash_ShowString24(uint16_t x, uint16_t y, const uint8_t *gbk_str,
                            uint16_t color, uint16_t bg)
{
    uint16_t cx = x, cy = y;
    while (*gbk_str) {
        if (*gbk_str < 0x80) {
            TFTLCD_ShowChar(cx, cy, *gbk_str, 12, color);
            cx += 12;
            gbk_str++;
        } else {
            uint8_t hi = *gbk_str++;
            uint8_t lo = *gbk_str++;
            FontFlash_ShowChar24(cx, cy, hi, lo, color, bg);
            cx += 24;
        }
        if (cx + 24 > tftlcd.width) {
            cx = x;
            cy += 24;
        }
    }
}

/* ========================================================================
 * 串口协议接收 -- PC → 串口 → Flash
 * 帧格式: AA 55 | addr[4 LE] | len[2 LE] | data[len] | xor | 55 AA
 * 应答: 0x06=ACK  0x15=NAK
 * ======================================================================== */
typedef enum {
    F_STATE_HDR1,
    F_STATE_HDR2,
    F_STATE_ADDR,
    F_STATE_LEN,
    F_STATE_DATA,
    F_STATE_XOR,
    F_STATE_TAIL1,
    F_STATE_TAIL2,
} font_recv_state_t;

static font_recv_state_t f_state = F_STATE_HDR1;
static uint32_t f_addr;
static uint16_t f_len;
static uint16_t f_idx;
static uint8_t  f_buf[256];
static uint8_t  f_xor_in;
static uint32_t f_total_bytes;  // debug: 收到的总字节数
static uint32_t f_frames_ok;    // debug: 成功帧数
static uint32_t f_frames_bad;   // debug: 失败帧数

void FontFlash_GetInfo(uint32_t *total, uint32_t *ok, uint32_t *bad)
{
    *total = f_total_bytes;
    *ok    = f_frames_ok;
    *bad   = f_frames_bad;
}

void FontFlash_RecvProc(Serial_t *ser)
{
    while (Serial_Available(ser)) {
        uint8_t b = Serial_ReadByte(ser);
        f_total_bytes++;

        switch (f_state) {
        case F_STATE_HDR1:
            if (b == 0xAA) f_state = F_STATE_HDR2;
            break;

        case F_STATE_HDR2:
            if (b == 0x55) f_state = F_STATE_ADDR;
            else f_state = F_STATE_HDR1;
            f_idx = 0;
            break;

        case F_STATE_ADDR:
            ((uint8_t *)&f_addr)[f_idx++] = b;
            if (f_idx == 4) { f_state = F_STATE_LEN; f_idx = 0; }
            break;

        case F_STATE_LEN:
            ((uint8_t *)&f_len)[f_idx++] = b;
            if (f_idx == 2) {
                if (f_len == 0 || f_len > 256)
                    f_state = F_STATE_HDR1;
                else
                    { f_state = F_STATE_DATA; f_idx = 0; f_xor_in = 0; }
            }
            break;

        case F_STATE_DATA:
            f_buf[f_idx++] = b;
            f_xor_in ^= b;
            if (f_idx == f_len) f_state = F_STATE_XOR;
            break;

        case F_STATE_XOR: {
            uint8_t rx_xor = b;
            f_state = F_STATE_TAIL1;
            if (rx_xor == f_xor_in) {
                f_frames_ok++;
                W25QXX_Write(&w25q128, f_addr, f_buf, f_len);
                Serial_SendByte(ser, 0x06);
            } else {
                f_frames_bad++;
                Serial_SendByte(ser, 0x15);
            }
            break;
        }

        case F_STATE_TAIL1:
            if (b == 0x55) f_state = F_STATE_TAIL2;
            else f_state = F_STATE_HDR1;
            break;

        case F_STATE_TAIL2:
            f_state = F_STATE_HDR1;
            break;
        }
    }
}

/* ========================================================================
 * UTF-8 → GBK 转换 (原 font_conv)
 * ======================================================================== */
static uint16_t utf8_decode(const uint8_t **p)
{
    uint8_t c = *(*p)++;
    if (c < 0x80) return c;
    if (c < 0xE0) {
        uint8_t c2 = *(*p)++;
        return (uint16_t)(((c & 0x1F) << 6) | (c2 & 0x3F));
    }
    uint8_t c2 = *(*p)++;
    uint8_t c3 = *(*p)++;
    return (uint16_t)(((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F));
}

static int32_t font_uni_to_offset(uint16_t unicode, uint32_t map_addr, uint16_t bpc)
{
    if (unicode == 0) return -1;

    uint8_t hdr[4];
    W25QXX_Read(&w25q128, map_addr, hdr, 4);
    uint32_t count = (uint32_t)hdr[0] | ((uint32_t)hdr[1] << 8)
                   | ((uint32_t)hdr[2] << 16) | ((uint32_t)hdr[3] << 24);
    if (count == 0 || count > 50000) return -1;  /* 映射表不存在/未烧录 */

    int32_t lo = 0, hi = (int32_t)count - 1;
    uint8_t rec[4];
    while (lo <= hi) {
        int32_t mid = lo + (hi - lo) / 2;
        W25QXX_Read(&w25q128, map_addr + 4 + (uint32_t)mid * 4, rec, 4);
        uint16_t uni = ((uint16_t)rec[1] << 8) | rec[0];
        uint16_t idx = ((uint16_t)rec[3] << 8) | rec[2];
        if (uni == unicode) return (int32_t)idx * bpc;
        if (uni < unicode) lo = mid + 1;
        else               hi = mid - 1;
    }
    return -1;
}

/* ========================================================================
 * UTF-8 字符串绘制 — size 控制 ASCII 和汉字大小
 *   size ≤ 16 → 汉字 16x16,  size ≥ 24 → 汉字 24x24
 * ======================================================================== */
void FontFlash_ShowString_UTF8(uint16_t x, uint16_t y, const char *utf8_str,
                                uint8_t size, uint16_t color)
{
    uint8_t  big = (size >= 24);                    /* 0=16×16, 1=24×24 */
    uint16_t cw  = big ? 24 : 16;                   /* 汉字宽 */
    uint16_t fb  = big ? FONT24_BYTES : FONT16_BYTES;
    uint32_t fa  = big ? FONT24_ADDR   : FONT16_ADDR;
    uint32_t ma  = big ? UNI_MAP24_ADDR : UNI_MAP16_ADDR;

    const uint8_t *p = (const uint8_t *)utf8_str;
    uint16_t cx = x, cy = y;

    while (*p) {
        if (*p < 0x80) {
            TFTLCD_ShowChar(cx, cy, *p++, size, color);
            cx += (size >= 16) ? 12 : (size >= 12) ? 12 : 6;
        } else {
            uint16_t unicode = utf8_decode(&p);
            int32_t offset = font_uni_to_offset(unicode, ma, fb);
            if (offset >= 0) {
                uint8_t font[72];  /* max(FONT16_BYTES, FONT24_BYTES) */
                W25QXX_Read(&w25q128, fa + (uint32_t)offset, font, fb);
                if (big) font_draw24(cx, cy, font, color, tft_bg_color);
                else     font_draw16(cx, cy, font, color, tft_bg_color);
            }
            cx += cw;
        }
        if (cx + cw > tftlcd.width) { cx = x; cy += cw; }
    }
}
