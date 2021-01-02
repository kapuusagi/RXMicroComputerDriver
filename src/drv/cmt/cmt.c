/**
 * @file ハードウェアタイマーを扱うためのドライバ。
 *       所定の時間が経過したときに割り込みハンドラにより、
 *       登録した関数を呼び出す機能を提供する。
 */
#include <iodefine.h>
#include "../../rx_utils/rx_utils.h"
#include "../sysconfig.h"
#include "cmt.h"

/**
 * Nマイクロ秒待機をループで待つためのTICK調整
 *
 * 1usecあたりのCMT0カウンタ増加量は
 *   (PCLKB / 8) / 1000000 = 7.5
 *
 * 整数値に調整して
 *   1tick = 2[usec] = 15[count]
 */
#define US_DELAY_TICK  (2)
#define US_DELAY_TICK_COUNT (15)

/**
 * マイクロ秒ディレイを実現するためのカウンタ値算出マクロ
 * usec以上の時間経過時に、歩進するカウンタの値を得る。
 */
#define USEC_DELAY_COUNT(usec) \
    (((usec) * US_DELAY_TICK_COUNT + US_DELAY_TICK_COUNT - 1) / US_DELAY_TICK)


/**
 * 1msをカウントするためのカウンタ値
 *
 * CMT_TIMER_COUNT_1MS = (PCLKB / 8) / 1000 - 1
 *                     = 60000000 / 8 / 1000 -1
 *                     = 7499
 */
#define CMT_TIMER_COUNT_1MS   (7499)

/**
 * 基底タイマーのTICK は1ミリ秒
 */
#define TIMER_TICK (1)





struct timer_data {
    timer_handler_t handler; /* ハンドラ */
    uint32_t count; /* 現在のカウンタ値 */
    uint32_t max_count; /* カウンタ値の最大 */
};

/**
 * タイマーカウンタ。
 * 1ms毎にインクリメントする。
 *
 * 32bit符号なし整数で表現可能な値は 約1193時間となる。
 * 電源がONされている間（スリープ中は停止）のため、1193時間連続稼動しないかぎり
 * オーバーフローは発生しない。
 * 尚、オーバーフローが発生しても経過時間の算出(now - prev)で正常に算出できる。
 * もちろん、スリープ前の値と比較してはいけない。
 */
static volatile uint32_t TimerCounter = 0;

/**
 * タイマーデータ
 */
static struct timer_data TimerData[NUM_TIMERS];

/**
 * ミリ秒毎のハンドラ
 */
static msec_handler_t MilliSecondHandler = NULL;

static uint8_t is_valid_timer_exists(void);

static void timer_proc(struct timer_data *timer);
static void timer_set(struct timer_data *timer, uint32_t interval_msec,
        timer_handler_t handler);
static void timer_clear(struct timer_data *timer);


/**
 * タイマードライバモジュールを初期化する。
 */
void
drv_cmt_init(void)
{
    SYSTEM.PRCR.WORD = 0xA502;
    MSTP(CMT0) = 0; /* CMT0動作 */
    MSTP(CMT1) = 0; /* CMT1動作 */
    SYSTEM.PRCR.WORD = 0xA500;

    IPR(CMT0, CMI0) = RX_MAX_PRIORITY;
    IEN(CMT0, CMI0) = 1;
    IPR(CMT1, CMI1) = TIMER_INT_PRIORITY;
    IEN(CMT1, CMI1) = 1;

    rx_memset(&TimerData, 0x0, sizeof(TimerData));
    TimerCounter = 0;

    /* CMT1.CMCR
     *   b15-b8: 0固定
     *   b7: 1固定
     *   b6: 0=割り込み停止
     *   b5-b2: 0固定
     *   b1-b0: 0=PCLK/8でカウント
     */
    CMT0.CMCR.WORD = 0x0080;
    CMT1.CMCR.WORD = 0x0080;

    /* 周期カウンタ設定
     *
     */
    CMT0.CMCOR = CMT_TIMER_COUNT_1MS;
    CMT1.CMCOR = CMT_TIMER_COUNT_1MS;

    /*
     * タイマー0スタート
     */
    CMT0.CMCNT = 0; /* カウンタクリア */
    CMT0.CMCR.BIT.CMIE = 1; /* 割り込み許可 */
    CMT.CMSTR0.BIT.STR0 = 1; /* タイマカウント開始 */

    return ;
}

/**
 * コンペアマッチタイマドライバを停止する。
 */
