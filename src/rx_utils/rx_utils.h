#ifndef RX_UTILS_H
#define RX_UTILS_H

#include <macro.h>
#include <_h_c_lib.h>

#include "rx_types.h"

// NOP命令
extern void _builtin_nop(void);
#define nop() _builtin_nop()

// 割り込み許可ビットを設定し、割り込みを有効にする。
extern void           _builtin_setpsw_i(void);
#define InterruptEnable() _builtin_setpsw_i()

// 割り込み許可ビットをクリアし、割り込みを禁止する。
extern void           _builtin_clrpsw_i(void);
#define InterruptDisable() _builtin_clrpsw_i()




#endif /* RX_UTILS_H */




