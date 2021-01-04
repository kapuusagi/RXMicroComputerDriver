/**
 * @file ミューテックス
 * @author 
 */
#include "../rx_utils/error_code.h"
#include "task.h"
#include "kernel.h"
#include "mutex.h"

static void mutex_update(void *arg);

/**
 * ミューテックスを初期化する。
 *
 * @param m ミューテックス
 */
void
mutex_init(struct mutex *m)
{
	wait_object_init(&(m->wait_object), mutex_update, m);
	m->owner_task_id = 0;

	return;
}

/**
 * ミューテックスを破棄する。
 *
 * @param m ミューテックス
 */
void
mutex_destroy(struct mutex *m)
{
	wait_object_destroy(&(m->wait_object));

	return ;
}
/**
 * ミューテックスをロックする。
 * すぐに所有権が取得できない場合、待機せずに制御を返す。
 *
 * @param m ミューテックス
 * @return 成功した場合には0、失敗した場合にはエラー番号。
 */
int
mutex_trylock(struct mutex *m)
{
	int self_task_id = kernel_get_self_id();
	if (m->owner_task_id == 0) {
		mutex_lock(m);
		return (m->owner_task_id == self_task_id);
	} else {
		return ERR_OPERATION_STATE;
	}
}

/**
 * ミューテックスをロックする。
 * 所有権が取得できない場合、取得できるまで待機する。
 *
 * @param m ミューテックス
 * @return 成功した場合には0、失敗した場合にはエラー番号。
 */
int
mutex_lock(struct mutex *m)
{
	int self_task_id = kernel_get_self_id();
	while (m->owner_task_id != self_task_id) {
	    kernel_sysc_wait_object(&(m->wait_object));
	}
	return 0;
}

/**
 * ミューテックスをアンロックする。
 *
 * @param m ミューテックス
 * @return 成功した場合には0、失敗した場合にはエラー番号。
 */
int
mutex_unlock(struct mutex *m)
{
	int self_task_id = kernel_get_self_id();
	if (m->owner_task_id != self_task_id) {
		return ERR_NOT_OWNER;
	}

	kernel_disable_context_switch();
	m->owner_task_id = 0;
	kernel_enable_context_switch();

	return 0;
}


/**
 * ミューテックスを更新する。
 *
 * @param arg ミューテックス
 */
static void
mutex_update(void *arg)
{
	struct mutex *m = (struct mutex*)(arg);

	if ((m->owner_task_id == 0) && wait_object_has_wait_entries(&(m->wait_object))) {
		const struct task_entry *entry = wait_object_release_one(&(m->wait_object));
		if (entry != NULL) {
			m->owner_task_id = entry->param.id;
		}
	}

	return ;
}
