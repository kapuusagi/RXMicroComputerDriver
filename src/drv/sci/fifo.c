/**
 * @file ソフトウェアFIFOモジュール
 * @author
 */
#include "fifo.h"

/**
 * FIFOを初期化する。
 * @param f FIFO
 */
void
fifo_init(struct fifo *f)
{
	int i;

	f->in = 0;
	f->out = 0;
	for (i = 0; i < sizeof(f->buf); i++) {
		f->buf[i] = 0x0;
	}
}

/**
 * FIFOを破棄する。
 * @param f FIFO
 */
void
fifo_destroy(struct fifo *f)
{
	f->out = 0;
	f->in = 0;
}

void
fifo_put(struct fifo *f, uint8_t d)
{
	uint16_t next;

	next = f->in + 1;
	if (next >= sizeof(f->buf)) {
		next = 0;
	}
	if (next == f->out) {
		// No enough space.
		return ;
	} else {
		f->buf[f->in] = d;
		f->in++;
		if (f->in >= sizeof(f->buf)) {
			f->in = 0;
		}
	}
}

/**
 * FIFOからデータを1byte取得する。
 * FIFOにデータがない場合には0が返る。
 *
 * @param f FIFO
 * @return データ。FIFOが空の場合には0が返る。
 */
uint8_t
fifo_get(struct fifo *f)
{
	uint8_t ret;

	if (f->in == f->out) {
		return 0; // No data.
	}

	ret = f->buf[f->out];
	f->out++;
	if (f->out >= sizeof(f->buf)) {
		f->out = 0;
	}
	return ret;
}

/**
 * FIFOにデータがあるかどうかを判定する。
 * @param f FIFO
 * @return データがある場合には非ゼロの値、データがない場合には0が返る。
 */
uint8_t
fifo_has_data(const struct fifo *f)
{
	return (f->in != f->out);
}

/**
 * FIFOに空きがあるかどうか判定する。
 * @param f FIFO
 * @return 空きがある場合には非ゼロの値、空きがない場合には0が返る。
 */
uint8_t
fifo_has_blank(const struct fifo *f)
{
	uint16_t in;

	in = f->in + 1;
	if (in >= sizeof(f->buf)) {
		in = 0;
	}
	return (in != f->out);
}

/**
 * FIFOに存在するデータ数を取得する。
 * @param f FIFO
 * @return データ数が返る。
 */
uint16_t
fifo_get_data_count(const struct fifo *f)
{
	if (f->in == f->out) {
		return 0; // No data.
	} else if (f->out < f->in) {
		return (f->in - f->out);
	} else {
		return (uint16_t)(f->out) + (uint16_t)(sizeof(f->buf) - f->in);
	}
}

