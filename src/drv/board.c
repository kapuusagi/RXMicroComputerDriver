#include <iodefine.h>
#include "../rx_utils/rx_utils.h"
#include "board.h"

/* クロックソース */
#define USE_PLLSRC_TO_HOCO           1

#if USE_PLLSRC_TO_HOCO == 0
/* 外部発振器(12MHz)を使用するかどうか。 */
#define USE_EXT_OSCILLATOR 0
#endif

#if (USE_EXT_OSCILLATOR == 0)
#endif

static void set_mainclock_to_PLL(void);
static void disable_subclock(void);

/**
 * ウェークアップする。
 */
void
board_init_on_reset(void)
{
	SYSTEM.PRCR.WORD = 0xA503; /* プロテクト解除 */
	/* P36 XTAL, P37 EXTALを設定 */
	RX_UTIL_SET_INPUT_FUNC_PORT(PORT3, B7);
	RX_UTIL_SET_OUTPUT_FUNC_PORT(PORT3, B6);


	set_mainclock_to_PLL();
	disable_subclock();

	SYSTEM.PRCR.WORD = 0xA500; /* プロテクト */
}

/**
 * メインクロックをPLLに切り替える。
 */
static void
set_mainclock_to_PLL(void)
{
#if USE_PLLSRC_TO_HOCO
	SYSTEM.MOSCCR.BIT.MOSTP = 1; /* メインクロック発振停止 */
	/* メインクロック発振停止待ち */
	while (SYSTEM.OSCOVFSR.BIT.MOOVF != 0) {
		rx_util_nop();
	}

	/* 内部リセット解除後、HOCOCR2 レジスタのHCFRQ[1:0] ビットで周波数を設定 */
	SYSTEM.HOCOCR2.BIT.HCFRQ = 0; /* HOCO 16MHz */

	/* HOCOCR レジスタのHCSTP ビットでHOCO クロックを動作に設定。
	 * 但し、OFS1.HOCOENが0であれば設定する必要は無し。 */
	if (OFSM.OFS1.BIT.HOCOEN != 0) {
		/* リセット後、HOCOで動作していないので動作させる。 */
		SYSTEM.HOCOCR.BIT.HCSTP = 0; /* HOCO動作 */
		/* HOCO発振安定待ち */
		while (SYSTEM.OSCOVFSR.BIT.HCOVF == 0) {
			rx_util_nop();
		}
	}

	/* ROMWT レジスタのROMWT[1:0] ビットを“10b” に設定 */
	SYSTEM.ROMWT.BIT.ROMWT = 2; /* 2ウェイト */
	while (SYSTEM.ROMWT.BIT.ROMWT != 2) {
		rx_util_nop();
	}

	/* SCKCR
	 * b31-b28:2 4分周 FCLK = 240MHz/4 = 60MHz
	 * b27-b24:1 2分周 ICLK = 240MHz/2 = 120MHz
	 * b23: 1 BCLK端子出力禁止(Hi-Z)
	 * b22: 1 SDCLK端子出力禁止(Hi-Z)
	 * b21-b20:0 (reserved)
	 * b19-b16:2 4分周 BCK = 240MHz/4 = 60MHz
	 * b15-b12:1 2分周 PCLKA = 240MHz/2 = 120MHz
	 * b11-b8:2 4分周 PCLKB = 240MHz/4 = 60MHz
	 * b7-b4:3 8分周 PCLKC = 240MHz/8 = 30MHz
	 * b3-b0:3 8分周 PCLKD = 240MHz/8 = 30MH
	 */
	SYSTEM.SCKCR.LONG = 0x21c21233;
	SYSTEM.SCKCR2.BIT.UCK = 4; /* 5分周 240MHz/5 = 48MHz */

	SYSTEM.PLLCR.BIT.PLIDIV = 0; /* 1分周 */
	SYSTEM.PLLCR.BIT.STC = 0x1d; /* 15逓倍 */
	SYSTEM.PLLCR.BIT.PLLSRCSEL = 1; /* PLL入力はHOCO */
	SYSTEM.PLLCR2.BIT.PLLEN = 0; /* PLL開始 */

	/* PLL動作待ち */
	while (SYSTEM.OSCOVFSR.BIT.PLOVF == 0) {
		rx_util_nop();
	}

	SYSTEM.SCKCR3.BIT.CKSEL = 4; /* PLL回路選択 */

	return ;
#else
	SYSTEM.MOSCCR.BIT.MOSTP = 1; /* メインクロック発振停止 */
	/* メインクロック発振停止待ち */
	while (SYSTEM.OSCOVFSR.BIT.MOOVF != 0) {
		rx_util_nop();
	}

#if USE_EXT_OSCILLATOR
	/* 外部クロック 12MHｚが入っているはずだが、PLLが発振安定しない */
	SYSTEM.MOFCR.BIT.MOFXIN = 0;
	SYSTEM.MOFCR.BIT.MODRV2 = 2; /* 12MHz入力 */
	SYSTEM.MOFCR.BIT.MOSEL = 1; /* 外部クロック入力 */
#else
	SYSTEM.MOFCR.BIT.MOSEL = 0; /* 内蔵発振子を使用する */
	SYSTEM.MOFCR.BIT.MODRV2 = 2; /* 8-16MHz */
	SYSTEM.MOSCWTCR.BYTE = 0x53; /* 発振ウェイトコントロール */
	SYSTEM.MOSCCR.BIT.MOSTP = 0; /* メインクロック発振動作 */
	/* メインクロック発振安定待ち */
	while (SYSTEM.OSCOVFSR.BIT.MOOVF != 1) {
		rx_util_nop();
	}
#endif

	SYSTEM.ROMWT.BIT.ROMWT = 2; /* 2ウェイト */
	while (SYSTEM.ROMWT.BIT.ROMWT != 2) {
		rx_util_nop();
	}


	/* SCKCR
	 * b31-b28:2 4分周 FCLK = 240MHz/4 = 60MHz
	 * b27-b24:1 2分周 ICLK = 240MHz/2 = 120MHz
	 * b23: 1 BCLK端子出力禁止(Hi-Z)
	 * b22: 1 SDCLK端子出力禁止(Hi-Z)
	 * b21-b20:0 (reserved)
	 * b19-b16:2 4分周 BCK = 240MHz/4 = 60MHz
	 * b15-b12:1 2分周 PCLKA = 240MHz/2 = 120MHz
	 * b11-b8:2 4分周 PCLKB = 240MHz/4 = 60MHz
	 * b7-b4:3 8分周 PCLKC = 240MHz/8 = 30MHz
	 * b3-b0:3 8分周 PCLKD = 240MHz/8 = 30MH
	 */
	SYSTEM.SCKCR.LONG = 0x21c21233;
	SYSTEM.SCKCR2.BIT.UCK = 4; /* 5分周 240MHz/5 = 48MHz */

	SYSTEM.PLLCR.BIT.PLIDIV = 0; /* 1分周 */
	SYSTEM.PLLCR.BIT.STC = 0x27; /* 20逓倍 */
	SYSTEM.PLLCR.BIT.PLLSRCSEL = 0; /* PLL入力はメインクロック発振器 */
	SYSTEM.PLLCR2.BIT.PLLEN = 0; /* PLL開始 */

	/* PLL動作待ち */
	while (SYSTEM.OSCOVFSR.BIT.PLOVF == 0) {
		rx_util_nop();
	}

	SYSTEM.SCKCR3.BIT.CKSEL = 4; /* PLL回路選択 */

	return ;
#endif
}

/**
 * サブクロックを使用しないため発振停止する。
 */
static void
disable_subclock(void)
{
	RTC.RCR4.BIT.RCKSEL = 0; /* サブクロック選択 */
	RTC.RCR3.BIT.RTCEN = 0; /* サブクロック発振停止 */
	while (RTC.RCR3.BIT.RTCEN != 0) {
		rx_util_nop();
	}
	SYSTEM.SOSCCR.BIT.SOSTP = 1; /* サブクロック発振停止 */
	while (SYSTEM.SOSCCR.BIT.SOSTP != 1) {
		rx_util_nop();
	}

	/* 発振停止待ち */
	while (SYSTEM.OSCOVFSR.BIT.SOOVF != 0) {
		rx_util_nop();
	}

	return ;
}
