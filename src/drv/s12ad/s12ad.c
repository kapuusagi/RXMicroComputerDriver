/**
 * @file 12bit A/Dコンバータ用
 *
 * 現状ソフトウェアトリガしか使っていない。
 * 専用の割り込みベクタがあったら完了通知を受け取るようにした方がきれいになる。
 *
 * 12bit A/Dコンバータを動作させ、update()が呼び出されるたびに
 * A/D変換開始やリスタート処理を行うようになっている。
 *
 * また本ドライバにはフィルタ機能があり、
 * 最低値と最大値を除いた値で平均をとる仕組みになっている。
 */
#include <iodefine.h>
#include "s12ad.h"
#include "../../rx_utils/rx_utils.h"

struct s12adch_data {
    uint16_t data[4]; /* 読み出したデータ */
    uint16_t average; /* 平均 */
    uint8_t data_count; /* データ数 */
};

#define AD_FILTER_COUNT (4)

/* RX65N EnvisionKitでの A/Dチャンネル割り当ては以下のようになっている。
 *   AN000 チャンネル1
 *   AN001 チャンネル2
 *   AN002 チャンネル3
 *   AN003 チャンネル4
 *   AN004 ポートを他の機能に割り当てて使用しているため、利用不可
 *     :
 *   AN007 ポートを他の機能に割り当てて使用しているため、利用不可
 *   AN100 ポートを他の機能に割り当てて使用しているため、利用不可
 *     :
 *   AN113 ポートを他の機能に割り当てて使用しているため、利用不可
 *   AN114 チャンネル5
 *   AN115 ポートを他の機能に割り当てて使用しているため、利用不可
 *   AN116 チャンネル6
 *   AN117 ポートを他の機能に割り当てて使用しているため、利用不可
 *     :
 *   AN120 ポートを他の機能に割り当てて使用しているため、利用不可
 */

#define S12AD_CH_1 (0) /* アナログチャンネル1 */
#define S12AD_CH_2 (1) /* アナログチャンネル2 */
#define S12AD_CH_3 (2) /* アナログチャンネル3 */
#define S12AD_CH_4 (3) /* アナログチャンネル4 */
#define S12AD_CH_COUNT (4)

#define S12AD1_CH_5 (0) /* アナログチャンネル5 */
#define S12AD1_CH_6 (1) /* アナログチャンネル6 */
#define S12AD1_CH_COUNT  (2)

/**
 * AN000～AN007 サンプリングステートカウント
 */
#define S12AD_SAMPLING_STATE_COUNT  10

/**
 * AN100～AN120 サンプリングステートカウント
 */
#define S12AD1_SAMPLING_STATE_COUNT 10

/**
 * MCU温度センサ サンプリングステートカウント
 * RX-65N温度センサ特性より、4.15usec以上必要。
 * 4.15 < サンプリングステートカウント * 1 / PCLKD * 1E6
 *      < サンプリングステートカウント * 1 / (15 * 1E6) * 1E6
 * より
 * サンプリングステートカウント > 4.15 * 15 = 62.25
 */
#define S12AD1_TEMP_SAMPLING_STATE_COUNT 64
/**
 * 内部基準電圧サンプリングステートカウント
 */
#define S12AD1_INTERNAL_BASE_SAMPLING_STATE_COUNT 10

/**
 * 12bit A/Dコンバータのデータ
 */
struct s12ad_data {
    struct s12adch_data chdata[S12AD_CH_COUNT];
    uint16_t diag_data; /* 自己診断結果データ */
    uint8_t is_busy:1; /* Busy フラグ。start_xxx, update()で更新する */
    uint8_t rsvd1:7;
    uint8_t rsvd2;
};
static struct s12ad_data S12ADData;

struct s12ad1_data {
    struct s12adch_data chdata[S12AD1_CH_COUNT];
    uint16_t temperature; /* 温度センサデータ */
    uint16_t internal_base; /* 内部基準電圧 */
    uint16_t diag_data; /* 自己診断結果データ */
    uint8_t is_busy:1; /* Busy フラグ。start_xxx, update()で更新する */
    uint8_t update_temp:1; /* 温度更新要求 */
    uint8_t update_internal_base:1; /* 内部基準電圧更新要求 */
    uint8_t rsvd:5;
};
static struct s12ad1_data S12AD1Data;

