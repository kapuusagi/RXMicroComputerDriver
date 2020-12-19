#ifndef RX_UTILS_H
#define RX_UTILS_H

#include "rx_types.h"

#define RX_UTIL_SET_INPUT_IO_PORT(__port__, __bit__) \
	(__port__).PDR.BIT.__bit__ = 0; \
	(__port__).PMR.BIT.__bit__ = 0;

#define RX_UTIL_SET_INPUT_FUNC_PORT(__port__, __bit__) \
	(__port__).PDR.BIT.__bit__ = 0; \
	(__port__).PMR.BIT.__bit__ = 1;

#define RX_UTIL_SET_OUTPUT_IO_PORT(__port__, __bit__, __initial__) \
	(__port__).PODR.BIT.__bit__ = (__initial__);\
	(__port__).PDR.BIT.__bit__ = 1; \
	(__port__).PMR.BIT.__bit__ = 0;

#define RX_UTIL_SET_OUTPUT_FUNC_PORT(__port__, __bit__) \
	(__port__).PODR.BIT.__bit__ = 0; \
	(__port__).PDR.BIT.__bit__ = 1; \
	(__port__).PMR.BIT.__bit__ = 1;

#ifdef __cplusplus
extern "C" {
#endif

void rx_util_nop(void);
void rx_util_wait(void);

void *rx_memset(void *buf, int d, uint32_t n);


#ifdef __cplusplus
}
#endif






#endif /* RX_UTILS_H */




