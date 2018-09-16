/**
 * @file ハードウェアタイマーを扱うためのドライバ。
 *       所定の時間が経過したときに割り込みハンドラにより、
 *       登録した関数を呼び出す機能を提供する。
 */
#include <iodefine.h>
#include "../../rx_utils/rx_utils.h"
#include "../sysconfig.h"
#include "cmt.h"


typedef struct timer_data {
	timer_handler_t handler; // ハンドラ
	void *arg; // ハンドラに渡す引数
	uint32_t elapse; // 経過時間
	uint32_t interval_millis; // インターバル間隔
} timer_data_t;

#define TIMER_COUNT 1

static timer_data_t TimerData[TIMER_COUNT];

static void timer_init(timer_data_t *ptimer);
static void timer_set(timer_data_t *ptimer,
		uint32_t interval_millis, timer_handler_t handle, void *arg);
static void timer_unset(timer_data_t *ptimer);
static void timer_proc(timer_data_t *ptimer);


/**
 * 1MSあたりをカウントするためのカウンタ値。
 * プロセッサのクロックなどが変更になった場合、これも変更する必要がある。
 *
 * PCLKB / 8 / 1000 = 60000000 / 8 / 1000 = 7500
 */
#define TIMER_CMCOR_1MS (7500)

/**
 * タイマードライバモジュールを初期化する。
 */
void
drv_cmt_init(void)
{
	int i;
	for (i = 0; i < TIMER_COUNT; i++) {
		timer_init(&TimerData[i]);
	}
	SYSTEM.PRCR.WORD = 0xA502;
	MSTP(CMT0) = 0;
	MSTP(CMT1) = 0;
	SYSTEM.PRCR.WORD = 0xA500;

	IPR(CMT0, CMI0) = TIMER_INT_PRIORITY;
	IEN(CMT0, CMI0) = 1;

	CMT0.CMCOR = TIMER_CMCOR_1MS;
	CMT0.CMCR.BIT.CKS = 0; // PLL/8
	CMT0.CMCNT = 0;
	CMT.CMSTR0.BIT.STR0 = 0;

	return ;
}

/**
 * タイマーをスタートさせる。
 *
 * @param timer_no タイマー番号
 * @param interval_msec インターバル間隔（ミリ秒）
 * @param handler タイマーハンドラ
 * @param arg タイマーハンドラに渡す引数
 */
void
drv_cmt_start(uint8_t timer_no, uint32_t interval_msec,
		timer_handler_t handler, void *arg)
{
	switch (timer_no) {
	case TIMER_NO_1:
		timer_set(&TimerData[0], interval_msec, handler, arg);
		CMT0.CMCR.BIT.CMIE = 1; // 割り込み有効
		CMT0.CMCNT = 0; // カウンタクリア
		CMT.CMSTR0.BIT.STR0 = 1; // タイマー開始
		break;
	}
}

/**
 * タイマーを停止させる。
 *
 * @param timer_no タイマー番号
 */
void
drv_cmt_stop(uint8_t timer_no)
{
	switch (timer_no) {
	case TIMER_NO_1:
		CMT0.CMCR.BIT.CMIE = 0; // 割り込み停止
		CMT.CMSTR0.BIT.STR0 = 0; // タイマー停止
		timer_unset(&TimerData[0]);
		break;
	}
}

/*
 * Note:
 * vect.hで宣言されている、
 * #pragma interrupt (xxxxx(vect=VECT(CMT0, CMI0)))は
 * コメントアウトすること。
 */
#pragma interrupt (cmt0_cmi0_isr(vect=VECT(CMT0, CMI0)))

/**
 * タイマー割り込みハンドラ
 */
static void
cmt0_cmi0_isr(void)
{
	CMT0.CMCR.BIT.CMIE = 0; // 割り込み停止
	IR(CMT0, CMI0) = 0;
	timer_proc(&TimerData[0]);
	CMT0.CMCR.BIT.CMIE = 1; // 割り込み有効
}

/**
 * タイマーデータを初期化する。
 * @param ptimer タイマーオブジェクト
 */
static void
timer_init(timer_data_t *ptimer)
{
	ptimer->handler = 0;
	ptimer->arg = 0;
	ptimer->interval_millis = 0;
	ptimer->elapse = 0;
}

/**
 * タイマーを設定する。
 *
 * @param ptimer タイマーオブジェクト
 * @param interval_millis インターバル間隔(ミリ秒)
 * @param handler タイマーハンドラ
 * @param arg タイマーハンドラに渡す引数
 */
static void
timer_set(timer_data_t *ptimer,
		uint32_t interval_millis, timer_handler_t handler, void *arg)
{
	ptimer->handler = handler;
	ptimer->arg = arg;
	ptimer->interval_millis = interval_millis;
	ptimer->elapse = 0;

}

/**
 * タイマーを解除する。
 *
 * @param ptimer タイマーオブジェクト
 */
static void
timer_unset(timer_data_t *ptimer)
{
	ptimer->handler = 0;
	ptimer->arg = 0;
	ptimer->interval_millis = 0;
	ptimer->elapse = 0;

	return ;
}

/**
 * タイマーのコールバック処理を行う。
 * @param ptimer タイマーオブジェクト
 */
static void
timer_proc(timer_data_t *ptimer)
{
	ptimer->elapse++;
	if (ptimer->elapse >= ptimer->interval_millis) {
		ptimer->handler(ptimer->arg);
		ptimer->elapse = 0;
	}
	return ;
}