static void s12ad_init(void);
static void s12ad_start_normal(void);
static void s12ad_start_diag(uint8_t diag_base);
static int s12ad_is_busy(void);
static void s12ad_stop(void);
static void s12ad_destroy(void);
static void s12ad_update(void);

static void s12ad1_init(void);
static void s12ad1_start_normal(void);
static void s12ad1_start_diag(uint8_t diag_base);
static int s12ad1_is_busy(void);
static void s12ad1_stop(void);
static void s12ad1_destroy(void);
static void s12ad1_update(void);

static void s12adch_data_init(struct s12adch_data *ch_data);
static void s12adch_data_destroy(struct s12adch_data *ch_data);
static void s12adch_data_update(struct s12adch_data *ch_data, uint16_t new_data);
static uint16_t s12adch_calc_average(const struct s12adch_data *ch_data);


/**
 * 12bit A/Dコンバータを初期化する。
 */
void
drv_s12ad_init(void)
{
    s12ad_init();
    s12ad1_init();

    return;
}

/**
 * モジュールを破棄する。
 */
void
drv_s12ad_destroy(void)
{
    s12ad_stop();
    s12ad1_stop();
    s12ad_destroy();
    s12ad1_destroy();

    return;
}


/**
 * S12ADに温度の更新を要求する。
 */
void
drv_s12ad_request_update_temp(void)
{
    S12AD1Data.update_temp = 1;
}

/**
 * S12ADに内部基準電圧の更新を要求する。
 */
void
drv_s12ad_request_update_internal_base(void)
{
    S12AD1Data.update_internal_base = 1;
}

/**
 * A/D変換を開始する。
 */
void
drv_s12ad_start_normal(void)
{
    if (!drv_s12ad_is_busy()) {
        s12ad_start_normal();
        s12ad1_start_normal();
    }

    return;
}

/**
 * 自己診断を開始する。
 *
 * @param ad_diag 基準レベル(AD_DIAG_xを指定する)
 */
void
drv_s12ad_start_diag(uint8_t ad_diag)
{
    if (!drv_s12ad_is_busy()) {
        s12ad_start_diag(ad_diag);
        s12ad1_start_diag(ad_diag);
    }

    return;
}

/**
 * A/D変換中かどうかを取得する。
 *
 * @return A/D変換中の場合には非ゼロの値が返る。
 */
int
drv_s12ad_is_busy(void)
{
    return s12ad_is_busy() || s12ad1_is_busy();
}


/**
 * A/Dコンバータを更新する。
 */
void
drv_s12ad_update(void)
{
    s12ad_update();
    s12ad1_update();

    return ;
}


/**
 * 12bit A/Dコンバータのデータを取得する。
 * 返す値はフィルタ済みのデータになる。
 *
 * @param ch チャンネル
 * @return A/Dコンバータのデータが返る。
 *             chが不正であったり、十分なデータが存在しない場合には0が返る。
 */
uint16_t
drv_s12ad_get_data(uint8_t ch)
{
    uint16_t retval;

    switch (ch) {
    case AD_CHANNEL_1:
    	retval = S12ADData.chdata[S12AD_CH_1].average;
    	break;
    case AD_CHANNEL_2:
    	retval = S12ADData.chdata[S12AD_CH_2].average;
    	break;
    case AD_CHANNEL_3:
    	retval = S12ADData.chdata[S12AD_CH_3].average;
    	break;
    case AD_CHANNEL_4:
    	retval = S12ADData.chdata[S12AD_CH_4].average;
    	break;
    case AD_CHANNEL_5:
    	retval = S12AD1Data.chdata[S12AD1_CH_5].average;
    	break;
    case AD_CHANNEL_6:
    	retval = S12AD1Data.chdata[S12AD1_CH_6].average;
    	break;
	default:
		retval = 0;
		break;
    }

    return retval;
}



