/**
 * @file セマフォ実装
 *       待ちを解放する順番はFIFOで実装してある。
 *       プライオリティは考慮してない。
 * @author 
 */

#include "kernel.h"
#include "semaphore.h"
#include "../rx_utils/error_code.h"

static void sem_update(void *arg);

/**
 * セマフォを初期化する。
 *
 * @param sem セマフォ
 * @param initial_count 初期値
 */
void
sem_init(struct semaphore *sem, uint16_t initial_count)
{
	wait_object_init(&(sem->wait_obj), sem_update, sem);
	sem->count = initial_count;

	return;
}

/**
 * セマフォを破棄する。
 *
 * @param sem セマフォ
 */
void
sem_destroy(struct semaphore *sem)
{
	wait_object_destroy(&(sem->wait_obj));
	sem->wait_obj.update = sem_update;
	sem->wait_obj.arg = sem;
	sem->count = 0;

	return;
}

/**
 * セマフォを待つ。もしセマフォが取得できない場合にはエラーを返す。
 *
 * @param sem セマフォ
 * @return セマフォが取得できた場合には0、取得できなかった場合にはエラーコードを返す。
 */
int
sem_trywait(struct semaphore *sem)
{
	if ((sem->count > 0) && !wait_object_has_wait_entries(&(sem->wait_obj))) {
		return sem_wait(sem);
	} else {
		return ERR_OPERATION_STATE;
	}
}

/**
 * セマフォを待つ
 *
 * @param sem セマフォ
 */
int
sem_wait(struct semaphore *sem)
{
	kernel_sysc_wait_object(&(sem->wait_obj));
	return 0;
}

/**
 * セマフォをインクリメントする。
 *
 * @param sem セマフォ
 */
void
sem_post(struct semaphore *sem)
{
	kernel_disable_context_switch();
	sem->count++;
	kernel_enable_context_switch();
	kernel_request_swtich();
}

/**
 * セマフォを更新する。
 *
 * @param arg セマフォ
 */
static void
sem_update(void *arg)
{
	struct semaphore *sem = (struct semaphore*)(arg);

	while ((sem->count > 0) && wait_object_has_wait_entries(&(sem->wait_obj))) {
		sem->count--;
		wait_object_release_one(&(sem->wait_obj));
	}
}

