/**
 * @file
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
