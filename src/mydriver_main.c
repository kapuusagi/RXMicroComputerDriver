
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
#include "drv/board.h"
#include "drv/port/drv_port.h"
#include "drv/cmt/cmt.h"
#include "rx_utils/rx_utils.h"

void main(void);
#ifdef __cplusplus
extern "C" {
void abort(void);
}
#endif

static void
timer_task(void *arg)
{
	uint8_t d;

	drv_port_update_input();
	d = drv_port_read(PORT_NO_DEBUG_LED_1);
	drv_port_write(PORT_NO_DEBUG_LED_1, !d);
	drv_port_update_output();
}

void
main(void)
{
	board_init_on_reset();
	drv_port_init();
	drv_cmt_init();

	drv_cmt_start(TIMER_NO_1, 5000, timer_task, 0);

    while(1) {
    	nop();
    }

}

#ifdef __cplusplus
void abort(void)
{

}
#endif
