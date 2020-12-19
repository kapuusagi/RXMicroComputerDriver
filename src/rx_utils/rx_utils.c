#include "rx_utils.h"

/**
 * NOP命令を発効する。
 */
#pragma inline_asm rx_util_nop
void
rx_util_nop(void)
{
	NOP;
}

/**
 * WAIT命令を発効する。
 */
#pragma inline_asm rx_util_wait
void
rx_util_wait(void)
{
	WAIT;
}

/**
 * メモリをdにセットする。
 *
 * @param buf 初期化領域の戦闘領域
 * @param d 値
 * @param n 領域サイズ
 * @return bufが返る。
 */
void *
rx_memset(void *buf, int d, uint32_t n)
{
	uint64_t qword = (uint8_t)(d & 0xff);
	for (int i = 0; i < 7; i++) {
		qword <<= 8;
		qword |= (uint8_t)(d & 0xff);
	}
	uint64_t *p64 = (uint64_t*)(buf);
	while (n > 7) {
		*p64 = qword;
		p64++;
		n -= 8;
	}
	uint8_t *p8 = (uint8_t*)(p64);
	for (int i = 0; i < n; i++) {
		*p8 = (uint8_t)(d & 0xff);
		p8++;
	}
	return buf;
}
