/**
 * @file タスク定義
 * @author 
 */

#ifndef TASK_H_
#define TASK_H_


#include "kernel_defs.h"
#include "systemcall_param.h"

#define TASK_STATE_DEAD    0
#define TASK_STATE_ACTIVE  1
#define TASK_STATE_PENDING 2
#define TASK_STATE_WAITING 3

struct rxcc_tcb {
    void *usp; /* ユーザースタックポインタ */
};

/**
 * タスクパラメータ
 */
struct task_param {
    uint16_t id; /* ID */
    uint8_t state; /* ステート */
    uint8_t priority; /* プライオリティ */
    struct rxcc_tcb tcb; /* タスクコンテキストブロック */
    task_func_t func; /* 実行する関数 */
    void *arg; /* 引数 */
    uint8_t syscall_type; /* システムコールタイプ */
    uint8_t rsvd[3];
    union system_call_param sysc; /* システムコール */
};

/**
 * タスクエントリ
 */
struct task_entry {
	struct task_entry *prev;
    struct task_entry *next;
    struct task_param param;
    uint32_t *stack;
};

#ifdef __cplusplus
extern "C" {
#endif

void task_init(struct task_entry *entry);
void task_destroy(struct task_entry *entry);

void task_setup(struct task_entry *entry, task_id_t id,
		uint8_t priority, task_func_t task_func, void *task_arg, void *initial_stack);

#ifdef __cplusplus
}
#endif


#endif /* TASK_H_ */
