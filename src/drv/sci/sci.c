/**
 * @file SCI 調歩同期式ドライバ
 * @author
 */
#include <iodefine.h>
#include "../../rx_utils/rx_utils.h"
#include "../board_config.h"

#include "sci.h"
#include "fifo.h"

#define SCI_INTERRUPT_PRIORITY 3

struct sci_config {
    uint32_t baudrate;
    uint8_t data_bits:5; /* データビット長 */
    uint8_t rsvd1:3;
    uint8_t stop_bits:2; /* ストップビット 1 or 2 */
    uint8_t parity:2; /* 0:パリティ無し 1:偶数パリティ 2:奇数パリティ */
    uint8_t flow_en:1; /* 0:フロー制御なし 1:CTSn/RTSnによるフロー制御あり */
    uint8_t rsvd2:3;
};

struct sci0_entry {
    RXREG struct st_sci0 *sci;
    struct fifo rx_fifo;
    struct fifo tx_fifo;
    const struct sci_config *config;
};

/**
 * デバッグ用UARTコンフィグ
 */
static const struct sci_config DebugSCIConfig = {
    .baudrate = 115200,
    .data_bits = 8, /* データビット長 = 8 */
    .stop_bits = 1, /* ストップビット=1 */
    .parity = 0, /* パリティなし */
    .flow_en = 0 /* フロー制御なし */
};

/**
 * SCI1 : デバッグ用UART
 */
static struct sci0_entry Sci1 = {
    .sci = &(SCI1),
    .config = &DebugSCIConfig
};

/**
 * Bluetoothモジュール SCIコンフィグ
 */
static const struct sci_config BluetoothSCIConfig = {
    .baudrate = 115200,
    .data_bits = 8, /* データビット長 = 8 */
    .stop_bits = 1, /* ストップビット=1 */
    .parity = 0, /* パリティなし */
    .flow_en = 0 /* フロー制御なし */
};
/**
 * SCI5 : Bluetooth モジュールI/F
 */
static struct sci0_entry Sci5 = {
    .sci = &(SCI5),
    .config = &BluetoothSCIConfig
};

/**
 * コンプレッサ用SCIコンフィグ
 */
static const struct sci_config CompressorSCIConfig = {
    .baudrate = 2400, /* 2400bps */
    .data_bits = 8, /* データビット長 = 8 */
    .stop_bits = 1, /* ストップビット = 1 */
    .parity = 0, /* パリティなし */
    .flow_en = 0 /* フロー制御なし */
};
/**
 * SCI6 : コンプレッサ I/F
 */
static struct sci0_entry Sci6 = {
    .sci = &(SCI6),
    .config = &CompressorSCIConfig
};

static struct sci0_entry* get_sci_entry(uint8_t ch);

static void sci0_init(struct sci0_entry *entry);
static void sci0_setup_baudrate(struct sci0_entry *entry);
static void sci0_destroy(struct sci0_entry *entry);
static void sci0_clear_error(struct sci0_entry *entry);
static int sci0_send(struct sci0_entry *entry, const uint8_t *data, uint16_t len);
static int sci0_recv(struct sci0_entry *entry, uint8_t *buf, uint16_t bufsize);

/**
 * SCIドライバを初期化する。
 */
void
drv_sci_init(void)
{
    SYSTEM.PRCR.WORD = 0xA502;
    MSTP(SCI5) = 0; /* SCI5動作 */
    SYSTEM.PRCR.WORD = 0xA500;

    RX_UTIL_SET_INPUT_FUNC_PORT(PORTC, B2); /* SCI5 RX */
    RX_UTIL_SET_OUTPUT_FUNC_PORT(PORTC, B3); /* SCI5 TX */

    /* ピン機能を有効化
     * マルチファンクションピンの設定
     * P0WIに0を書いてからPFSWE=1に設定し、ピン設定を変更する。 */
    MPC.PWPR.BIT.B0WI = 0;
    MPC.PWPR.BIT.PFSWE = 1;
    /* PC2PFS/PC3PFS
     * b7: 0 (reserved)
     * b6: 0 IRQn入力端子として使用しない。
     * b5-b0: 0x0a RXD5/TXD5
     */
    MPC.PC2PFS.BYTE = 0x0a; /* PC2 RXD5 */
    MPC.PC3PFS.BYTE = 0x0a; /* PC3 TXD5 */

    MPC.PWPR.BIT.PFSWE = 0;
    MPC.PWPR.BIT.B0WI = 1;

    sci0_init(&Sci5);

    IPR(SCI5, TXI5) = SCI_INTERRUPT_PRIORITY;
    IPR(SCI5, RXI5) = SCI_INTERRUPT_PRIORITY;

    IEN(SCI5, TXI5) = 1;
    IEN(SCI5, RXI5) = 1;

    return ;
}
/**
 * SCIドライバを破棄する。
 */
