
/***********************************************************************/
/*                                                                     */
/*  FILE        : Main.c                                   */
/*  DATE        :Tue, Oct 31, 2006                                     */
/*  DESCRIPTION :Main Program                                          */
/*  CPU TYPE    :                                                      */
/*                                                                     */
/*  NOTE:THIS IS A TYPICAL EXAMPLE.                                    */
/*                                                                     */
/***********************************************************************/
//#include "typedefine.h"
#ifdef __cplusplus
//#include <ios>                        // Remove the comment when you use ios
//_SINT ios_base::Init::init_cnt;       // Remove the comment when you use ios
#endif

#include <iodefine.h>
#include <machine.h>
#include "drv/board.h"
#include "drv/port/port.h"
#include "drv/cmt/cmt.h"
#include "drv/s12ad/s12ad.h"
#include "rx_utils/rx_utils.h"

void main(void);
#ifdef __cplusplus
extern "C" {
void abort(void);
}
#endif

static void
timer_task(uint32_t elapse_millis)
{
	uint8_t d;

	drv_port_update_input();
	d = drv_port_read(PORT_NO_DEBUG_LED_1);
	drv_port_write(PORT_NO_DEBUG_LED_1, !d);
	drv_port_update_output();
}

static void
timer_task_1ms(uint32_t elapse_millis)
{
	drv_s12ad_update();
}

void
main(void)
{
	board_init_on_reset();
	drv_port_init();
	drv_cmt_init();
	drv_s12ad_init();

	drv_s12ad_start(AD_CHANNEL_1);

	drv_cmt_start(TIMER_1, 5000, timer_task);
	drv_cmt_start(TIMER_2, 1, timer_task_1ms);

	rx_util_wait();

}

#ifdef __cplusplus
void abort(void)
{

}
#endif
