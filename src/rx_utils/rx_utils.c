/**
 * @file RXユーティリティ
 * @author
 */

#include <stdarg.h>
#include "rx_utils.h"

/* Note:ハードウェアに併せてインクルードを変更する */
#include "../drv/sci/sci.h"

struct fmt_info {
	char type; /* 書式文字 ('c' 'x' 'd'など。 %後にあらわれた文字が格納される。 */
	uint8_t field_width; /* フィールド幅 */
	uint8_t decimal_digit; /* 小数点桁数 */
	uint8_t is_zero_fill :1; /* ゼロ埋めするかどうか。(0:スペース, 1:ゼロで埋める) */
	uint8_t rsvd :7;
};

static int rx_vsnprintf(char *buf, uint16_t bufsize, const char *fmt, va_list ap);
static int parse_format(const char **pfmt, struct fmt_info *info);
static int print_int32(char *pbuf, uint16_t left, int32_t value,
		const struct fmt_info *info);
static int print_uint32(char *pbuf, uint16_t left, uint32_t value,
        const struct fmt_info *info);
static int print_hex(char *pbuf, uint16_t left, uint32_t value, char base,
		const struct fmt_info *info);
static int print_real(char *pbuf, uint16_t left, double value,
		const struct fmt_info *info);
static int print_string(char *pbuf, uint16_t left, const char *str,
		const struct fmt_info *info);

/**
 * 書式文字列をバッファに書き出す。
 * 成功した場合、必ずNULL終端したことが保証される。
 *
 * @param buf バッファ
 * @param bufsize バッファサイズ
 * @param fmt 書式
 * @return 成功した場合、書き込んだバイト数が返る。
 *         buf, bufsize, fmtに不正な値が渡された場合や、
 *         書式が解析できないものであった場合には-1が返る。
 */
int
rx_snprintf(char *buf, uint16_t bufsize, const char *fmt, ...)
{
	va_list ap;
	int retval;

	if ((buf == NULL ) || (bufsize == 0) || (fmt == NULL )) {
		return -1;
	}

	va_start(ap, fmt);
	retval = rx_vsnprintf(buf, bufsize, fmt, ap);
	va_end(ap);

    return retval;
}
/**
 * RXマイコン用 vsnprintf 実装
 * Note: ヘッダファイルにstdargを入れたくないのでstatic関数とした。
 *
 * @param buf バッファ
 * @param bufsize バッファサイズ
 * @param fmt 書式文字列
 * @param ap 引数リスト
 * @return 成功した場合、書き込んだバイト数が返る。
 *         buf, bufsize, fmtに不正な値が渡された場合や、
 *         書式が解析できないものであった場合には-1が返る。
 */
static int
rx_vsnprintf(char *buf, uint16_t bufsize, const char *fmt, va_list ap)
{
    const char *pfmt;
    char *pbuf;
    uint16_t left;
    struct fmt_info info;
    int n;

    pfmt = fmt;
    pbuf = buf;
    left = bufsize;

    while ((*pfmt != '\0') && ((left - 1) > 0)) {
        if (*pfmt != '%') {
            *pbuf = *pfmt;
            pbuf++;
            left--;
            pfmt++;
        } else {
            if (parse_format(&pfmt, &info) != 0) {
                return -1;
            }
            switch (info.type) {
            case 'i':
            case 'd':
                n = print_int32(pbuf, left, va_arg(ap, int32_t), &info);
                break;
            case 'u':
                n = print_uint32(pbuf, left, va_arg(ap, uint32_t), &info);
                break;
            case 'x':
                n = print_hex(pbuf, left, va_arg(ap, uint32_t), 'a', &info);
                break;
            case 'X':
                n = print_hex(pbuf, left, va_arg(ap, uint32_t), 'A', &info);
                break;
            case 'f':
                n = print_real(pbuf, left, va_arg(ap, double), &info);
                break;
            case 's':
                n = print_string(pbuf, left, va_arg(ap, const char *), &info);
                break;
            case 'c':
                *pbuf = va_arg(ap, int);
                n = 1;
                break;
            default:
                /* 不明書式の場合にはそのまま書式指定文字を書き出す。
                 * %%のようなケースはここで処理される。 */
                *pbuf = info.type;
                n = 1;
                break;
            }
            if (n >= 0) {
                pbuf += n;
                left -= n;
            }
        }
    }

    *pbuf = '\0';

    return (bufsize - left);
}