void
drv_sci_destroy(void)
{
    IEN(SCI5, TXI5) = 0;
    IEN(SCI5, RXI5) = 0;

    EN(SCI5, ERI5) = 0; /* 受信エラー割り込み禁止 */

    sci0_destroy(&Sci5);

    SYSTEM.PRCR.WORD = 0xA502;
    MSTP(SCI5) = 1; /* SCI5停止 */
    SYSTEM.PRCR.WORD = 0xA500;

    return ;
}

/**
 * SCIでデータを送信する。
 * 送信FIFOに入りきらないデータは捨てられる。
 *
 * @param ch SCIチャンネル(SCI_CH_xを使用する)
 * @param data 送信データ
 * @param len 送信データサイズ[byte]
 * @return 成功した場合には送信バイト数が返る。
 *         エラーが発生した場合には-1が返る。
 */
int
drv_sci_send(uint8_t ch, const uint8_t *data, uint16_t len)
{
    struct sci0_entry *entry;
    int retval = -1;

    entry = get_sci_entry(ch);
    if (entry != NULL) {
        retval = sci0_send(entry, data, len);
    }
    return retval;
}
/**
 * SCIでデータを受信する。
 * 関数呼び出し時点で、受信済みのデータを取得して返す。
 * 所定のバイト数受信するような待機は行わない。
 *
 * @param ch SCIチャンネル(SCICH_xを使用する)
 * @param buf 受信バッファ
 * @param bufsize バッファサイズ[byte]
 * @return 成功した場合には受信バイト数が返る。
 *         エラーが発生した場合には-1が返る。
 */
int
drv_sci_recv(uint8_t ch, uint8_t *buf, uint16_t bufsize)
{
    struct sci0_entry *entry;
    int retval = -1;

    entry = get_sci_entry(ch);
    if (entry != NULL) {
        retval = sci0_recv(entry, buf, bufsize);
    }
    return retval;
}

/**
 * チャンネルに対応するSCIエントリを得る。
 *
 * @param ch SCIチャンネル
 * @return SCIエントリ
 */
static struct sci0_entry*
get_sci_entry(uint8_t ch)
{
    switch (ch) {
        case SCI_CH_DEBUG:
            return &Sci1;
        default:
            return NULL;
    }
}

/**
 * SCI0(と同じモジュール)を初期化する。
 *
 * @param entry SCIエントリ
 */
static void
sci0_init(struct sci0_entry *entry)
{
    RXREG struct st_sci0 *sci = entry->sci;

    fifo_init(&(entry->rx_fifo));
    fifo_init(&(entry->tx_fifo));

    /* 送受信禁止 */
    sci->SCR.BIT.RE = 0;
    sci->SCR.BIT.TE = 0;

    sci->SCR.BIT.CKE = 0; /* SCKn端子Hi-Z */
    sci->SIMR1.BIT.IICM = 0; /* シリアルインタフェースモード */
    sci->SPMR.BIT.CKPH = 0; /* クロック遅れ無し */
    sci->SPMR.BIT.CKPOL = 0; /* クロック極性反転なし */

    sci->SMR.BIT.CM = 0; /* 調歩同期式モード */
    if (entry->config->data_bits == 9) {
        /* データビット長 = 9 */
        sci->SCMR.BIT.CHR1 = 0;
        sci->SMR.BIT.CHR = 0;
    } else if (entry->config->data_bits == 7) {
        /* データビット長 = 7 */
        sci->SCMR.BIT.CHR1 = 1;
        sci->SMR.BIT.CHR = 1;
    } else {
        /* データビット長 = 8 */
        sci->SCMR.BIT.CHR1 = 1;
        sci->SMR.BIT.CHR = 0;
    }


    sci->SMR.BIT.PE = (entry->config->parity != 0) ? 1 : 0;
    sci->SMR.BIT.PM = (entry->config->parity == 2) ? 1 : 0;
    sci->SMR.BIT.STOP = (entry->config->stop_bits == 2) ? 1 : 0;
    sci->SMR.BIT.MP = 0; /* マルチプロセッサモード通信を禁止 */

    sci->SCMR.BIT.SMIF = 0; /* 非スマートカードインタフェースモード */
    sci->SCMR.BIT.SINV = 0; /* ビット反転しない */
    sci->SEMR.BIT.ACS0 = 0; /* 外部クロック */
    sci->SEMR.BIT.BRME = 0; /* ビットレートモジュレーション無効 */
    sci->SEMR.BIT.ABCS = 0; /* 基本クロック16サイクルの期間を1ビット期間の転送レート */
    sci->SEMR.BIT.NFEN = 1; /* デジタルノイズフィルタ機能有効 */
    sci->SEMR.BIT.BGDM = 0; /* ボーレートジェネレータから通常の周波数のクロック出力 */
    sci->SEMR.BIT.RXDESEL = 0; /* RXDn端子のLowレベルでスタートビット検出 */

    /* ボーレート設定 */
    sci0_setup_baudrate(entry);

    sci->SCR.BIT.RE = 1; /* シリアル受信許可 */
    sci->SCR.BIT.RIE = 1; /* RXI/ERI割り込み許可 */
    sci->SCR.BIT.TE = 1; /* シリアル送信許可 */
    sci->SCR.BIT.TIE = 0; /* TXI割り込み禁止 */

    return;
}

