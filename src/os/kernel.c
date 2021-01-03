/**
 * @file カーネルCソースファイル
 * @author
 */

#include <iodefine.h>
#include "../rx_utils/rx_utils.h"
#include "../rx_utils/error_code.h"
#include "../drv/cmt/cmt.h"
#include "task.h"
#include "task_list.h"
#include "kernel.h"

static void* init_stack(void *stack, void *func, void *arg);
static struct task_entry *next_task(void);
static void update_waiting_tasks(void);
static int update_waiting_condition(struct task_entry *entry);
static void task_entry_proc(struct task_param *param);
static int is_all_task_waiting(void);


/**
 * スケジューラが動作中かどうか。
 */
static volatile uint8_t IsSchedulerRuning = 0;
/**
 * 戻りコンテキストブロック
 */
static struct rxcc_tcb ReturnTcb;

/**
 * コンテキストブロック
 * コンテキストブロックはソフトウェア割り込みにてスイッチする。
 */
struct rxcc_tcb *CurrentTcb;

/**
 * カレントタスク
 */
static struct task_entry *CurrentTask;

/**
 * タスク開始時のPSWレジスタ初期値
 * b20: 1 ユーザーモードで実行
 * b17: 1 USP
 * b16: 1 割り込み許可
 */
#define INITIAL_PSW 0x00130000
/**
 * タスク開始時のFPSWレジスタ初期値
 */
#define INITIAL_FPSW 0x00000100



/**
 * 空きタスク
 */
static struct task_list BlankEntries;

/**
 * 待機エントリ
 */
static struct task_list WaitingEntries;

/**
 * アクティブエントリ
 */
static struct task_list ActiveEntries;

/**
 * タスクデータ
 */
static struct task_entry TaskEntries[MAX_TASKS];


/**
 * タスクIDジェネレータ
 */
static uint16_t TaskIdGen = 0;

/**
 * カーネルを初期化する。
 */
void
kernel_init(void)
{
    rx_memset(&ReturnTcb, 0x0, sizeof(ReturnTcb));
    CurrentTcb = NULL;

    task_list_init(&ActiveEntries);
    task_list_init(&BlankEntries);
    for (int i = 0; i < MAX_TASKS; i++) {
    	struct task_entry *entry = &(TaskEntries[i]);
        task_init(entry);
        task_list_add(&BlankEntries, entry);
    }

    IPR(ICU, SWINT) = KERNEL_PRIORITY;
    IEN(ICU, SWINT) = 1;
    return ;
}


/**
 * タスクを登録する。
 *
 * @param priority プライオリティ
 * @param func 関数
 * @param arg funcに渡す引数
 * @param stack スタック
 * @param stack_size スタックサイズ。
 * @return 成功した場合、タスクIDが返る。失敗した場合、リソースがなくて登録に失敗した場合、-1が返る。
 */
int
kernel_register_task(uint16_t priority,
		task_func_t func, void *arg,
		stack_type_t *stack, uint32_t stack_size)
{
    if ((func == NULL) || (stack == NULL) || (stack_size == 0)) {
        return -1;
    }

    struct task_entry *entry = task_list_pop(&BlankEntries);
    if (entry == NULL) {
    	/* 空きなし */
        return -1;
    }

    uint8_t id = TaskIdGen++;
    void* usp = init_stack(stack + stack_size / sizeof(stack_type_t),
            task_entry_proc, &(entry->param));
    task_setup(entry, id, priority, func, arg, usp);

    task_list_add(&ActiveEntries, entry);

    return (int)(entry->param.id);
}


/**
 * タスクのスタックを初期化する。
 *
 * @param stack タスク
 * @param func タスクエントリ関数
 * @param arg タスクエントリ関数に渡す引数。
 * @return TCBポインタ。
 */
static void*
init_stack(void *stack, void *func, void *arg)
{
    uint32_t *p = (uint32_t*)(stack);

    /* CC for RX でのINT割り込みが発生した時、退避されるレジスタをエミュレートする */
    *(--p) = 0x0;

    *(--p) = INITIAL_PSW; /* PSW */

    *(--p) = (uint32_t)(func); /* PC */

    *(--p) = 15; /* R15 */
    *(--p) = 14; /* R14 */
    *(--p) = 13; /* R13 */
    *(--p) = 12; /* R12 */
    *(--p) = 11; /* R11 */
    *(--p) = 10; /* R10 */
    *(--p) = 9; /* R9 */
    *(--p) = 8; /* R8 */
    *(--p) = 7; /* R7 */
    *(--p) = 6; /* R6 */
    *(--p) = 5; /* R5 */
    *(--p) = 4; /* R4 */
    *(--p) = 3; /* R3 */
    *(--p) = 2; /* R2 */
    *(--p) = (uint32_t)(arg); /* R1 */

    *(--p) = INITIAL_FPSW; /* INITIAL_FPSW */

    return p;
}

/**
 * スケジューラを開始する。
 * 既にスケジューラが動作中の場合には何もしない。
 */
void
kernel_start_scheduler(void)
{
    IsSchedulerRuning = 1;

    task_list_init(&WaitingEntries);

    /* 戻ってこれるように、現在のTCBを保存するようにする。 */
    CurrentTcb = &ReturnTcb;

    /* 現在のタスクをなしとする */
    CurrentTask = NULL;

	/* ソフトウェア割り込み発行してコンテキストスイッチする */
	ICU.SWINTR.BIT.SWINT = 1;

    /* ここに制御が返るのは、全てのスレッドが完了している場合のみ(たぶん) */
    IsSchedulerRuning = 0;
}