/**
 * %で指定された書式を解析する。
 * この関数は書式を解析すると、pfmtで指定されるポインタを次の字句まで進める。
 *
 * @param pfmt 書式文字列のポインタ
 * @param info 書式情報を格納する構造体
 * @return 解析に成功した場合には0が返る。
 *         失敗した場合には-1が返る。
 */
static int
parse_format(const char **pfmt, struct fmt_info *info)
{
	const char *ptr = *pfmt;

	if (*ptr != '%') {
		return -1; /* %で始まっていない。 */
	}
	ptr++; /* skip '%' */

	if (*ptr == '\0') {
		return -1;
	}

	if (*ptr == '0') {
		/* 0 fill */
		info->is_zero_fill = 1;
		ptr++;
		if (*ptr == '\0') {
			return -1;
		}
	} else {
		info->is_zero_fill = 0;
	}

	if ((*ptr >= '0') && (*ptr <= '9')) {
		/* Specify field width. */
		info->field_width = *ptr - '0';
		ptr++;
		if (*ptr == '\0') {
			return -1;
		}
	} else {
		info->field_width = 0;
	}

	if (*ptr == '.') {
		ptr++;
		if (*ptr == '\0') {
			return -1;
		}
		if ((*ptr >= '0') && (*ptr <= '9')) {
			info->decimal_digit = *ptr - '0';
			ptr++;
			if (*ptr == '\0') {
				return -1;
			}
		} else {
			info->decimal_digit = 4;
		}
	} else {
		info->decimal_digit = 4;
	}

	info->type = *ptr;
	ptr++;
	(*pfmt) = ptr;

	return 0;
}

/**
 * 32bit 整数書式を書き込む。
 *
 * @param pbuf バッファ
 * @param left バッファサイズ
 * @param value 書き込む値
 * @param info 書式情報
 * @return 書き込んだバイト数が返る。
 */
static int
print_int32(char *pbuf, uint16_t left, int32_t value,
		const struct fmt_info *info)
{
	char tmp[16]; /* This buffer has enough size to display integer. */
	char *p = tmp + sizeof(tmp) - 1;
	int32_t d;
	int32_t quotient;
	uint8_t field_width;
	uint8_t is_negative;
	int retval;

	if (value < 0) {
		is_negative = 1;
	} else {
		is_negative = 0;
	}

	*p = '\0';

	field_width = 0;
	do {
		quotient = value / 10;
		d = value - quotient * 10;

		p--;
		if (d >= 0) {
			*p = '0' + d;
		} else {
			*p = '0' - d;
		}
		field_width++;

		value = quotient;
	} while (value != 0);

	if (is_negative) {
		p--;
		(*p) = '-';
		field_width++;
	}

	while (field_width < info->field_width) {
		p--;
		(*p) = (info->is_zero_fill) ? '0' : ' ';
		field_width++;
	}

	if ((field_width + 1) <= left) {
		/* Copyable with NULL character. */
		rx_memcpy(pbuf, p, field_width + 1); /* include NULL character. */
		retval = field_width;
	} else {
		/* Do not enough buffer. */
		rx_memcpy(pbuf, p, left - 1);
		pbuf[left - 1] = '\0';
		retval = left - 1;
	}

	return retval;
}
/**
 * 32bit 整数書式を書き込む。
 *
 * @param pbuf バッファ
 * @param left バッファサイズ
 * @param value 書き込む値
 * @param info 書式情報
 * @return 書き込んだバイト数が返る。
 */
