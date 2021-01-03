/**
 * @file 待機オブジェクトリスト
 * @author 
 */

#include "../rx_utils/rx_types.h"
#include "wait_object_list.h"


/**
 * 待機オブジェクトリストを初期化する。
 *
 * @param list 待機オブジェクトリスト
 */
void
wait_object_list_init(struct wait_object_list *list)
{
	list->head = NULL;

	return ;
}

/**
 * 待機オブジェクトリストを破棄する。
 *
 * @param list 待機オブジェクトリスト
 */
void
wait_object_list_destroy(struct wait_object_list *list)
{
	/* no action. */
	return ;
}

/**
 * 待機オブジェクトリストに含まれているかどうかを得る。
 *
 * @param list 待機オブジェクトリスト
 * @param wait_obj 待機オブジェクト
 * @return 含まれている場合には非ゼロの値、それ以外は0が返る。
 */
int
wait_object_list_contains(const struct wait_object_list *list, const struct wait_object *wait_obj)
{
	const struct wait_object *entry = list->head;
	while (entry != NULL) {
		if (entry == wait_obj) {
			return 1;
		}
		entry = entry->next;
	}
	return 0;
}

/**
 * 待機オブジェクトリストに追加する。
 *
 * @param list 待機オブジェクトリスト
 * @param wait_obj 待機オブジェクト
 */
void
wait_object_list_add(struct wait_object_list *list, struct wait_object *wait_obj)
{
	if (!wait_object_list_contains(list, wait_obj)) {
		/* リストに保持されていない */
		wait_obj->next = list->head;
		list->head = wait_obj;
	}

	return ;
}

/**
 * 待機オブジェクトリストの先頭を得る。
 *
 * @param list 待機オブジェクトリスト
 * @return 先頭のオブジェクト
 */
struct wait_object *
wait_object_list_head(struct wait_object_list *list)
{
	return list->head;
}

/**
 * 指定した待機オブジェクトを削除する。
 *
 * @param list 待機オブジェクトリスト
 * @param wait_obj 待機オブジェクト
 */
void
wait_object_list_remove(struct wait_object_list *list, struct wait_object *wait_obj)
{
	if (list->head == wait_obj) {
		list->head = wait_obj->next;
		wait_obj->next = NULL;
	} else {
		struct wait_object *prev = list->head;
		while ((prev != NULL) && (prev->next != wait_obj)) {
			prev = prev->next;
		}
		if (prev != NULL) {
			prev->next = wait_obj->next;
			wait_obj->next = NULL;
		}
	}
}
