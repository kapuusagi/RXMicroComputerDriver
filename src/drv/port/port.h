#ifndef DRV_PORT_H
#define DRV_PORT_H

#include "../../rx_utils/rx_types.h"

enum {
	/* 出力ポート */
	PORT_NO_DEBUG_LED_1 = 0,

	/* 入力ポート */
	PORT_NO_USER_SW_1,
	PORT_NO_BTN_UP,
	PORT_NO_BTN_DOWN,
	PORT_NO_BTN_LEFT,
	PORT_NO_BTN_RIGHT,
	PORT_NO_BTN_CENTER,
};

void drv_port_init(void);

void drv_port_update_input(void);
void drv_port_update_output(void);

void drv_port_write(uint8_t port_no, uint8_t on);
uint8_t drv_port_read(uint8_t port_no);


#endif /* DRV_PORT_H */