/**
 * S12ADモジュールを初期化する。
 */
static void
s12ad_init(void)
{
    int i;

    for (i = 0; i < S12AD_CH_COUNT; i++) {
        s12adch_data_init(&S12ADData.chdata[i]);
    }
    S12ADData.is_busy = 0;
    S12ADData.diag_data = 0;

    SYSTEM.PRCR.WORD = 0xA502;
    MSTP(S12AD) = 0; /* S12AD 動作 */
    SYSTEM.PRCR.WORD = 0xA500;

    RX_UTIL_SET_INPUT_FUNC_PORT(PORT4, B0); /* AN000 AD CH1 */
    RX_UTIL_SET_INPUT_FUNC_PORT(PORT4, B1); /* AN001 AD CH2 */
    RX_UTIL_SET_INPUT_FUNC_PORT(PORT4, B2); /* AN002 AD CH3 */
    RX_UTIL_SET_INPUT_FUNC_PORT(PORT4, B3); /* AN003 AD CH4 */

    MPC.PWPR.BIT.B0WI = 0;
    MPC.PWPR.BIT.PFSWE = 1;

    /* P4nPFS
     * b7: 1 アナログ端子として使用する。
     * b6: 0 IRQn入力端子として使用しない。
     * b5-b0: 0 (reserved)
     */
    MPC.P40PFS.BYTE = 0x80;
    MPC.P41PFS.BYTE = 0x80;
    MPC.P42PFS.BYTE = 0x80;
    MPC.P43PFS.BYTE = 0x80;

    MPC.PWPR.BIT.PFSWE = 0;
    MPC.PWPR.BIT.B0WI = 1;

    /* ADCSR
     *   b15: ADST=0 A/D変換停止
     *   b14-b13: ADCS=0 シングルスキャンモード
     *   b12: ADIE=0 スキャン終了時のS12ADIO割り込み発生の禁止
     *   b11-b10: 0 (Reserved)
     *   b9: TRGE=0 同期、非同期トリガによるA/D変換の開始を禁止。
     *   b8: 0 同期トリガによるA/D変換の開始を選択
     *   b7: DBLE=0 ダブルトリガモード非選択
     *   b6: GBADIE=0 グループBnoスキャン終了後に割り込み発生を禁止。
     *   b5: 0 (Reserved)
     *   b4-b0: DBLANS=0
     */
    S12AD.ADCSR.WORD = 0x0000;

    /* ADANSA0
     *   b15-b8: 0(reserved)
     *   b7: 0 AN007 A/D変換対象外
     *   b6: 0 AN006 A/D変換対象外
     *   b5: 0 AN005 A/D変換対象外
     *   b4: 0 AN004 A/D変換対象
     *   b3: 1 AN003 A/D変換対象
     *   b2: 1 AN002 A/D変換対象
     *   b1: 1 AN001 A/D変換対象
     *   b0: 1 AN000 A/D 変換対象
     */
    S12AD.ADANSA0.WORD = 0x000f;

    /* ADADS0
     *   b15-b8: 0 (reserved)
     *   b7-b0: 0 AN000～AN007のA/D変換値加算/平均モード非選択
     */
    S12AD.ADADS0.WORD = 0x0000;

    /* ADADC
     *   b7: 0 加算モード選択
     *   b6-b3: 0 (Reserved)
     *   b2-b0: 0 1回変換
     */
    S12AD.ADADC.BYTE = 0x00;

    /* ADCER
     * b15: 0 右詰にする。
     * b14-b12: 0 (reserved)
     * b11: 0 自己診断無効
     * b10: 0 自己診断電圧ローテーションモード
     * b9-b8: 0 自己診断電圧固定モード時は設定禁止
     * b7-b6: 0 (reserved)
     * b5: 1 自動クリア有効
     * b4-b3: 0 (reserved)
     * b2-b1: 0 12bit精度でA/D変換を実施
     * b0: 0 (reserved)
     */
    S12AD.ADCER.WORD = 0x0020;

    /* ADSTRGR
     * b15-b14: 0 (reserved)
     * b13-b8: 3f シングルスキャンモード/連続すきゃノードのA/D変換開始トリガなし。
     * b7-b6: 0 (reserved)
     * b5-b0: 3f グループスキャンBのA/D変換開始トリガなし。
     */
    S12AD.ADSTRGR.WORD = 0x3f3f;

    /* ADSSTR0～ADSSTR7
     *   b7-b0: 0x14 サンプリング時間 */
    S12AD.ADSSTR0 = S12AD_SAMPLING_STATE_COUNT;
    S12AD.ADSSTR1 = S12AD_SAMPLING_STATE_COUNT;
    S12AD.ADSSTR2 = S12AD_SAMPLING_STATE_COUNT;
    S12AD.ADSSTR3 = S12AD_SAMPLING_STATE_COUNT;
    S12AD.ADSSTR4 = S12AD_SAMPLING_STATE_COUNT;
    S12AD.ADSSTR5 = S12AD_SAMPLING_STATE_COUNT;
    S12AD.ADSSTR6 = S12AD_SAMPLING_STATE_COUNT;
    S12AD.ADSSTR7 = S12AD_SAMPLING_STATE_COUNT;

    return ;
}

