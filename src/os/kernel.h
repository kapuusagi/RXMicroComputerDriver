/**
 * @file カーネルヘッダ
 * @author
 */
#ifndef KERNEL_H_
#define KERNEL_H_

#include "../rx_utils/rx_types.h"
#include "kernel_defs.h"
#include "kernel_config.h"



#ifdef __cplusplus
extern "C" {
#endif

void kernel_init(void);

int kernel_register_task(uint16_t priority,
		task_func_t func, void *arg,
		stack_type_t *stack, uint32_t stack_size);

void kernel_start_scheduler(void);
void kernel_request_swtich(void);


/* task APIs */
void kernel_sysc_wait(uint32_t wait_millis);

#ifdef __cplusplus
}
#endif



#endif /* KERNEL_H_ */
