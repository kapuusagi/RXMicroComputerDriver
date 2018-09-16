#ifndef DRV_TIMER_H
#define DRV_TIMER_H

#include "../../rx_utils/rx_types.h"

enum {
	TIMER_NO_1 = 0,
};

typedef void (*timer_handler_t)(void *arg);

void drv_timer_init(void);
void drv_timer_start(uint8_t timer_no, uint32_t interval_msec,
		timer_handler_t handler, void *arg);
void drv_timer_stop(uint8_t timer_no);

#endif /* DRV_TIMER_H */
