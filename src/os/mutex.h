/**
 * @file ミューテックス
 * @author 
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include "wait_object.h"

struct mutex {
	struct wait_object wait_object;
	int owner_task_id;
};


#ifdef __cplusplus
extern "C" {
#endif

void mutex_init(struct mutex *m);
void mutex_destroy(struct mutex *m);

int mutex_lock(struct mutex *m);
int mutex_unlock(struct mutex *m);

int mutex_trylock(struct mutex *m);


#ifdef __cplusplus
}
#endif


#endif /* OS_MUTEX_H_ */
