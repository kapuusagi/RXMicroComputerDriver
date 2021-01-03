/**
 * @file タスク用API定義を記述する。
 * @author 
 */

#include "kernel.h"
#include "kernel_api.h"

/**
 * 指定ミリ秒待機する
 *
 * @param wait_millis ミリ秒
 */
void
sleep(uint32_t wait_millis)
{
	kernel_sysc_wait(wait_millis);
}

/**
 * CPUの使用権を委譲する。
 */
void
yield(void)
{
	kernel_sysc_yield();
}
