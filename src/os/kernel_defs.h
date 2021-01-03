/**
 * @file カーネルの型指定などの宣言を記述する。
 * @author 
 */

#ifndef KERNEL_DEFS_H_
#define KERNEL_DEFS_H_

#include "../rx_utils/rx_types.h"

typedef uint16_t task_id_t;
typedef void (*task_func_t)(void *arg);

typedef uint32_t stack_type_t;


#endif /* OS_KERNEL_DEFS_H_ */