void
drv_cmt_destroy(void)
{
    uint8_t no;

    CMT.CMSTR0.BIT.STR0 = 0;
    CMT.CMSTR0.BIT.STR1 = 0;

    /* 割り込みを停止し、割り込みフラグをクリアする。 */
    CMT0.CMCR.BIT.CMIE = 0; /* 割り込み禁止 */
    IR(CMT0, CMI0) = 0;
    CMT1.CMCR.BIT.CMIE = 0; /* 割り込み禁止 */
    IR(CMT1, CMI1) = 0;
    for (no = 0; no < NUM_TIMERS; no++) {
        timer_clear(&TimerData[no]);
    }

    SYSTEM.PRCR.WORD = 0xA502;
    MSTP(CMT1) = 1; /* CMT1停止 */
    MSTP(CMT0) = 1; /* CMT0停止 */
    SYSTEM.PRCR.WORD = 0xA500;

    return ;
}

/**
 * 1ミリ秒毎にコールされるハンドラを設定する。
 * このハンドラはカーネルによるタイムスライス用途を想定している。
 * 多重割り込み禁止の状態でコールされる。
 *
 * ハンドラの処理は十分に短い時間で完了することを想定する。
 *
 * @param handler ハンドラ
 */
void
drv_cmt_set_msec_handler(msec_handler_t handler)
{
    rx_util_disable_interrupt();
    MilliSecondHandler = handler;
    rx_util_enable_interrupt();
}


/**
 * タイマーを開始する。
 *
 * @param id タイマーID
 * @param interval_msec インターバル時間[ミリ秒]
 * @param handler 呼び出すハンドラ
 */
void
drv_cmt_start(uint8_t id, uint32_t interval_msec, timer_handler_t handler)
{
    switch (id) {
    case TIMER_1:
        timer_set(&TimerData[0], interval_msec, handler);
        break;
    case TIMER_2:
        timer_set(&TimerData[1], interval_msec, handler);
        break;
    }
    if (is_valid_timer_exists()) {
        CMT1.CMCNT = 0; /* カウンタクリア */
        CMT1.CMCR.BIT.CMIE = 1; /* 割り込み許可 */
        CMT.CMSTR0.BIT.STR1 = 1; /* タイマカウント開始 */
    }
    return ;
}

/**
 * タイマーを停止させる。
 *
 * @param id タイマー番号
 */
void
drv_cmt_stop(uint8_t id)
{
    switch (id) {
    case TIMER_1:
        /* タイマー停止 */
        timer_clear(&TimerData[0]);
        break;
    case TIMER_2:
        /* タイマー停止 */
        timer_clear(&TimerData[1]);
        break;
    }
    if (!is_valid_timer_exists()) {
        CMT.CMSTR0.BIT.STR1 = 0;
        CMT1.CMCR.BIT.CMIE = 0; /* 割り込み禁止 */
    }
    return;
}

/**
 * 指定ミリ秒以上待機する。
 * タイマーが動いていない場合には即座に返る。
 * CPUを占有しつづけるため、使用するときは注意すること。
 * 割り込みハンドラでは使用しないこと。ハングアップの原因になる。
 *
 * @param msec 待機時間（ミリ秒）
 */
void
drv_cmt_delay_ms(uint32_t msec)
{
    uint32_t enter_count;
    uint32_t elapse;

    if (CMT.CMSTR0.BIT.STR0 == 0) {
        return ; /* タイマー0が稼動していない。 */
    }

    enter_count = TimerCounter;
    do {
        elapse = TimerCounter - enter_count;
    } while (elapse < msec);

    return ;
}

/**
 * 指定マイクロ秒以上待機する。
 * タイマーが動いていない場合には即座に返る。
 * CPUを占有しつづけるため、使用するときは注意すること。
 * 特に割り込みハンドラでの使用は最大限注意する。
 *
 * @param usec マイクロ秒
 */
void
drv_cmt_delay_us(uint16_t usec)
{
    uint16_t enter_count;
    uint16_t now_count;
    uint16_t elapse;
    uint32_t wait_count;
    uint32_t ms;

    if (CMT.CMSTR0.BIT.STR0 != 0) {
        if (usec >= 1000) {
            ms = usec / 1000;
            drv_cmt_delay_ms(ms);
            usec -= (ms * 1000);
        }

        wait_count = USEC_DELAY_COUNT(usec);

        enter_count = CMT0.CMCNT;
        do {
            now_count = CMT0.CMCNT;
            if (now_count >= enter_count) {
                elapse = now_count - enter_count;
            } else {
                /* CMCNT is overflow. */
                elapse = (now_count + CMT_TIMER_COUNT_1MS) - enter_count;
            }
        } while (elapse < wait_count);
    }

    return ;
}

