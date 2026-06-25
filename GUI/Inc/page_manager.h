#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H
#include <stdint.h>

#define NAV_H      50
#define BTN_W      (tftlcd.width / PAGE_COUNT)

typedef enum {
    PAGE_FILE,
    PAGE_PERIPHERAL,
    PAGE_SENSOR,
    PAGE_DEBUG,
    PAGE_COUNT,
} PageID;

typedef struct {
    const char *label;
    void (*init)(void);
    void (*loop)(uint16_t x, uint16_t y, uint8_t pressed);
} Page;

void PageManager_Init(void);
void PageManager_Loop(void);
uint8_t PageManager_IsPage(PageID id);

#endif