/**
 * S12ADの通常A/D変換を開始する。
 * 停止中のみ実行可能
 */
static void
s12ad_start_normal(void)
{
    if (!s12ad_is_busy()) {
        S12AD.ADCER.BIT.DIAGLD = 0;
        S12AD.ADCER.BIT.DIAGVAL = 0;
        S12AD.ADCER.BIT.DIAGM = 0;

        S12AD.ADCSR.BIT.ADST = 1;
        S12ADData.is_busy = 1;
    }
    return;
}

/**
 * S12ADの自己診断A/D変換を開始する。
 * 停止中のみ実行可能。
 *
 * @param diag_base 基準電圧。(AD_DIAG_x）を使用
 */
static void
s12ad_start_diag(uint8_t diag_base)
{
    if (!s12ad_is_busy()) {
        S12AD.ADCER.BIT.DIAGVAL = diag_base;
        S12AD.ADCER.BIT.DIAGLD = 1;
        S12AD.ADCER.BIT.DIAGM = 1;

        S12AD.ADCSR.BIT.ADST = 1;
        S12ADData.is_busy = 1;
    }
    return;
}


/**
 * S12ADにて、A/D変換中かどうかを判定する。
 *
 * @return A/D変換中の場合には非ゼロの値が返る。A/D停止中の場合には0が返る。
 */
static int
s12ad_is_busy(void)
{
    return S12ADData.is_busy;
}

/**
 * S12ADのA/D変換を停止する。
 */
static void
s12ad_stop(void)
{
    if (S12AD.ADGSPCR.BIT.PGS != 0) {
        S12AD.ADGSPCR.BIT.PGS = 0;
    }

    if (S12AD.ADCSR.BIT.ADCS == 1) {
        S12AD.ADSTRGR.WORD = 0x3f3f; /* トリガ入力無効 */
        if (S12AD.ADGCTRGR.BIT.GRCE == 1) {
            S12AD.ADGCTRGR.BIT.TRSC = 0x3f;
            S12AD.ADGCTRGR.BIT.GCADIE = 0; /* グループCスキャン終了割り込み禁止 */
        }
        S12AD.ADCSR.BIT.GBADIE = 0; /* グループBスキャン終了割り込み禁止 */
        S12AD.ADCSR.BIT.ADIE = 0; /* グループAスキャン終了割り込み禁止 */
    } else {
        S12AD.ADSTRGR.BIT.TRSA = 0x3f; /* トリガ入力無効 */
        S12AD.ADCSR.BIT.ADIE = 0; /* スキャン終了割り込みを禁止 */
    }

    S12AD.ADCSR.BIT.ADST = 0; /* A/D変換停止 */
    S12ADData.is_busy = 0;
    return ;
}

/**
 * S12ADを停止する。
 */