/**
 * SCI0(と同じモジュール)のボーレートを設定する。
 *
 * @param entry SCI0エントリ
 */
static void
sci0_setup_baudrate(struct sci0_entry *entry)
{
    RXREG struct st_sci0 *sci = entry->sci;
    int n = 0;
    uint8_t cks = 0;
    const uint16_t ck_table[] = { 32, 128, 512, 2048 };

    for (cks = 0; cks < 4; cks++) {
        n = PCLKB_CLOCK / (ck_table[cks] * entry->config->baudrate);
        if ((n <= 256) &&  (n >= 1)) {
            /* brrに設定可能な値がある */
            break;
        }
    }

    if ((cks <= 3) && (n >= 1) && (n <= 256)) {
        /* 設定可能 */
        sci->SMR.BIT.CKS = cks;
        sci->BRR = (uint8_t)((n - 1) & 0xff);
    } else {
        /* 設定不可 -> 9600bps 強制設定 */
        sci->SMR.BIT.CKS = 0;
        sci->BRR = 194;
    }
    return;
}

/**
 * SCI0(と同じモジュール)を破棄する。
 *
 * @param entry SCIエントリ
 */
static void
sci0_destroy(struct sci0_entry *entry)
{
    RXREG struct st_sci0 *sci = entry->sci;

    sci->SCR.BIT.TIE = 0; /* TXI割り込み停止 */
    sci->SCR.BIT.RIE = 0; /* RXI/ERI割り込み停止 */
    sci0_clear_error(entry);

    sci->SCR.BIT.RE = 0; /* 受信禁止 */
    sci->SCR.BIT.TE = 0; /* 送信禁止 */

    fifo_destroy(&(entry->rx_fifo));
    fifo_destroy(&(entry->tx_fifo));

    return;
}
/**
 * SCI0(と同じモジュール)のエラーをクリアする。
 *
 * @param entry SCI0エントリ
 */
static void
sci0_clear_error(struct sci0_entry *entry)
{
    RXREG struct st_sci0 *sci = entry->sci;

    if (sci->SSR.BIT.PER != 0) {
        sci->SSR.BIT.PER = 0;
    }
    if (sci->SSR.BIT.FER != 0) {
        sci->SSR.BIT.FER = 0;
    }
    if (sci->SSR.BIT.ORER != 0) {
        sci->SSR.BIT.ORER = 0;
    }

    return;
}
/**
 * SCI0(と同じモジュール)の送信制御を行う。
 *
 * @param entry SCIエントリ
 * @param data 送信データ
 * @param len 送信データ長[byte]
 * @return 成功した場合には送信したバイト数が返る。
 *         エラーが発生した場合には-1が返る。
 */
