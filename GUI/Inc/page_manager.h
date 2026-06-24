#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

typedef enum {
    PAGE_FILE,
    PAGE_PERIPHERAL,
    PAGE_SENSOR,
    PAGE_COUNT
} PageID;

typedef struct {
    const char *label;
    void (*init)(void);
    void (*loop)(void);
} Page;

void PageManager_Init(void);
void PageManager_Loop(void);

#endif
