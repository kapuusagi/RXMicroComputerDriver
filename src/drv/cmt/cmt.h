/**
 * @file CMTドライバ
 */
#ifndef DRV_CMT_H
#define DRV_CMT_H

#include "../../rx_utils/rx_types.h"

enum {
	TIMER_1 = 0,
	TIMER_2,
	NUM_TIMERS /* タイマー数定義用 */
};

typedef void (*msec_handler_t)(void);
typedef void (*timer_handler_t)(uint32_t elapse_millis);

#ifdef __cplusplus
extern "C" {
#endif

void drv_cmt_init(void);
void drv_cmt_destroy(void);
void drv_cmt_set_msec_handler(msec_handler_t handler);
void drv_cmt_start(uint8_t id, uint32_t interval_msec, timer_handler_t handler);
void drv_cmt_stop(uint8_t id);

void drv_cmt_delay_ms(uint32_t msec);
void drv_cmt_delay_us(uint16_t usec);
uint32_t drv_cmt_get_counter(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_CMT_H */
