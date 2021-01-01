/**
 * @file RXユーティリティ
 * @author
 */
#ifndef RX_UTILS_H
#define RX_UTILS_H

#include <iodefine.h>
#include "rx_types.h"
#include "rx_utils_cpu.h"

#define RX_UTIL_SET_INPUT_IO_PORT(__port__, __bit__) \
		{ \
			(__port__).PDR.BIT.__bit__ = 0; \
			(__port__).PMR.BIT.__bit__ = 0; \
		}

#define RX_UTIL_SET_INPUT_FUNC_PORT(__port__, __bit__) \
		{ \
			(__port__).PDR.BIT.__bit__ = 0; \
			(__port__).PMR.BIT.__bit__ = 1; \
		}

#define RX_UTIL_SET_OUTPUT_IO_PORT(__port__, __bit__, __initial__) \
		{ \
			(__port__).PODR.BIT.__bit__ = (__initial__);\
			(__port__).PDR.BIT.__bit__ = 1; \
			(__port__).PMR.BIT.__bit__ = 0; \
		}

#define RX_UTIL_SET_OUTPUT_FUNC_PORT(__port__, __bit__) \
		{ \
			(__port__).PODR.BIT.__bit__ = 0; \
			(__port__).PDR.BIT.__bit__ = 1; \
			(__port__).PMR.BIT.__bit__ = 1; \
		}

#define RX_MAX_PRIORITY 15

#ifdef __cplusplus
extern "C" {
#endif

int rx_snprintf(char *buf, uint16_t bufsize, const char *fmt, ...);
uint32_t rx_strlen(const char *s);
int rx_strtol(const char *s, int unit, const char **ptr);
void rx_debug(const char *fmt, ...);
void *rx_memset(void *s, int c, size_t n);
int rx_memcmp(const void *b1, const void *b2, size_t n);
void *rx_memcpy(void *dest, const void *src, size_t n);


#ifdef __cplusplus
}
#endif






#endif /* RX_UTILS_H */




