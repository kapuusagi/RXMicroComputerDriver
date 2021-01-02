/**
 * @file RXマイコン CPU回りのユーティリティ
 * @author 
 */
#include "rx_utils_cpu.h"


#define CPU_PSW_I  (1 << 16)
#define CPU_PSW_U  (1 << 17)


/**
 * 割り込みが許可されているかどうかを取得する。
 *
 * @return 割り込み許可されている場合には1, 禁止されている場合には0
 */
int
rx_util_is_interrupt_enable(void)
{
    long reg;

    reg = rx_util_get_psw();

    return ((reg & CPU_PSW_I) != 0) ? 1 : 0;
}


/**
 * IPL(割り込みレベル)を取得する。
 *
 * @return 割り込みレベル。(0～15)
 */
int
rx_util_get_ipl(void)
{
    uint32_t psw_reg = rx_util_get_psw();
    return (psw_reg >> 24) & 0xf;
}


/**
 * ユーザーモードかどうかを取得する。
 * @return ユーザーモードの場合には非ゼロの値、スーパーバイザーモードの場合には0
 */
int
rx_util_is_user_mode(void)
{
    uint32_t psw_reg = rx_util_get_psw();
    return ((psw_reg & CPU_PSW_U) != 0) ? 1 : 0;
}


