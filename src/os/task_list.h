/**
 * @file タスクリスト
 * @author 
 */

#ifndef TASK_LIST_H_
#define TASK_LIST_H_

#include "task.h"

struct task_list {
	struct task_entry *head;
	struct task_entry *tail;
};


#ifdef __cplusplus
extern "C" {
#endif

void task_list_init(struct task_list *list);
void task_list_destroy(struct task_list *list);

void task_list_add(struct task_list *list, struct task_entry *entry);
struct task_entry* task_list_pop(struct task_list *list);
struct task_entry* task_list_head(struct task_list* list);
void task_list_remove(struct task_list *list, struct task_entry *entry);
int task_list_is_empty(struct task_list *list);




#ifdef __cplusplus
}
#endif


#endif /* OS_TASK_LIST_H_ */
