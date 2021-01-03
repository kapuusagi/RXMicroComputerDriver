/**
 * @file タスク用API宣言を記述する。
 *       各タスクでのインクルードと使用を想定する。
 * @author 
 */

#ifndef KERNEL_API_H_
#define KERNEL_API_H_

#include "semaphore.h"

void sleep(uint32_t wait_millis);
void yield(void);


#endif /* KERNEL_API_H_ */
