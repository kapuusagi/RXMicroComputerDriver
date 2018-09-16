#ifndef DRV_CMT_H
#define DRV_CMT_H

#include "../../rx_utils/rx_types.h"

enum {
	TIMER_NO_1 = 0,
	TIMER_NO_2
};

typedef void (*timer_handler_t)(void *arg);

void drv_cmt_init(void);
void drv_cmt_start(uint8_t timer_no, uint32_t interval_msec,
		timer_handler_t handler, void *arg);
void drv_cmt_stop(uint8_t timer_no);

#endif /* DRV_CMT_H */
