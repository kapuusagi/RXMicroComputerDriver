/**
 * @file セマフォ
 * @author 
 */

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include "wait_object.h"

struct semaphore {
	struct wait_object wait_obj;
	uint16_t count;
};

void sem_init(struct semaphore *sem, uint16_t initial_count);
void sem_destroy(struct semaphore *sem);
int sem_wait(struct semaphore *sem);
void sem_post(struct semaphore *sem);


#endif /* OS_SEMAPHORE_H_ */