static int
sci0_send(struct sci0_entry *entry, const uint8_t *data, uint16_t len)
{
    RXREG struct st_sci0 *sci = entry->sci;
    uint16_t send_bytes;
    const uint8_t *p;
    uint8_t d;

    send_bytes = 0;
    p = data;
    while (send_bytes < len) {
        if (!fifo_has_blank(&(entry->tx_fifo))) {
            /* 送信バッファに空きがない */
            break;
        }
        fifo_put(&(entry->tx_fifo), *p);
        send_bytes++;
        p++;
    }
    if (sci->SCR.BIT.TIE == 0) {
        /* 先頭バイトを書き込んでスタートさせる。 */
        d = fifo_get(&(entry->tx_fifo));
        sci->SCR.BIT.TIE = 1;
        sci->TDR = d;
    }

    return send_bytes;
}
/**
 * SCI0(と同じモジュール)の受信制御を行う。
 * 関数呼び出し時点で、受信済みのデータを取得して返す。
 * 所定のバイト数受信するような待機は行わない。
 *
 * @param entry SCIエントリ
 * @param buf 受信バッファ
 * @param bufsize 受信バッファサイズ[byte]
 * @return 成功した場合には受信したバイト数が返る。
 *         エラーが発生した場合には-1が返る。
 */
static int
sci0_recv(struct sci0_entry *entry, uint8_t *buf, uint16_t bufsize)
{
    RXREG struct st_sci0 *sci = entry->sci;
    uint16_t recv_bytes;
    uint8_t *p;

    recv_bytes = 0;
    p = buf;
    while (recv_bytes < bufsize) {
        if (!fifo_has_data(&(entry->rx_fifo))) {
            /* 受信データがない */
            break;
        }
        *p = fifo_get(&(entry->rx_fifo));
        p++;
        recv_bytes++;
    }
    if ((sci->SCR.BIT.RIE == 0) && fifo_has_blank(&(entry->rx_fifo))) {
        sci->SCR.BIT.RIE = 1; /* 空きができたので割り込み許可 */
    }

    return recv_bytes;
}

/**
 * SCI0(と同じモジュール)の送信割り込みを処理する。
 *
 * @param entry SCIエントリ
 */
static void
sci0_tx_intr_handler(struct sci0_entry *entry)
{
    RXREG struct st_sci0 *sci = entry->sci;
    uint8_t d;

    if (fifo_has_data(&(entry->tx_fifo))) {
        d = fifo_get(&(entry->tx_fifo));
        sci->TDR = d;
    } else {
        /* 送信可能なデータがないため、割り込み停止する。 */
        sci->SCR.BIT.TIE = 0;
    }
    return;
}

/**
 * SCI0(と同じモジュール)の受信割り込みを処理する。
 *
 * @param entry SCIエントリ
 */
static void
sci0_rx_intr_handler(struct sci0_entry *entry)
{
    RXREG struct st_sci0 *sci = entry->sci;
    uint8_t d;

    d = sci->RDR;
    fifo_put(&(entry->rx_fifo), d); /* 空きがないと読み捨てになる */
    if (fifo_has_blank(&(entry->rx_fifo))) {
        sci->SCR.BIT.RIE = 0; /* 受信バッファがいっぱいなので、RXI/EXI割り込み禁止 */
    }

    return;
}

/* 割り込みハンドラ */
#pragma interrupt(INT_Excep_SCI1_TXI1(vect=VECT(SCI1, TXI1)))
void
INT_Excep_SCI1_TXI1(void)
{
    sci0_tx_intr_handler(&Sci1);
}
#pragma interrupt(INT_Excep_SCI1_RXI1(vect=VECT(SCI1, RXI1)))
void
INT_Excep_SCI1_RXI1(void)
{
    sci0_rx_intr_handler(&Sci1);
}
#pragma interrupt(INT_Excep_SCI5_TXI5(vect=VECT(SCI5, TXI5)))
void
INT_Excep_SCI5_TXI5(void)
{
    sci0_tx_intr_handler(&Sci5);
}
#pragma interrupt(INT_Excep_SCI5_RXI5(vect=VECT(SCI5, RXI5)))
void
INT_Excep_SCI5_RXI5(void)
{
    sci0_rx_intr_handler(&Sci5);
}

#pragma interrupt(INT_Excep_SCI6_TXI6(vect=VECT(SCI6, TXI6)))
void
INT_Excep_SCI6_TXI6(void)
{
    sci0_tx_intr_handler(&Sci6);
}
#pragma interrupt(INT_Excep_SCI6_RXI6(vect=VECT(SCI6, RXI6)))
void
INT_Excep_SCI6_RXI6(void)
{
    sci0_rx_intr_handler(&Sci6);
}
