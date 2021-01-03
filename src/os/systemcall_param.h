/**
 * @file システムコールパラメータ定義
 * @author 
 */

#ifndef SYSTEMCALL_PARAM_H_
#define SYSTEMCALL_PARAM_H_

#define SYSCALL_NONE        0
#define SYSCALL_WAIT_MSEC   1

/**
 * システムコールパラメータ
 */
union system_call_param {
	struct {
		uint32_t begin;
		uint32_t wait_millis;
	} wait;
};



#endif /* SYSTEMCALL_PARAM_H_ */
