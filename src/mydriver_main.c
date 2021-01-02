
#include <iodefine.h>
#include <machine.h>
#include "drv/board.h"
#include "drv/port/port.h"
#include "drv/sci/sci.h"
#include "drv/cmt/cmt.h"
#include "drv/s12ad/s12ad.h"
#include "rx_utils/rx_utils.h"

static void
timer_task(uint32_t elapse_millis)
{
	uint8_t d;

	drv_port_update_input();
	d = drv_port_read(PORT_NO_DEBUG_LED_1);
	drv_port_write(PORT_NO_DEBUG_LED_1, !d);
	drv_port_update_output();

	drv_sci_send(SCI_CH_DEBUG, "debug\n", 6);

}

static void
timer_task_10ms(uint32_t elapse_millis)
{
	drv_s12ad_update();
	if (!drv_s12ad_is_busy()) {
		drv_s12ad_start_normal();
	}
}

void
main(void)
{
	board_init_on_reset();
	drv_port_init();
	drv_s12ad_init();
	drv_cmt_init();
	drv_sci_init();


	drv_s12ad_start_normal();

	drv_cmt_start(TIMER_1, 5000, timer_task);
	drv_cmt_start(TIMER_2, 10, timer_task_10ms);

	while (1) {
		rx_util_wait();
	}

}