static int
print_uint32(char *pbuf, uint16_t left, uint32_t value,
        const struct fmt_info *info)
{
    char tmp[16]; /* This buffer has enough size to display integer. */
    char *p = tmp + sizeof(tmp) - 1;
    uint32_t d;
    uint32_t quotient;
    uint8_t field_width;
    int retval;

    *p = '\0';

    field_width = 0;
    do {
        quotient = value / 10;
        d = value - quotient * 10;

        p--;
        *p = '0' + d;
        field_width++;

        value = quotient;
    } while (value != 0);

    while (field_width < info->field_width) {
        p--;
        (*p) = (info->is_zero_fill) ? '0' : ' ';
        field_width++;
    }

    if ((field_width + 1) <= left) {
        /* Copyable with NULL character. */
        rx_memcpy(pbuf, p, field_width + 1); /* include NULL character. */
        retval = field_width;
    } else {
        /* Do not enough buffer. */
        rx_memcpy(pbuf, p, left - 1);
        pbuf[left - 1] = '\0';
        retval = left - 1;
    }

    return retval;
}


/**
 * 16進数表現で書き込む
 *
 * @param pbuf バッファ
 * @param left バッファサイズ
 * @param value 書き込む値
 * @param base 0xAに相当する文字
 * @param info 書式情報
 * @return 書き込んだバイト数が返る。
 */
static int
print_hex(char *pbuf, uint16_t left, uint32_t value, char base,
		const struct fmt_info *info)
{
	char tmp[16]; /* This buffer has enough size to display integer. */
	char *p = tmp + sizeof(tmp) - 1;
	uint32_t d;
	uint32_t quotient;
	uint8_t field_width;
	int retval;

	*p = '\0';

	field_width = 0;
	do {
		quotient = value >> 4;
		d = value - (quotient << 4);

		p--;
		if (d < 10) {
			*p = '0' + d;
		} else {
			*p = base + (d - 10);
		}
		field_width++;

		value = quotient;
	} while (value != 0);

	while (field_width < info->field_width) {
		p--;
		(*p) = (info->is_zero_fill) ? '0' : ' ';
		field_width++;
	}

	if ((field_width + 1) <= left) {
		/* Copyable with NULL character. */
		rx_memcpy(pbuf, p, field_width + 1); /* include NULL character. */
		retval = field_width;
	} else {
		/* Do not enough buffer. */
		rx_memcpy(pbuf, p, left - 1);
		pbuf[left - 1] = '\0';
		retval = left - 1;
	}

	return retval;
}

/**
 * 実数を書き込む。
 *
 * @param pbuf バッファ
 * @param left バッファサイズ
 * @param value 書き込む値
 * @param info 書式情報
 * @return 書き込んだバイト数が返る。
 */
static int
print_real(char *pbuf, uint16_t left, double value,
		const struct fmt_info *info)
{
	char tmp[32]; /* This buffer has enough size to display integer. */
	char *p;
	double decimal_part;
	double product;
	double round_off;
	int32_t integer_part;
	int32_t d;
	int32_t quotient;
	uint8_t field_width;
	uint8_t is_negative;
	uint8_t decimal_digit;
	int retval;

	if (value < 0) {
		is_negative = 1;
	} else {
		is_negative = 0;
	}

	/* 四捨五入の処理 */
    round_off = 0.5;
    decimal_digit = info->decimal_digit;
    while (decimal_digit > 0) {
        round_off /= 10;
        decimal_digit--;
    }
    value += round_off;

	integer_part = (int32_t) (value);
	decimal_part = value - integer_part;
	field_width = 0;

	/* 小数部を文字列化 */
	p = tmp + sizeof(tmp) / 2 - 1;
	decimal_digit = info->decimal_digit;
	if (decimal_digit > 0) {
		*p = '.';
		p++;
		field_width++;
		do {
			product = decimal_part * 10;
			d = (int) (product);

			*p = (d >= 0) ? ('0' + d) : ('0' - d);
			p++;
			field_width++;

			decimal_part = product - d;
			decimal_digit--;
		} while (decimal_digit > 0);
	}
	*p = '\0';

	/* 整数部を文字列化 */
	p = tmp + (sizeof(tmp) / 2) - 1;

	do {
		quotient = integer_part / 10;
		d = integer_part - quotient * 10;

		p--;
		*p = (d >= 0) ? ('0' + d) : ('0' - d);
		field_width++;

		integer_part = quotient;
	} while (integer_part != 0);
	if (is_negative) {
		p--;
		(*p) = '-';
		field_width++;
	}

	/* フィールドをスペースで埋める */
	while (field_width < info->field_width) {
		p--;
		*p = ' ';
		field_width++;
	}

	if ((field_width + 1) <= left) {
		/* NULL文字も含めてコピーできる */
		rx_memcpy(pbuf, p, field_width + 1); /* include NULL character. */
		retval = field_width;
	} else {
		/* 十分なバッファサイズがない。 */
		rx_memcpy(pbuf, p, left - 1);
		pbuf[left - 1] = '\0';
		retval = left - 1;
	}

	return retval;
}

