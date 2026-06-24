//
// Created by 35156 on 2026/6/24.
//

#ifndef FONT_FLASH_H
#define FONT_FLASH_H
#include <stdint.h>
#include "w25qxx.h"
#include "serial.h"

/* 字库存放地址 */
#define FONT16_ADDR      0x000000   // 16x16 字库基址 (占 ~656KB)
#define FONT16_SIZE_MAX  (1 * 1024 * 1024)  // 预留 1MB

#define FONT24_ADDR      (FONT16_ADDR + FONT16_SIZE_MAX)
#define FONT24_SIZE_MAX  (2 * 1024 * 1024)  // 预留 2MB

/* 字模尺寸 */
#define FONT16_BYTES     32   // 16×16 / 8 = 32 字节/字
#define FONT24_BYTES     72   // 24×24 / 8 = 72 字节/字

/* Unicode 映射表在 Flash 中的地址 (Python 工具烧录) */
#define UNI_MAP16_ADDR   0x600000
#define UNI_MAP24_ADDR   0x700000

/* ===== 公开 API ===== */

void FontFlash_Init(void);

/* 调试：获取接收统计 */
void FontFlash_GetInfo(uint32_t *total, uint32_t *ok, uint32_t *bad);

/* 串口接收字库数据 (在主循环中调用) */
void FontFlash_RecvProc(Serial_t *ser);

/* 计算 GBK 码在字库中的偏移 */
uint32_t FontFlash_GetOffset(uint8_t hi, uint8_t lo, uint16_t font_bytes);

/* 从 Flash 读字模并画到 TFT */
void FontFlash_ShowChar16(uint16_t x, uint16_t y, uint8_t hi, uint8_t lo,
                          uint16_t color, uint16_t bg);
void FontFlash_ShowChar24(uint16_t x, uint16_t y, uint8_t hi, uint8_t lo,
                          uint16_t color, uint16_t bg);

/* 字符串绘制 (str 为 GBK 编码) */
void FontFlash_ShowString16(uint16_t x, uint16_t y, const uint8_t *gbk_str,
                            uint16_t color, uint16_t bg);
void FontFlash_ShowString24(uint16_t x, uint16_t y, const uint8_t *gbk_str,
                            uint16_t color, uint16_t bg);

/* UTF-8 字符串绘制 — size 同时控制 ASCII 和汉字大小 (汉字: ≤16→16x16, ≥24→24x24) */
void FontFlash_ShowString_UTF8(uint16_t x, uint16_t y, const char *utf8_str,
                                uint8_t size, uint16_t color);

#endif //FONT_FLASH_H
