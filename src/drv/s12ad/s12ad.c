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

#define AD_FILTER_COUNT  4

#define S12AD_DONE_IRQNO  (64)

typedef struct ad_channel {
	uint8_t is_active:1; // 動作対象かどうか
	uint8_t rsvd:3;
	uint8_t data_count:4; // 取得データ数
	uint16_t data[AD_FILTER_COUNT]; // スキャンしたデータ。最新はAD_FILTER_COUNT-1
	uint16_t ad_value; // 確定値
} ad_channel_t;

static uint8_t IsAdStarted = 0;

#define AD_CHANNEL_COUNT 1
static ad_channel_t ADChannels[AD_CHANNEL_COUNT];

static void s12ad_restart(void);
static uint16_t ad_channel_get_adval(ad_channel_t *ch);
static void ad_channel_clear(ad_channel_t *ch);
static void ad_channel_update(ad_channel_t *ch, uint16_t data);

/**
 * 12bit ADコンバータを初期化する。
 */
void
drv_s12ad_init(void)
{
	rx_memset(&ADChannels, 0x0, sizeof(ADChannels));

	SYSTEM.PRCR.WORD = 0xA502;
	MSTP(S12AD) = 0; // S12ADを稼働
	SYSTEM.PRCR.WORD = 0xA500;

	// 使用するポート
	PORT4.PDR.BIT.B0 = 0;
	PORT4.PMR.BIT.B0 = 1;

	// ADCSR
	//   b15: 0=AD変換停止
	//   b14-b13: 0=シングルスキャンモード
	//   b12: 0=スキャン終了後の割り込み禁止
	//   b11-b10: 0固定
	//   b9: 0=トリガを禁止
	//   b8: 0=同期トリガによるA/D変換の開始を禁止
	//   b7: 0=ダブルトリガモードにしない
	//   b6: 0=グループBスキャン割り込み禁止
	//   b5: 0=固定
	//   b4-b0: 0=ダブルトリガチャンネル選択
	S12AD.ADCSR.WORD = 0x0000;

	S12AD.ADADS0.WORD = 0; // AN000~AN007のA/D変換加算・平均モードを使わない。
	S12AD.ADADC.BYTE = 0; // AN000~AN007のA/D変換加算・平均モードを使わない。

	// ADCER
	//   b15: 0=A/Dデータレジスタの値は右詰め
	//   b14-b12: 0固定
	//   b11: 0=A/Dコンバータ故障の自己診断機能は使わない
	//   b10: 0=自己診断モードはローテーションモード
	//   b9-b8: 0=自己診断変換電圧選択しない
	//   b7-b6: 0固定
	//   b5: 1=読み出した後、自動クリアを行う
	//   b4-b3: 0固定
	//   b2-b1: 0=12ビット精度でA/D変換を実施
	//   b0: 0固定
	S12AD.ADCER.WORD = 0x0020;

	S12AD.ADSTRGR.WORD = 0x0000; // 変換開始トリガを設定しない。

	return ;
}

/**
 * 指定チャンネルのA/D変換を有効にする。
 *
 * @param channel_no チャンネル番号
 */
void
drv_s12ad_start(uint8_t channel_no)
{
	int is_active_channel_changed = 0;

	switch (channel_no) {
	case AD_CHANNEL_1:
		if (ADChannels[0].is_active == 0) {
			ADChannels[0].is_active = 1;
			is_active_channel_changed = 1;
		}
		break;
	}
	if (is_active_channel_changed) {
		// AD start.
		if (IsAdStarted) {
			s12ad_restart();
		}
	}

	return ;
}

/**
 * 指定チャンネルのA/Dを停止する。
 *
 * @param channel_no チャンネル番号
 */
void
drv_s12ad_stop(uint8_t channel_no)
{
	int is_active_channel_changed = 0;

	switch (channel_no) {
	case AD_CHANNEL_1:
		if (ADChannels[0].is_active) {
			ADChannels[0].is_active = 0;
			ad_channel_clear(&ADChannels[0]);
		}
		break;
	}
	if (is_active_channel_changed) {
		if (IsAdStarted) {
			s12ad_restart();
		}
	}
	return ;
}

/**
 * 現在の設定をもとにA/Dをスタート/停止する。
 */
