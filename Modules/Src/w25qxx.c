//
// W25Q128 SPI Flash 驱动 (合并 spi_bus)
//

#include "w25qxx.h"
#include "main.h"
#include "spi.h"

/* ===================================================================
 * 指令
 * =================================================================== */
#define CMD_WRITE_ENABLE     0x06
#define CMD_READ_STATUS1     0x05
#define CMD_READ_DATA        0x03
#define CMD_PAGE_PROGRAM     0x02
#define CMD_SECTOR_ERASE     0x20
#define CMD_CHIP_ERASE       0xC7
#define CMD_JEDEC_ID         0x9F
#define CMD_POWER_DOWN       0xB9
#define CMD_RELEASE_PWR_DN   0xAB

#define WIP_BIT  0x01

/* ===================================================================
 * 实例
 * =================================================================== */
W25QXX w25q128 = {
    .hspi    = &hspi1,
    .cs_port = W25Q128_CS_GPIO_Port,
    .cs_pin  = W25Q128_CS_Pin,
};

/* ===================================================================
 * CS 控制 (原 spi_bus)
 * =================================================================== */
void W25QXX_CS_Low(W25QXX *f)
{
    HAL_GPIO_WritePin(f->cs_port, f->cs_pin, GPIO_PIN_RESET);
}

void W25QXX_CS_High(W25QXX *f)
{
    HAL_GPIO_WritePin(f->cs_port, f->cs_pin, GPIO_PIN_SET);
}

void W25QXX_ReadWrite(W25QXX *f, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (tx && rx) {
        HAL_SPI_TransmitReceive(f->hspi, (uint8_t *)tx, rx, len, HAL_MAX_DELAY);
    } else if (tx) {
        HAL_SPI_Transmit(f->hspi, (uint8_t *)tx, len, HAL_MAX_DELAY);
    } else if (rx) {
        HAL_SPI_Receive(f->hspi, rx, len, HAL_MAX_DELAY);
    }
}

/* ===================================================================
 * 内部辅助
 * =================================================================== */
static void w25q_select(W25QXX *f)  { W25QXX_CS_Low(f); }
static void w25q_deselect(W25QXX *f) { W25QXX_CS_High(f); }

static uint8_t w25q_read_status(W25QXX *f)
{
    uint8_t tx[2] = {CMD_READ_STATUS1, 0xFF};
    uint8_t rx[2];
    w25q_select(f);
    W25QXX_ReadWrite(f, tx, rx, 2);
    w25q_deselect(f);
    return rx[1];
}

static void w25q_wait_ready(W25QXX *f)
{
    uint32_t timeout = 1000000;
    while ((w25q_read_status(f) & WIP_BIT) && --timeout);
}

static void w25q_write_enable(W25QXX *f)
{
    uint8_t cmd = CMD_WRITE_ENABLE;
    w25q_select(f);
    W25QXX_ReadWrite(f, &cmd, NULL, 1);
    w25q_deselect(f);
}

static void w25q_send_addr(W25QXX *f, uint8_t cmd, uint32_t addr)
{
    uint8_t buf[4] = { cmd,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr) };
    W25QXX_ReadWrite(f, buf, NULL, 4);
}

/* ===================================================================
 * 公开 API
 * =================================================================== */
void W25QXX_Init(W25QXX *f)
{
    /* 初始化 CS 引脚 */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = f->cs_pin;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(f->cs_port, &gpio);
    W25QXX_CS_High(f);

    f->id       = W25QXX_ReadID(f);
    f->capacity = W25QXX_CAPACITY;
}

uint16_t W25QXX_ReadID(W25QXX *f)
{
    uint8_t tx[4] = {CMD_JEDEC_ID, 0xFF, 0xFF, 0xFF};
    uint8_t rx[4] = {0};
    w25q_select(f);
    W25QXX_ReadWrite(f, tx, rx, 4);
    w25q_deselect(f);
    return ((uint16_t)rx[2] << 8) | rx[3];
}

void W25QXX_Read(W25QXX *f, uint32_t addr, uint8_t *buf, uint32_t len)
{
    w25q_select(f);
    w25q_send_addr(f, CMD_READ_DATA, addr);
    W25QXX_ReadWrite(f, NULL, buf, len);
    w25q_deselect(f);
}

void W25QXX_EraseSector(W25QXX *f, uint32_t addr)
{
    w25q_write_enable(f);
    w25q_select(f);
    w25q_send_addr(f, CMD_SECTOR_ERASE, addr);
    w25q_deselect(f);
    w25q_wait_ready(f);
}

void W25QXX_WritePage(W25QXX *f, uint32_t addr, const uint8_t *buf, uint16_t len)
{
    if (len == 0 || len > 256) return;
    w25q_write_enable(f);
    w25q_select(f);
    w25q_send_addr(f, CMD_PAGE_PROGRAM, addr);
    W25QXX_ReadWrite(f, buf, NULL, len);
    w25q_deselect(f);
    w25q_wait_ready(f);
}

void W25QXX_Write(W25QXX *f, uint32_t addr, const uint8_t *buf, uint32_t len)
{
    static int32_t last_erased_sec = -1;

    uint32_t end      = addr + len;
    uint32_t sec_start = addr / W25QXX_SECTOR_SIZE;
    uint32_t sec_end   = (end - 1) / W25QXX_SECTOR_SIZE;

    for (uint32_t s = sec_start; s <= sec_end; s++) {
        if ((int32_t)s != last_erased_sec) {
            W25QXX_EraseSector(f, s * W25QXX_SECTOR_SIZE);
            last_erased_sec = (int32_t)s;
        }
    }

    uint32_t pos = 0;
    while (pos < len) {
        uint32_t cur_addr    = addr + pos;
        uint16_t page_remain = 256 - (cur_addr & 0xFF);
        uint16_t chunk       = (len - pos) > page_remain ? page_remain : (uint16_t)(len - pos);
        W25QXX_WritePage(f, cur_addr, buf + pos, chunk);
        pos += chunk;
    }
}

void W25QXX_EraseAll(W25QXX *f)
{
    uint8_t cmd = CMD_CHIP_ERASE;
    w25q_write_enable(f);
    w25q_select(f);
    W25QXX_ReadWrite(f, &cmd, NULL, 1);
    w25q_deselect(f);
    w25q_wait_ready(f);
}

void W25QXX_PowerDown(W25QXX *f)
{
    uint8_t cmd = CMD_POWER_DOWN;
    w25q_select(f);
    W25QXX_ReadWrite(f, &cmd, NULL, 1);
    w25q_deselect(f);
}

void W25QXX_WakeUp(W25QXX *f)
{
    uint8_t cmd = CMD_RELEASE_PWR_DN;
    w25q_select(f);
    W25QXX_ReadWrite(f, &cmd, NULL, 1);
    w25q_deselect(f);
}
