/**
 * @file タスクリスト
 *       アクティブタスクリストと待ちタスクリスト、空きタスクリストで使用する。
 * @author 
 */
#include "task_list.h"


/**
 * タスクリストを初期化する。
 *
 * @param list タスクリスト
 */
void
task_list_init(struct task_list *list)
{
	list->head = NULL;
	list->tail = NULL;
	return ;
}

/**
 * タスクリストを破棄する。
 *
 * @param list タスクリスト
 */
void
task_list_destroy(struct task_list *list)
{
	/* no action. */
	return ;
}

/**
 * タスクリスト末尾に追加する。
 *
 * @param list タスクリスト
 * @param entry エントリ
 */
void
task_list_add(struct task_list *list, struct task_entry *entry)
{
	if (list->head == NULL) {
		/* 1つもタスクが無い */
		list->head = entry;
		list->tail = entry;
		entry->prev = NULL;
		entry->next = NULL;
	} else {
		list->tail->next = entry;
		entry->prev = list->tail;
		list->tail = entry;
		entry->next = NULL;
	}

	return ;
}

/**
 * タスクリストの先頭を取り出す。
 *
 * @param list タスクリスト
 * @return リストの先頭が返る。タスクリストが空の場合にはNULLが返る。
 */
struct task_entry*
task_list_pop(struct task_list *list)
{
	struct task_entry* entry = list->head;
	if (entry != NULL) {
		task_list_remove(list, entry);
	}

	return entry;
}


/**
 * タスクリストの先頭を得る。
 * リストからの消去は行わない。
 *
 * @param list タスクリスト
 * @return 先頭のエントリが返る。
 */
struct task_entry*
task_list_head(struct task_list* list)
{
	return list->head;
}

/**
 * タスクリストからentryを削除する。
 *
 * @param list タスクリスト
 * @param entry エントリ
 */
void
task_list_remove(struct task_list *list, struct task_entry *entry)
{
	if (entry->prev == NULL) {
		list->head = entry->next;
	}
	if (entry->next == NULL) {
		list->tail = entry->prev;
	}
	if (entry->prev != NULL) {
		entry->prev->next = entry->next;
	}
	if (entry->next != NULL) {
		entry->next->prev = entry->prev;
	}
	entry->prev = NULL;
	entry->next = NULL;

	return ;
}


/**
 * タスクリストが空かどうかを得る。
 *
 * @param list タスクリスト
 * @return 空の場合には非ゼロの値、それ以外は0が返る。
 */
int
task_list_is_empty(struct task_list *list)
{
	return (list->head == NULL);
}