/**
 * 文字列を書き込む。
 *
 * @param pbuf バッファ。
 * @param left バッファサイズ
 * @param str 書き込む文字列
 * @param info Printing 書式
 * @return 書き込んだバイト数が返る。
 */
static int
print_string(char *pbuf, uint16_t left, const char *str,
		const struct fmt_info *info)
{
	uint16_t len;
	const char *p;
	int retval;

	len = 0;
	for (p = str; *p != '\0'; p++) {
		len++;
	}

	if ((len + 1) <= left) {
		/* Copyable NULL character. */
		rx_memcpy(pbuf, str, len + 1); /* include NULL character. */
		retval = len;
	} else {
		rx_memcpy(pbuf, str, left - 1);
		pbuf[left - 1] = '\0';
		retval = left - 1;
	}
	return retval;
}

/**
 * dstで指定される文字列バッファの末尾にsrcで指定される文字列を追記する。
 * 空き領域がない場合や、引数が不正な場合には0を返してコピーは行われない。
 * POSIX strncatと同様、バッファのサイズが十分でない場合にはNULL終端されないことに注意すること。
 *
 * @param dst コピー先文字列バッファ。NULL終端済みであること。
 * @param src コピー元文字列
 * @param bufsize コピー先のバッファサイズ
 * @return コピーしたバイト数が返る。
 */
int
rx_strncat(char *dst, const char *src, uint32_t bufsize)
{
	uint32_t src_len;
	uint32_t dst_len;
	uint32_t left_size;
	uint32_t copy_bytes;
	uint32_t i;
	char *dstp;
	const char *srcp;

	if ((dst == NULL ) || (src == NULL ) || (bufsize == 0)) {
		return 0;
	}

	src_len = rx_strlen(src);
	dst_len = rx_strlen(dst);

	if ((src_len >= bufsize) || (src_len == 0)) {
		return 0;
	}

	left_size = bufsize - src_len; /* 空きバイト数 */
	copy_bytes = (left_size > src_len) ? src_len : left_size;
	dstp = dst + dst_len;
	srcp = src;
	for (i = 0; i < copy_bytes; i++) {
		*dstp = *srcp;
		dstp++;
		srcp++;
	}
	if (copy_bytes < left_size) {
		(*dstp) = '\0'; /* 十分なサイズがあるならばNULL終端させる。 */
	}

	return copy_bytes;
}

/**
 * s の文字列長を取得する。
 * @param s 文字列
 * @return 文字列長が返る。
 *         sにNULLを指定した場合には0が返る。
 */
uint32_t
rx_strlen(const char *s)
{
	uint32_t len;

	if (s == NULL) {
		return 0;
	}

	len = 0;
	while (*s != '\0') {
		s++;
		len++;
	}
	return len;
}

