/**
 * @file ソフトウェアFIFOモジュール
 * @author
 */

#ifndef DRV_UART_FIFO_H_
#define DRV_UART_FIFO_H_

#include "../../rx_utils/rx_types.h"

/**
 * FIFO
 */
struct fifo {
	uint8_t buf[512]; // Buffer
	uint16_t out; // Read out
	uint16_t in; // Write in
};

void fifo_init(struct fifo *f);
void fifo_destroy(struct fifo *f);

void fifo_put(struct fifo *f, uint8_t d);
uint8_t fifo_get(struct fifo *f);
uint8_t fifo_has_data(const struct fifo *f);
uint8_t fifo_has_blank(const struct fifo *f);
uint16_t fifo_get_data_count(const struct fifo *f);

#endif /* DRV_UART_FIFO_H_ */
