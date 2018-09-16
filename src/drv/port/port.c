/**
 * @file ポートドライバファイル
 *
 * 物理的なI/Oポートを抽象化して公開する。
 * 本モジュールでは、物理的なポートへの割り当てと、出力論理変換を行う。
 * ポートドライバを使う側は、正論理/負論理を考慮しないで、ON/OFFだけで
 * アクセスすることができる。
 * 入力に対してはチャタリング防止のためにフィルタ処理を加えている。
 */
#include "port.h"

#include <iodefine.h>
#include <string.h>

/*------------------------------------------------------
 * ポートドライバ定義
 *------------------------------------------------------*/

struct input_port {
	uint8_t data:1; // Data.
	uint8_t prev_data:1;
	uint8_t rsvd:6;
	uint8_t count; // Accept count.
};


static struct {
	uint8_t debug_led_1:1;
	uint8_t rsvd:6;
} OutputPorts;

static struct {
	struct input_port user_sw_1;
	struct input_port btn_up;
	struct input_port btn_down;
	struct input_port btn_left;
	struct input_port btn_right;
	struct input_port btn_center;
} InputPorts;

// 入力スイッチフィルタ
#define USER_SW_FILTER (0)

static void input_port_update(struct input_port *port, uint8_t onoff, uint8_t filter_count);

/**
 * ポートドライバを初期化する。
 */
void
drv_port_init(void)
{
	memset(&OutputPorts, 0x0, sizeof(OutputPorts));
	memset(&InputPorts, 0x0, sizeof(InputPorts));

	// デバッグLED出力ポート
	PORT7.PDR.BIT.B0 = 1;
	PORT7.PMR.BIT.B0 = 0;

	// ユーザーSWポート
	PORT0.PDR.BIT.B5 = 0;
	PORT0.PMR.BIT.B5 = 0;

	// ジョイスティック
	PORT6.PDR.BIT.B7 = 0;
	PORT6.PMR.BIT.B7 = 0;

	PORTF.PDR.BIT.B5 = 0;
	PORTF.PMR.BIT.B5 = 0;

	PORT1.PDR.BIT.B3 = 0;
	PORT1.PMR.BIT.B3 = 0;

	PORT0.PDR.BIT.B3 = 0;
	PORT0.PMR.BIT.B3 = 0;

	PORT1.PDR.BIT.B2 = 0;
	PORT1.PDR.BIT.B2 = 0;

	drv_port_update_output();

	return ;
}

/**
 * ポートデータを更新する。
 * 入力ポートに対しては何もしない。
 *
 * @param port_no ポート番号
 * @param on 0:OFF, 1:ON
 */
void
drv_port_write(uint8_t port_no, uint8_t on)
{
	switch (port_no) {
	case PORT_NO_DEBUG_LED_1:
		OutputPorts.debug_led_1 = on;
		break;
	default:
		break;
	}
	return ;
}

/**
 * ポートの値を読む。
 *
 * @param port_no ポート番号(PORT_NO_xx)
 * @return ポートの状態(ON/OFF)が返る。
 */
uint8_t
drv_port_read(uint8_t port_no)
{
	switch (port_no)
	{
	case PORT_NO_DEBUG_LED_1:
		return OutputPorts.debug_led_1;
	case PORT_NO_USER_SW_1:
		return InputPorts.user_sw_1.data;
	case PORT_NO_BTN_UP:
		return InputPorts.btn_up.data;
	case PORT_NO_BTN_DOWN:
		return InputPorts.btn_down.data;
	case PORT_NO_BTN_LEFT:
		return InputPorts.btn_left.data;
	case PORT_NO_BTN_RIGHT:
		return InputPorts.btn_right.data;
	case PORT_NO_BTN_CENTER:
		return InputPorts.btn_center.data;
	default:
		return 0;
	}
}

/**
 * 入力ポートの状態を読み、状態を更新する。
 */
void
drv_port_update_input(void)
{
	input_port_update(&InputPorts.user_sw_1,
			(PORT0.PIDR.BIT.B5 != 0) ? 1 : 0, USER_SW_FILTER);

	input_port_update(&InputPorts.btn_up,
			(PORT6.PIDR.BIT.B7 != 0) ? 1 : 0, USER_SW_FILTER);
	input_port_update(&InputPorts.btn_down,
			(PORTF.PIDR.BIT.B4 != 0) ? 1 : 0, USER_SW_FILTER);
	input_port_update(&InputPorts.btn_left,
			(PORT1.PIDR.BIT.B3 != 0) ? 1 : 0, USER_SW_FILTER);
	input_port_update(&InputPorts.btn_right,
			(PORT0.PIDR.BIT.B3 != 0) ? 1 : 0, USER_SW_FILTER);
	input_port_update(&InputPorts.btn_center,
			(PORT1.PIDR.BIT.B2 != 0) ? 1 : 0, USER_SW_FILTER);

}

/**
 * 出力ポートの状態を更新する。
 */
void
drv_port_update_output(void)
{
	PORT7.PODR.BIT.B0 = (OutputPorts.debug_led_1 != 0) ? 0 : 1;
}

/**
 * １つの入力ポートの状態を更新する。
 * onoffに渡す値は正論理と負論理を解決したものを渡す。
 * 新しいポートの値が検出されたとき、filter_count数だけ検出できた場合に確定値とする。
 *
 * @param port ポート
 * @param onoff 0:OFF, 1:ON
 * @param filter_count フィルタ数。
 */
static void
input_port_update(struct input_port *port, uint8_t onoff, uint8_t filter_count)
{
	if (onoff != port->prev_data) {
		// 今回値は前回値と異なるので、前回値を今回値で更新し、
		// カウンタを初期化する。
		port->prev_data = onoff;
		port->count = 0;
	} else {
		// 今回値は前回値と同じ。
		if (port->prev_data == port->data) {
			// 今回値と前回値、確定値がすべて同じ。
			// 何もしない。
		} else {
			// 今回値と前回値は同じだが、確定値と異なる。
			port->count++;
			if (port->count >= filter_count) {
				// 今回値と前回値が同じだが、
				// 確定値が異なる状態が filter_count数検出された。
				// 確定値を更新する。
				port->data = port->prev_data;
				port->count = 0;
			} else {
				// 新しい値の検出回数がfilter_count回以下。
			}
		}
	}
}
