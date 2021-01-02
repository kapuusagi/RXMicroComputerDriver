/**
 * @file SCI 調歩同期式通信ドライバ
 * @author
 */

#ifndef DRV_SCI_H_
#define DRV_SCI_H_

#include "../../rx_utils/rx_types.h"

enum {
    SCI_CH_DEBUG = 0,
	SCI_CH_1
};



void drv_sci_init(void);
void drv_sci_destroy(void);

int drv_sci_send(uint8_t ch, const uint8_t *data, uint16_t len);
int drv_sci_recv(uint8_t ch, uint8_t *buf, uint16_t bufsize);

#endif /* DRV_SCI_SCI_H_ */