static void
s12ad_destroy(void)
{
    int ch;

    for (ch = 0; ch < S12AD_CH_COUNT; ch++) {
        s12adch_data_destroy(&S12ADData.chdata[ch]);
    }
    S12ADData.is_busy = 0;

    SYSTEM.PRCR.WORD = 0xA502;
    MSTP(S12AD) = 1; /* S12AD 停止 */
    SYSTEM.PRCR.WORD = 0xA500;
}


/**
 * S12ADを更新する。
 */
static void
s12ad_update(void)
{
    uint16_t adval_1, adval_2, adval_3, adval_4;
    uint16_t adrd;

    if (MSTP(S12AD) == 1) {
        return ; /* A/Dモジュール停止中 */
    }

    if (S12ADData.is_busy == 0) {
        return ; /* A/D変換要求出してない */
    }

    if (S12AD.ADCSR.BIT.ADST != 0) {
        return ; /* A/D変換処理中 */
    }

    if (S12AD.ADCER.BIT.DIAGM == 1) {
        /* 自己診断モード */
        adrd = S12AD.ADRD.WORD;
        S12ADData.diag_data = adrd & 0xfff; /* b15-b14は診断内容が返るので無視 */
    } else {
        /* 通常モード */
        adval_1 = S12AD.ADDR0;
        adval_2 = S12AD.ADDR1;
        adval_3 = S12AD.ADDR2;
        adval_4 = S12AD.ADDR3;

        s12adch_data_update(&S12ADData.chdata[S12AD_CH_1], adval_1);
        s12adch_data_update(&S12ADData.chdata[S12AD_CH_2], adval_2);
        s12adch_data_update(&S12ADData.chdata[S12AD_CH_3], adval_3);
        s12adch_data_update(&S12ADData.chdata[S12AD_CH_4], adval_4);
    }
    S12ADData.is_busy = 0; /* スキャン完了 */

    return;
}

/**
 * S12AD1を初期化する。
 */