/**
 * CMTが持っている、1ms毎に歩進するカウンタの値を取得する。
 * このカウンタの値は32bit符号なし整数で、0xffffffffに到達すると0x0に戻る。
 * 何らかの処理の前後でこの値を取得し、差分をとることで経過時間を取得できる。
 * 取得する型はuint32_tを使用する事。
 *
 * 例）
 *     uint32_t start_cnt, now;
 *     uint32_t msec;
 *
 *     start_cnt = drv_cmt_get_counter();
 *
 *       : // 何らかの処理
 *
 *     now = drv_cmt_get_counter();
 *     msec = now - start_cnt; // ミリ秒
 *
 * @return カウンタの値
 */
uint32_t
drv_cmt_get_counter(void)
{
    return TimerCounter;

}

/**
 * 有効なタイマーデータが存在するかどうかを判定する。
 * @return 有効なタイマーが存在する場合には1、それ以外は0が返る。
 */
static uint8_t
is_valid_timer_exists(void)
{
    uint8_t no;

    for (no = 0; no < NUM_TIMERS; no++) {
        if (TimerData[no].handler != NULL) {
            return 1;
        }
    }

    return 0;
}

/*
 * Note:
 * vect.hで宣言されている、
 * #pragma interrupt (xxxxx(vect=VECT(CMT0, CMI0)))は
 * コメントアウトすること。
 */

/**
 * タイマー割り込みハンドラ
 */
#pragma interrupt (Excep_CMT0_CMI0(vect=VECT(CMT0, CMI0)))
void
Excep_CMT0_CMI0(void)
{
    CMT0.CMCR.BIT.CMIE = 0; /* CMI0割り込み停止 */
    IR(CMT0, CMI0) = 0; /* 割り込みフラグクリア */
    TimerCounter++;

    if (MilliSecondHandler) {
        MilliSecondHandler();
    }
    CMT0.CMCR.BIT.CMIE = 1; /* CMI0 割り込み許可 */
}

#pragma interrupt (Excep_CMT1_CMI1(vect=VECT(CMT1, CMI1)))
void
Excep_CMT1_CMI1(void)
{
    uint8_t no;
    uint8_t is_interrupt_enable;

    CMT1.CMCR.BIT.CMIE = 0; /* CMI1割り込み停止 */
    IR(CMT1, CMI1) = 0; /* 割り込みフラグクリア */

    is_interrupt_enable = rx_util_is_interrupt_enable();
    if (is_interrupt_enable == 0) {
        /* 割り込み許可 */
        rx_util_enable_interrupt();
    }

    for (no = 0; no < NUM_TIMERS; no++) {
        if (TimerData[no].handler != NULL) {
            timer_proc(&TimerData[no]);
        }
    }
    if (is_interrupt_enable == 0) {
        rx_util_disable_interrupt();
    }

    CMT1.CMCR.BIT.CMIE = 1; /* CMI1 割り込み許可 */
}

/**
 * タイマーの処理を行う。
 * カウンタを歩進させ、指定されたインターバル時間だけ経過したときにハンドラを呼び出す。
 * @param timer タイマーオブジェクト
 */
static void
timer_proc(struct timer_data *timer)
{
    timer->count++;
    if (timer->count >= timer->max_count) {
        /* 指定μ秒経過したのでハンドラを呼び出す。 */
        timer->handler(TIMER_TICK);
        timer->count = 0;
    }

    return ;
}

/**
 * タイマーを設定する。
 * @param timer タイマーオブジェクト
 * @param interval_msec 処理間隔（ミリ秒）
 * @param handler コールバックハンドラ
 * @param arg コールバックに渡す引数
 */
static void
timer_set(struct timer_data *timer, uint32_t interval_msec,
        timer_handler_t handler)
{
    timer->count = 0;
    timer->max_count = interval_msec;
    timer->handler = handler;
}

/**
 * タイマーをクリアする。
 * @param timer タイマーオブジェクト
 */
static void
timer_clear(struct timer_data *timer)
{
    timer->count = 0;
    timer->handler = 0;
    timer->max_count = 0;
}

