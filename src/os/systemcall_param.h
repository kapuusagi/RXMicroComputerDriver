/**
 * @file システムコールパラメータ定義
 * @author 
 */

#ifndef SYSTEMCALL_PARAM_H_
#define SYSTEMCALL_PARAM_H_

#include "../rx_utils/rx_types.h"

#define SYSCALL_NONE        0
#define SYSCALL_WAIT_MSEC   1
#define SYSCALL_WAIT_OBJECT 2

struct wait_object;

/**
 * システムコールパラメータ
 */
union system_call_param {
	struct {
		uint32_t begin;
		uint32_t wait_millis;
	} wait;
	struct wait_object *wait_object;
};



#endif /* SYSTEMCALL_PARAM_H_ */
