#ifndef ZHONGSOU_SAVE_PAGE_H
#define ZHONGSOU_SAVE_PAGE_H

#include <stdint.h>

#define UNKNOWN_SAVE_TIME 0

uint64_t page_save_time(uint64_t);

void wait_page_save_message(void);

#endif // ZHONGSOU_SAVE_PAGE_H
