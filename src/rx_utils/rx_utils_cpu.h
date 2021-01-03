/**
 * @file CPUユーティリティ
 * @author 
 */

#ifndef RX_UTIL_CPU_H_
#define RX_UTIL_CPU_H_

#include "rx_types.h"
#include <machine.h>

#ifdef __cplusplus
extern "C" {
#endif

void rx_util_nop(void);
void rx_util_wait(void);
void rx_util_enable_interrupt(void);
void rx_util_disable_interrupt(void);
int rx_util_is_interrupt_enable(void);
uint32_t rx_util_get_psw(void);
void rx_util_set_psw(uint32_t psw);
int rx_util_get_ipl(void);
void rx_util_set_ipl(uint8_t ipl);
int rx_util_is_user_mode(void);

#ifdef __cplusplus
}
#endif


#endif /* RX_UTIL_CPU_H_ */
