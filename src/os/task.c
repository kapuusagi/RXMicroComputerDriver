/**
 * @file タスクエントリ。
 *       タスクエントリの初期化と破棄処理の定義を記述する。
 * @author 
 */

#include "task.h"

/**
 * タスクデータを初期化する。
 *
 * @param entry タスクデータ
 */
void
task_init(struct task_entry *entry)
{
	entry->prev = NULL;
	entry->next = NULL;
	entry->wait_next = NULL;
    entry->param.id = 0;
    entry->param.state = TASK_STATE_DEAD;
    entry->param.priority = 0;
    entry->param.func = NULL;
    entry->param.arg = NULL;
    entry->param.tcb.usp = NULL;
    entry->stack = NULL;

    return ;
}

/**
 * タスクの破棄処理をする。
 *
 * @param entry データ
 */
void
task_destroy(struct task_entry *entry)
{
    entry->param.id = 0;
    entry->param.state = TASK_STATE_DEAD;

    return ;
}


/**
 * タスクエントリをセットアップする。
 *
 * @param entry タスクエントリ
 * @param id タスクID
 * @param priority プライオリティ
 * @param task_func タスクのエントリ関数
 * @param task_arg タスクのエントリ関数に渡すポインタ
 * @param initial_stack 初期スタックポインタ値
 */
void
task_setup(struct task_entry *entry, task_id_t id,
		uint8_t priority, task_func_t task_func, void *task_arg, void *initial_stack)
{
    entry->next = NULL;
    entry->param.id = id;
    entry->param.state = TASK_STATE_PENDING;
    entry->param.priority = priority;
    entry->param.func = task_func;
    entry->param.arg = task_arg;
    entry->param.tcb.usp = initial_stack;

    return ;
}

