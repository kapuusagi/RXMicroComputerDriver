/**
 * @file RXタイプ定義
 * @author
 */
#ifndef RX_TYPES_H
#define RX_TYPES_H

#include <typedefine.h>
#include <stddef.h>

typedef _SBYTE int8_t;
typedef _UBYTE uint8_t;
typedef _SWORD int16_t;
typedef _UWORD uint16_t;
typedef _SDWORD int32_t;
typedef _UDWORD uint32_t;
typedef _SQWORD int64_t;
typedef _UQWORD uint64_t;

#ifndef _SIZE_T
typedef _UDWORD size_t;
#endif

/* RXREG定義
 *   gccの場合にはvolatileだけで良さそう。
 *   ccrxの場合には __evenaccess を付けないと、レジスタへのビットアクセスがうまくいかない。（たぶん命令が違う)
 */
#define RXREG  volatile __evenaccess



#endif /* RX_TYPES_H */