static void
s12ad_restart(void)
{
	uint16_t adansa0;

	if (S12AD.ADCSR.BIT.ADST != 0) {
		// A/D変換が実行中の場合、一度停止する。
		S12AD.ADCSR.BIT.ADST = 0;
	}

	// A/Dチャンネル選択
	adansa0 = 0;
	if (ADChannels[0].is_active != 0) {
		adansa0 |= (1 << 0); // AN000
	}
	S12AD.ADANSA0.WORD = adansa0;
	if (adansa0 != 0) {
		IsAdStarted = 1;
		S12AD.ADCSR.BIT.ADIE = 1; // 割り込み通知可
		S12AD.ADCSR.BIT.ADST = 1; // AD変換開始
	} else {
		IsAdStarted = 0;
		S12AD.ADCSR.BIT.ADIE = 0; // 割り込み通知禁止
		S12AD.ADCSR.BIT.ADST = 0; // AD変換停止
	}

	return ;
}

/**
 * A/Dの状態を更新し、データの取得と再設定を行う。
 * A/Dが完了していれば取得する。
 * A/Dが完了していなければ取得も再設定も行わない。
 */
void
drv_s12ad_update(void)
{
	uint16_t ad_channels;

	if (IsAdStarted) {
		if (S12AD.ADCSR.BIT.ADST != 0) {
			// まだADが完了していない。
			return ;
		}
		// 選択したチャンネルのA/D変換が完了した。
		ad_channels = S12AD.ADANSA0.WORD;
		if ((ad_channels & (1 << 0)) != 0) {
			ad_channel_update(&ADChannels[0], S12AD.ADDR0);
		}

		IsAdStarted = 0;
	}

	// 必要なら次のスキャンを開始する。
	s12ad_restart();
}

/**
 * 指定チャンネルのA/D変換値を取得する。
 * @param channel_no チャンネル番号
 * @return A/D変換した結果の値。(0x0～0xfff)
 */
uint16_t
drv_s12ad_get(uint8_t channel_no)
{
	switch (channel_no) {
	case AD_CHANNEL_1:
		return ad_channel_get_adval(&ADChannels[0]);
	default:
		return 0;
	}
}




/**
 * A/Dチャンネルの値を取得する。
 * @param ch チャンネル
 * @return A/Dチャンネルの値。(0x0～0xfff)
 */
static uint16_t
ad_channel_get_adval(ad_channel_t *ch)
{
	if ((ch->is_active != 0)
			&& (ch->data_count >= AD_FILTER_COUNT)) {
		// チャンネルが有効で
		// スキャンデータ数が十分であるとき。
		return ch->ad_value;
	} else {
		return 0;
	}
}

/**
 * A/Dチャンネルのデータをクリアする。
 * @param ch チャンネル
 */
static void
ad_channel_clear(ad_channel_t *ch)
{
	uint8_t i;

	ch->data_count = 0;
	ch->ad_value = 0;
	for (i = 0; i < AD_FILTER_COUNT; i++) {
		ch->data[i] = 0x0;
	}
}
/**
 * A/Dチャンネルのデータを更新する。
 * @param ch チャンネル
 * @param data A/D値
 */
static void
ad_channel_update(ad_channel_t *ch, uint16_t data)
{
	uint8_t i;
	uint16_t min, max;
	uint32_t sum;

	min = 0xfff;
	max = 0;
	sum = 0;
	for (i = 0; i < AD_FILTER_COUNT; i++) {
		if (i < (AD_FILTER_COUNT - 1)) {
			ch->data[i] = ch->data[i + 1];
		} else {
			ch->data[i] = data;
		}
		if (ch->data[i] < min) {
			min = ch->data[i];
		}
		if (ch->data[i] > max) {
			max = ch->data[i];
		}
		sum += ch->data[i];
	}
	if (ch->data_count < AD_FILTER_COUNT) {
		ch->data_count++;
	}
	if (ch->data_count >= AD_FILTER_COUNT) {
		// 十分なデータ量が揃ったので確定値を取得する。
		// 最大値と最小値のデータ分を除外。
		sum -= ((uint32_t)(min) + (uint32_t)(max));
		ch->ad_value = ((sum * 10) / (AD_FILTER_COUNT - 2)) / 10;
	}
}

