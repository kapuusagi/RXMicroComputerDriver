/**
 * @file 待機オブジェクト
 * @author 
 */
#include "../rx_utils/rx_types.h"
#include "task.h"
#include "wait_object.h"

static struct task_entry* get_tail(struct task_entry *wait_entry);
static void update_default(struct wait_object *obj);

/**
 * 待機オブジェクトを初期化する。
 *
 * @param obj 待機オブジェクト
 * @param update 更新関数
 * @param arg 更新関数に渡すパラメータ
 */
void
wait_object_init(struct wait_object *obj, void (*update)(), void *arg)
{
	obj->next = NULL;
	obj->wait_entries = NULL;
	obj->update = update;
	obj->arg = arg;
	return ;
}

/**
 * 待機オブジェクトを破棄する。
 *
 * いないと思うけど、このオブジェクトを待機しているタスクがいたらバグ。
 * 呼び出し元でdestroyする前に確認すること。
 *
 * @param obj 待機オブジェクト
 */
void
wait_object_destroy(struct wait_object *obj)
{
	obj->next = NULL;
	obj->wait_entries = NULL;
	obj->update = NULL;
	obj->arg = NULL;
	return ;
}

/**
 * 待機中のタスクを1つリリースする。
 *
 * @param obj 待機オブジェクト
 * @return リリースしたタスクのインスタンス。リリースタスクが無い場合にはNULL.
 */
struct task_entry *
wait_object_release_one(struct wait_object *obj)
{
	struct task_entry *entry = obj->wait_entries;
	if (entry != NULL) {
		obj->wait_entries = entry->wait_next;
		entry->wait_next = NULL;
		if (entry->param.sysc.wait_object == obj) {
			entry->param.sysc.wait_object = NULL;
		}
	}

	return entry;
}

/**
 * プライオリティは考慮されない。FIFOで順番に取り出すことを想定する。
 *
 * @param obj 待機オブジェクト
 * @param entry エントリ
 */
void
wait_object_add(struct wait_object *obj, struct task_entry *entry)
{
	if (obj->wait_entries != NULL) {
		struct task_entry *tail = get_tail(obj->wait_entries);
		tail->wait_next = entry;
	} else {
		obj->wait_entries = entry;
	}
	entry->wait_next = NULL;

	return ;
}

/**
 * 末尾を得る。
 *
 * @param entry 先頭エントリ
 * @return 末尾エントリ
 */
static struct task_entry*
get_tail(struct task_entry *wait_entry)
{
	while (wait_entry->wait_next != NULL) {
		wait_entry = wait_entry->wait_next;
	}

	return wait_entry;
}

/**
 * 待機オブジェクトに対して、待機しているタスクが存在するかどうかを得る。
 *
 * @param obj オブジェクト
 * @return 待機タスクが存在する場合には非ゼロの値、それ以外は0が返る。
 */
int
wait_object_has_wait_entries(struct wait_object *obj)
{
	return (obj->wait_entries != NULL);
}

/**
 * デフォルトの更新処理をする。
 * デフォルトの実装では、この待機オブジェクトに対して待っている全てのタスクをペンディング状態に戻す。
 *
 * @param wait_obj 待機オブジェクト
 */
static void
update_default(struct wait_object *obj)
{
	while (obj->wait_entries != NULL) {
		struct task_entry *wait_entry = obj->wait_entries;
		obj->wait_entries = wait_entry->wait_next;
		wait_entry->wait_next = NULL;
	}

	return ;
}

/**
 * 待機オブジェクトを更新する。
 *
 * @param obj 待機オブジェクト
 */
void
wait_object_update(struct wait_object *obj)
{
	if (obj->update != NULL) {
		obj->update(obj->arg);
	} else {
		update_default(obj);
	}
}

/**
 * entryで指定されるタスクが、objを待っているかどうかを得る。
 *
 * @param obj 待機オブジェクト
 * @param entry タスク
 * @return 待っている場合には非ゼロの値を返す。それ以外は0を返す。
 */
int
wait_object_is_waiting_entry(const struct wait_object *obj, const struct task_entry *entry)
{
	const struct task_entry *wait_entry = obj->wait_entries;
	while (wait_entry != NULL) {
		if (wait_entry == entry) {
			return 1;
		}
		wait_entry = wait_entry->wait_next;
	}
	return 0;
}