static void
s12ad1_init(void)
{
    int i;

    for (i = 0; i < S12AD1_CH_COUNT; i++) {
        s12adch_data_init(&S12AD1Data.chdata[i]);
    }
    S12AD1Data.update_temp = 0;
    S12AD1Data.update_internal_base = 0;
    S12AD1Data.is_busy = 0;
    S12AD1Data.diag_data = 0;
    S12AD1Data.temperature = 0;
    S12AD1Data.internal_base = 0;

    SYSTEM.PRCR.WORD = 0xA502;
    MSTP(S12AD1) = 0; /* S12AD1 動作 */
    SYSTEM.PRCR.WORD = 0xA500;

    RX_UTIL_SET_INPUT_FUNC_PORT(PORT9, B0); /* AN114 */
    RX_UTIL_SET_INPUT_FUNC_PORT(PORT9, B2); /* AN116 */

    MPC.PWPR.BIT.B0WI = 0;
    MPC.PWPR.BIT.PFSWE = 1;

    /* P90PFS/P92PFS
     * b7: 1 アナログ端子として使用する。
     * b6: 0 (reserved)
     * b5-b0: 0 Hi-Z
     */
    MPC.P90PFS.BYTE = 0x80; /* AN114 */
    MPC.P92PFS.BYTE = 0x80; /* AN116 */

    MPC.PWPR.BIT.PFSWE = 0;
    MPC.PWPR.BIT.B0WI = 1;

    /* ADCSR
     *   b15: ADST=0 A/D変換停止
     *   b14-b13: ADCS=0 シングルスキャンモード
     *   b12: ADIE=0 スキャン終了時のS12ADIO割り込み発生の禁止
     *   b11-b10: 0 (Reserved)
     *   b9: TRGE=0 同期、非同期トリガによるA/D変換の開始を禁止。
     *   b8: 0 同期トリガによるA/D変換の開始を選択
     *   b7: DBLE=0 ダブルトリガモード非選択
     *   b6: GBADIE=0 グループBnoスキャン終了後に割り込み発生を禁止。
     *   b5: 0 (Reserved)
     *   b4-b0: DBLANS=0
     */
    S12AD1.ADCSR.WORD = 0x0000;

    /* ADANSA0
     *   b15: 0 AN115 A/D変換対象
     *   b14: 1 AN114 A/D変換対象
     *   b13: 0 AN113 A/D変換対象外
     *   b12: 0 AN112 A/D変換対象外
     *   b11: 0 AN111 A/D変換対象外
     *   b10: 0 AN110 A/D変換対象外
     *   b9: 0 AN109 A/D変換対象外
     *   b8： 0 AN108 A/D変換対象外
     *   b7: 0 AN107 A/D変換対象外
     *   b6: 0 AN106 A/D変換対象外
     *   b5: 0 AN105 A/D変換対象外
     *   b4: 0 AN104 A/D変換対象外
     *   b3: 0 AN103 A/D変換対象外
     *   b2: 0 AN102 A/D変換対象外
     *   b1: 0 AN101 A/D変換対象外
     *   b0: 0 AN100 A/D変換対象外
     */
    S12AD1.ADANSA0.WORD = 0x4000;
    /* ADANSA1
     * b15-b5: 0 (reserved)
     * b4: 0 AN120 A/D変換対象外
     * b3: 0 AN119 A/D変換対象外
     * b2: 0 AN118 A/D変換対象外
     * b1: 0 AN117 A/D変換対象外
     * b0: 1 AN116 A/D変換対象
     */
    S12AD1.ADANSA1.WORD = 0x0001;

    /* ADADS0
     * b15-b0: AN100～AN115のA/D変換値加算/平均モード非選択
     */
    S12AD1.ADADS0.WORD = 0x0000;

    /* ADADS1
     * b15-b5: 0 (reserved)
     * b4-b0: 0 AN116～AN120のA/D変換値加算/平均モード非選択
     */
    S12AD1.ADADS1.WORD = 0x0000;

    /* ADADC
     *   b7: 0 加算モード選択
     *   b6-b3: 0 (Reserved)
     *   b2-b0: 0 1回変換
     */
    S12AD1.ADADC.BYTE = 0x00;

    /* ADCER
     * b15: 0 右詰にする。
     * b14-b12: 0 (reserved)
     * b11: 0 自己診断無効
     * b10: 0 自己診断電圧ローテーションモード
     * b9-b8: 0 自己診断電圧固定モード時は設定禁止
     * b7-b6: 0 (reserved)
     * b5: 1 自動クリア有効
     * b4-b3: 0 (reserved)
     * b2-b1: 0 12bit精度でA/D変換を実施
     * b0: 0 (reserved)
     */
    S12AD1.ADCER.WORD = 0x0020;

    /* ADSTRGR
     * b15-b14: 0 (reserved)
     * b13-b8: 3f シングルスキャンモード/連続すきゃノードのA/D変換開始トリガなし。
     * b7-b6: 0 (reserved)
     * b5-b0: 3f グループスキャンBのA/D変換開始トリガなし。
     */
    S12AD1.ADSTRGR.WORD = 0x3f3f;

    /* ADEXICR
     * b15: 0 出力禁止
     * b14-b13: 0 アナログ入力チャンネルANx
     * b12: 0 (reserved)
     * b11: 0
     * b10: 0
     * b9: 0 内部基準電圧をA/D変換しない。
     * b8: 0 シングルスキャンモード/連続スキャンモード/グループAでの温度センサA/D変換しない。
     * b7-b2: 0 (Reserved)
     * b1: 0 内部基準電圧のA/D変換値加算/平均モード非選択
     * b0: 0 温度センサ出力 A/D変換値加算/平均モード非選択
     */
    S12AD1.ADEXICR.WORD = 0x0000;

    /* ADSSTR0～ADSSTR15, ADSSTRL, ADSSTRT, ADSSTRO
     *   b7-b0: 0x14 サンプリング時間 */
    S12AD1.ADSSTR0 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR1 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR2 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR3 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR4 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR5 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR6 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR7 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR8 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR9 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR10 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR11 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR12 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR13 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR14 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTR15 = S12AD1_SAMPLING_STATE_COUNT;
    S12AD1.ADSSTRL = S12AD1_SAMPLING_STATE_COUNT; /* AN116～AN120 */
    S12AD1.ADSSTRT = S12AD1_TEMP_SAMPLING_STATE_COUNT; /* 温度センサ */
    S12AD1.ADSSTRO = S12AD1_INTERNAL_BASE_SAMPLING_STATE_COUNT; /* 内部基準電圧 */

    return;
}
/**
 * S12AD1の通常A/D変換を開始する。
 * 停止中のみ実行可能
 */