/**
 * デバッグ出力する。
 * 本関数はプラットフォームに依存する。
 *
 * @param fmt 書式文字列
 */
void
rx_debug(const char *fmt, ...)
{
	char msgbuf[256];
	va_list ap;

	if ((fmt == NULL) || (fmt[0] == '\0')) {
		return;
	}

	va_start(ap, fmt);
    rx_vsnprintf(msgbuf, 256, fmt, ap);
#ifdef EMULATOR
    OutputDebugStringA((msg))
#else
    uint8_t is_interrupt_enable = rx_util_is_interrupt_enable();
    if (is_interrupt_enable) {
        /* デバッグデータストア中に割り込まれるとおかしくなるので割り込み禁止 */
        rx_util_disable_interrupt();
    }
    drv_sci_send(SCI_CH_DEBUG, (const uint8_t*)(msgbuf), rx_strlen(msgbuf));
    if (is_interrupt_enable) {
        /* 割り込み許可を戻す */
        rx_util_enable_interrupt();
    }
#endif
	va_end(ap);
}



/**
 * sで指定されるポインタをnバイト分、cの値にセットする。
 *
 * @param s 領域先頭のポインタ
 * @param c 設定値
 * @param n バイト数
 * @return sが返る。
 */
void *
rx_memset(void *s, int c, size_t n)
{
	if (s != NULL) {
	    uint64_t *pqw = s;
	    uint64_t qword = 0;
	    for (int i = 0; i < 8; i++) {
	        qword <<= 8;
	        qword |= (uint8_t)(c & 0xff);
	    }

	    while (n > 7) {
	        (*pqw) = qword;
	        pqw++;
	        n -= 8;
	    }
	    register uint8_t *pb = (uint8_t*)(pqw);
	    while (n > 0) {
	        (*pb) = (uint8_t)(c);
	        pb++;
	        n--;
	    }
	}

	return s;
}

/**
 * メモリの一致判定をする。
 *
 * @param b1 領域1
 * @param b2 領域2
 * @param n 比較するバイト数[byte]
 * @return 一致している場合には0, 不一致の場合には非ゼロの値が返る。
 */
int
rx_memcmp(const void *b1, const void *b2, size_t n)
{
    if ((b1 != b2) && (b1 != NULL) && (b2 != NULL)) {
        const uint64_t *pqw1 = (const uint64_t*)(b1);
        const uint64_t *pqw2 = (const uint64_t*)(b2);
        while (n > 7) {
            if (*pqw1 != *pqw2) {
                return (*pqw1 > *pqw2) ? 1 : -1;
            }
            n -= 8;
            pqw1++;
            pqw2++;
        }

        const uint8_t *pb1 = (const uint8_t*)(pqw1);
        const uint8_t *pb2 = (const uint8_t*)(pqw2);
        while (n > 0) {
            if (*pb1 != *pb2) {
                return (*pb1 > *pb2) ? 1 : -1;
            }
            n--;
            pb1++;
            pb2++;
        }
    }

    return 0;
}

/**
 * srcの先頭 n バイトを dest にコピーする。
 * コピー元の領域とコピー先の領域が重なってはならない。
 *
 * @param dest コピー先
 * @param src コピー元
 * @param n コピーするサイズ
 * @return destが返る。
 */
void *
rx_memcpy(void *dest, const void *src, size_t n)
{
    if ((src != NULL) && (dest != NULL)) {
        const uint64_t *pqw_src = (const uint64_t*)(src);
        uint64_t *pqw_dest = (uint64_t*)(dest);
        while (n > 7) {
            *pqw_dest = *pqw_src;
            n -= 8;
            pqw_src++;
            pqw_dest++;
        }

        const uint8_t *pb_src = (const uint8_t*)(pqw_src);
        uint8_t *pb_dest = (uint8_t*)(pqw_dest);
        while (n > 0) {
            *pb_dest = *pb_src;
            n--;
            pb_src++;
            pb_dest++;
        }
    }

    return dest;
}

