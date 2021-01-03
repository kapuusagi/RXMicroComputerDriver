/**
 * @file 待機オブジェクトリスト
 *       カーネルで使用する待機オブジェクト列を表現するためのオブジェクト。
 *       全ての待機オブジェクトの状態を更新すると、カーネル処理に時間がかかりすぎるため、
 *       待機対象のオブジェクトだけ更新するため、待機オブジェクトリストとして用意した。
 * @author 
 */

#ifndef WAIT_OBJECT_LIST_H_
#define WAIT_OBJECT_LIST_H_

#include "wait_object.h"

struct wait_object_list {
	struct wait_object *head;
};


#ifdef __cplusplus
extern "C" {
#endif

void wait_object_list_init(struct wait_object_list *list);
void wait_object_list_destroy(struct wait_object_list *list);
int wait_object_list_contains(const struct wait_object_list *list, const struct wait_object *wait_obj);
void wait_object_list_add(struct wait_object_list *list, struct wait_object *wait_obj);
struct wait_object *wait_object_list_head(struct wait_object_list *list);
void wait_object_list_remove(struct wait_object_list *list, struct wait_object *wait_obj);

#ifdef __cplusplus
}
#endif

#endif /* WAIT_OBJECT_LIST_H_ */