static void
s12ad1_start_normal(void)
{
    if (!s12ad1_is_busy()) {
        S12AD1.ADCER.BIT.DIAGLD = 0;
        S12AD1.ADCER.BIT.DIAGVAL = 0;
        S12AD1.ADCER.BIT.DIAGM = 0;

        if (S12AD1Data.update_temp != 0) {
            S12AD1.ADEXICR.BIT.TSSA = 1;
        }
        if (S12AD1Data.update_internal_base != 0) {
            S12AD1.ADEXICR.BIT.OCSA = 1;
        }
        S12AD1.ADCSR.BIT.ADST = 1;
        S12AD1Data.is_busy = 1;
    }
    return;
}

/**
 * S12AD1の自己診断A/D変換を開始する。
 * 停止中のみ実行可能。
 *
 * @param diag_base 基準電圧。(AD_DIAG_x）を使用
 */
static void
s12ad1_start_diag(uint8_t diag_base)
{
    if (!s12ad1_is_busy()) {
        S12AD1.ADCER.BIT.DIAGVAL = diag_base;
        S12AD1.ADCER.BIT.DIAGLD = 1;
        S12AD1.ADCER.BIT.DIAGM = 1;

        S12AD1.ADCSR.BIT.ADST = 1;
        S12AD1Data.is_busy = 1;
    }
    return;
}

/**
 * S12AD1にて、A/D変換中かどうかを判定する。
 *
 * @return A/D変換中の場合には非ゼロの値が返る。A/D停止中の場合には0が返る。
 */
static int
s12ad1_is_busy(void)
{
    return S12AD1Data.is_busy;
}

/**
 * S12AD1のA/D変換を停止する。
 */
static void
s12ad1_stop(void)
{
    if (S12AD1.ADGSPCR.BIT.PGS != 0) {
        S12AD1.ADGSPCR.BIT.PGS = 0;
    }

    if (S12AD1.ADCSR.BIT.ADCS == 1) {
        S12AD1.ADSTRGR.WORD = 0x3f3f; /* トリガ入力無効 */
        if (S12AD1.ADGCTRGR.BIT.GRCE == 1) {
            S12AD1.ADGCTRGR.BIT.TRSC = 0x3f;
            S12AD1.ADGCTRGR.BIT.GCADIE = 0; /* グループCスキャン終了割り込み禁止 */
        }
        S12AD1.ADCSR.BIT.GBADIE = 0; /* グループBスキャン終了割り込み禁止 */
        S12AD1.ADCSR.BIT.ADIE = 0; /* グループAスキャン終了割り込み禁止 */
    } else {
        S12AD1.ADSTRGR.BIT.TRSA = 0x3f; /* トリガ入力無効 */
        S12AD1.ADCSR.BIT.ADIE = 0; /* スキャン終了割り込みを禁止 */
    }

    S12AD1.ADCSR.BIT.ADST = 0; /* A/D変換停止 */

    S12AD1Data.is_busy = 0;

    return ;
}

/**
 * S12AD1を停止する。
 */
static void
s12ad1_destroy(void)
{
    int ch;

    for (ch = 0; ch < S12AD1_CH_COUNT; ch++) {
        s12adch_data_destroy(&S12AD1Data.chdata[ch]);
    }

    SYSTEM.PRCR.WORD = 0xA502;
    MSTP(S12AD1) = 1; /* S12AD 停止 */
    SYSTEM.PRCR.WORD = 0xA500;

    return;
}

/**
 * S12AD1を更新する。
 */
