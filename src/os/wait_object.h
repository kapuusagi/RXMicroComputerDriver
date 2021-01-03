/**
 * @file 待機オブジェクト
 *       オブジェクトに対する「待ち」を提供するための基本機能を提供する。
 * @author 
 */

#ifndef WAIT_OBJECT_H_
#define WAIT_OBJECT_H_

struct task_entry;

struct wait_object {
	struct wait_object *next;
	struct task_entry *wait_entries;
	void (*update)(void *arg);
	void *arg;
};


#ifdef __cplusplus
extern "C" {
#endif

void wait_object_init(struct wait_object *obj, void (*update)(), void *arg);
void wait_object_destroy(struct wait_object *obj);
struct task_entry *wait_object_release_one(struct wait_object *obj);
void wait_object_add(struct wait_object *obj, struct task_entry *entry);
int wait_object_has_wait_entries(struct wait_object *obj);
void wait_object_update(struct wait_object *obj);
int wait_object_is_waiting_entry(const struct wait_object *obj, const struct task_entry *entry);

#ifdef __cplusplus
}
#endif


#endif /* WAIT_OBJECT_H_ */