/**
 * スケジューラを更新する。
 */
void
kernel_update_scheduler(void)
{
	if (CurrentTask != NULL) {
		if (CurrentTask->param.state == TASK_STATE_DEAD) {
			task_list_add(&BlankEntries, CurrentTask);
		} else if (CurrentTask->param.state == TASK_STATE_WAITING) {
			task_list_add(&WaitingEntries, CurrentTask);
		} else {
			CurrentTask->param.state = TASK_STATE_PENDING;
			task_list_add(&ActiveEntries, CurrentTask);
		}
		CurrentTask = NULL;
	}

	/* Waitingステートのタスクを更新し、待機完了したタスクをスケジューラに回す */
	update_waiting_tasks();

	while (1) {
	    /* 次に実行するタスクを取得する */
	    CurrentTask = next_task();
	    if (CurrentTask != NULL) {
	    	break;
	    } else if (task_list_is_empty(&WaitingEntries)) {
	    	/* タスクが全てなくなった */
	    	break;
	    } else {
	    	/* WaitingEntriesの待機解除されるまで回す */
	    	rx_util_enable_interrupt();
	    	rx_util_set_ipl(0);
	    	update_waiting_tasks();
	    	rx_util_set_ipl(KERNEL_PRIORITY);
	    	rx_util_disable_interrupt();
	    }
	}
    if (CurrentTask != NULL) {
    	CurrentTask->param.state = TASK_STATE_ACTIVE;
        CurrentTcb = &(CurrentTask->param.tcb);
    } else {
        CurrentTcb = &(ReturnTcb);
    }
}

/**
 * 次に実行するべきタスクを得る。
 *
 * @return 次に実行するべきタスク。
 */
static struct task_entry *
next_task(void)
{
	/* ActiveTasksから、最もプライオリティが高いやつを引っ張り出して返す */
	struct task_entry* entry = task_list_head(&ActiveEntries);
	if (entry == NULL) {
		return NULL;
	} else {
		struct task_entry *ret_entry = entry;
		while (entry != NULL) {
			if (entry->param.priority > ret_entry->param.priority) {
				ret_entry = entry;
			}
			entry = entry->next;
		}

		task_list_remove(&ActiveEntries, ret_entry);

		return ret_entry;
	}
}

/**
 * 待機タスクの状態を更新する。
 */
static void
update_waiting_tasks(void)
{
	/* 待機条件を更新してアクティブ列に渡す */
	struct task_entry *entry = task_list_head(&WaitingEntries);
	while (entry != NULL) {
		struct task_entry *next = entry->next;
		if (update_waiting_condition(entry)) {
			/* 待機完了したので、アクティブ列に戻す */
			task_list_remove(&WaitingEntries, entry);
			entry->param.state = TASK_STATE_PENDING;
			task_list_add(&ActiveEntries, entry);
		}
		entry = next;
	}
}

/**
 * 待機条件を更新する。
 *
 * @param entry タスク
 * @return 待機完了した場合には非ゼロの値を返す。それ以外は0を返す。
 */
static int
update_waiting_condition(struct task_entry *entry)
{
	/* ここでウェイト条件を調べて返す */
	union system_call_param *sysc = &(entry->param.sysc);

	switch (entry->param.syscall_type) {
	case SYSCALL_WAIT_MSEC:
		{
			uint32_t now = drv_cmt_get_counter();
			return ((now - sysc->wait.begin) >= sysc->wait.wait_millis);
		}
	default:
		return 1;
	}
	return 1;
}


/**
 * タスクエントリ関数。
 * タスク終了時、スケジューラを呼び出すためにラップする。
 *
 * @param param タスクパラメータ
 */
static void
task_entry_proc(struct task_param *param)
{
    if (param->func != NULL) {
        param->func(param->arg);
    }
    param->state = TASK_STATE_DEAD;

    kernel_request_swtich();
}

/**
 * タスクの切り替え要求を出す。
 * タイマー割り込みなどから呼び出すことを前提にしている。
 */
void
kernel_request_swtich(void)
{
	if (!is_all_task_waiting()) {
		/* アクティブなタスクが存在する時だけ割り込みを入れて切り替える。
		 * これをやらないと、待機タスク解除待ちをしている時に通知され、
		 * スタックオーバーフローする。 */
        ICU.SWINTR.BIT.SWINT = 1;
	}
}

/**
 * 全てのタスクが待機状態かどうかを取得する。
 * スケジューラが動いていないか、全てのタスクが待機状態になっているとき、この関数は非ゼロを返す。
 *
 * @return 全てのタスクが待機状態の場合には非ゼロの値, それ以外は0
 */
static int
is_all_task_waiting(void)
{
	return (CurrentTask == NULL) && task_list_is_empty(&ActiveEntries);
}


/**
 * 現在実行中のタスクに待機要求を出すする。
 *
 * @param wait_millis 待機時間[ミリ秒]
 */
void
kernel_sysc_wait(uint32_t wait_millis)
{
	struct task_entry *entry = CurrentTask;

	entry->param.syscall_type = SYSCALL_WAIT_MSEC;
	entry->param.sysc.wait.begin = drv_cmt_get_counter();
	entry->param.sysc.wait.wait_millis = wait_millis;
	entry->param.state = TASK_STATE_WAITING;
	kernel_request_swtich();
}