static void
s12ad1_update(void)
{
    uint16_t adval_5, adval_6;
    uint16_t adrd;

    if (MSTP(S12AD1) == 1) {
        return ; /* A/Dモジュール停止中 */
    }

    if (S12AD1Data.is_busy == 0) {
        return ; /* A/D変換要求出していない */
    }

    if (S12AD1.ADCSR.BIT.ADST != 0) {
        return ; /* A/D変換処理中 */
    }

    if (S12AD1.ADCER.BIT.DIAGM == 1) {
        /* 自己診断モード */
        adrd = S12AD1.ADRD.WORD;
        S12AD1Data.diag_data = adrd & 0xfff; /* b15-b14は診断内容が返るので無視 */
    } else {
        /* 通常モード */
        adval_5 = S12AD1.ADDR14;
        adval_6 = S12AD1.ADDR16;
        if ((S12AD1Data.update_temp != 0) && (S12AD1.ADEXICR.BIT.TSSA != 0)) {
            S12AD1Data.temperature = S12AD1.ADTSDR;
            S12AD1Data.update_temp = 0;
            S12AD1.ADEXICR.BIT.TSSA = 0;
        }
        if ((S12AD1Data.update_internal_base != 0) && (S12AD1.ADEXICR.BIT.OCSA != 0)) {
            S12AD1Data.internal_base = S12AD1.ADOCDR;
            S12AD1Data.update_internal_base = 0;
            S12AD1.ADEXICR.BIT.OCSA = 0;
        }

        s12adch_data_update(&S12AD1Data.chdata[S12AD1_CH_5], adval_5);
        s12adch_data_update(&S12AD1Data.chdata[S12AD1_CH_6], adval_6);
    }
    S12AD1Data.is_busy = 0; /* スキャン完了 */

    return;
}





/**
 * 12bit A/Dコンバータのデータを初期化する。
 * @param ch_data データ
 */
static void
s12adch_data_init(struct s12adch_data *ch_data)
{
    int i;

    for (i = 0; i < AD_FILTER_COUNT; i++) {
        ch_data->data[i] = 0;
    }
    ch_data->data_count = 0;
    ch_data->average = 0;

    return ;
}

/**
 * 12bit A/Dコンバータのデータを破棄する。
 * @param ch_data チャンネルデータ
 */
static void
s12adch_data_destroy(struct s12adch_data *ch_data)
{
    return ;
}

/**
 * 12bit A/Dコンバータの取得したデータを追加して更新する。
 *
 * @param ch_data チャンネルデータ
 * @param new_data 新しいデータ
 */
static void
s12adch_data_update(struct s12adch_data *ch_data, uint16_t new_data)
{
    int i;

    if (ch_data->data_count < AD_FILTER_COUNT) {
        ch_data->data[ch_data->data_count] = new_data;
        ch_data->data_count++;
        ch_data->average = 0; /* 十分なデータが無い場合には 0 */
    } else {
        for (i = 1; i < AD_FILTER_COUNT; i++) {
            ch_data->data[i - 1] = ch_data->data[i];
        }
        ch_data->data[AD_FILTER_COUNT - 1] = new_data;
        ch_data->average = s12adch_calc_average(ch_data);
    }

    return;
}

/**
 * 12bit A/Dコンバータの平均値を取得する。
 *
 * @param ch_data チャンネルデータ
 * @return 平均値が返る。十分なデータが揃っていない場合には0が返る。
 */
static uint16_t
s12adch_calc_average(const struct s12adch_data *ch_data)
{
    uint32_t sum;
    uint32_t min, max;
    uint32_t average;
    int i;

    if (ch_data->data_count < AD_FILTER_COUNT) {
        return 0;
    }

    min = 0;
    max = 0;
    sum = 0;
    for (i = 0; i < AD_FILTER_COUNT; i++) {
        sum += ch_data->data[i];
        if (i == 0) {
            min = ch_data->data[i];
            max = ch_data->data[i];
        } else {
            if (ch_data->data[i] < min) {
                min = ch_data->data[i];
            }
            if (ch_data->data[i] > max) {
                max = ch_data->data[i];
            }
        }
    }
    sum -= (min + max); /* ノイズとして、最大値と最小値を除く。 */

    average = sum / (AD_FILTER_COUNT - 2);
    if (average > 0xFFF) {
        average = 0xFFF;
    }

    return average;
}
