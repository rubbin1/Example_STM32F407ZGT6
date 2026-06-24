//
// W25Q128 SPI Flash 驱动 (合并 spi_bus)
//

#ifndef W25QXX_H
#define W25QXX_H
#include <stdint.h>
#include "stm32f4xx_hal.h"

#define W25QXX_SECTOR_SIZE    4096
#define W25QXX_BLOCK_SIZE     65536
#define W25QXX_CAPACITY       (16 * 1024 * 1024)

/* Flash 实例: SPI + CS 控制 + 属性 */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *cs_port;
    uint16_t           cs_pin;
    uint32_t           capacity;
    uint16_t           id;
} W25QXX;

extern W25QXX w25q128;

void  W25QXX_Init(W25QXX *f);
void  W25QXX_CS_Low(W25QXX *f);
void  W25QXX_CS_High(W25QXX *f);
void  W25QXX_ReadWrite(W25QXX *f, const uint8_t *tx, uint8_t *rx, uint16_t len);

uint16_t W25QXX_ReadID(W25QXX *f);
void     W25QXX_Read(W25QXX *f, uint32_t addr, uint8_t *buf, uint32_t len);
void     W25QXX_EraseSector(W25QXX *f, uint32_t addr);
void     W25QXX_WritePage(W25QXX *f, uint32_t addr, const uint8_t *buf, uint16_t len);
void     W25QXX_Write(W25QXX *f, uint32_t addr, const uint8_t *buf, uint32_t len);
void     W25QXX_EraseAll(W25QXX *f);
void     W25QXX_PowerDown(W25QXX *f);
void     W25QXX_WakeUp(W25QXX *f);

#endif
