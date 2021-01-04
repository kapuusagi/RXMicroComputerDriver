
#include <iodefine.h>
#include <machine.h>
#include "drv/board.h"
#include "drv/port/port.h"
#include "drv/sci/sci.h"
#include "drv/cmt/cmt.h"
#include "drv/s12ad/s12ad.h"
#include "rx_utils/rx_utils.h"

#include "os/kernel.h"
#include "os/kernel_api.h"

static struct semaphore Sem;
static struct mutex Mutex;

static stack_type_t Task1Stack[128];
static stack_type_t Task2Stack[128];
static stack_type_t Task3Stack[128];
static int Task1Id = 0;
static int Task2Id = 0;


static void
task1(void *arg)
{
	mutex_lock(&Mutex);
	const char *name = (const char*)(arg);
	for (int i = 0; i < 10; i++) {
		sem_wait(&Sem);
		rx_debug("%s\n", name);
	}
	mutex_unlock(&Mutex);

	return ;
}

static void
task2(void *arg)
{
	const char *name = (const char*)(arg);

	mutex_lock(&Mutex);
	for (int i = 0; i < 5; i++) {
		sem_wait(&Sem);
		rx_debug("%s\n", name);
	}
	mutex_unlock(&Mutex);

	return ;
}

static void
task3(void *arg)
{
	const char *name = (const char*)(arg);
	rx_debug("%s\n", name);

	while (kernel_task_is_alive(Task1Id) || kernel_task_is_alive(Task2Id)) {
		sleep(1000);
		rx_debug("%s post one.\n", name);
		sem_post(&Sem);
	}

	rx_debug("%s exit.\n", name);
}

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

	kernel_init();

	drv_s12ad_start_normal();

//	drv_cmt_set_msec_handler(kernel_request_swtich);
	drv_cmt_start(TIMER_1, 1000, timer_task);
	drv_cmt_start(TIMER_2, 10, timer_task_10ms);

	rx_debug("-----------------------\n");
	drv_cmt_delay_ms(1000);
	while (1) {
		sem_init(&Sem, 0);
		mutex_init(&Mutex);

		Task1Id = kernel_register_task(11, task1, "task1", Task1Stack, sizeof(Task1Stack));
		Task2Id = kernel_register_task(10, task2, "task2", Task2Stack, sizeof(Task2Stack));
		kernel_register_task(12, task3, "task3", Task3Stack, sizeof(Task3Stack));

		kernel_start_scheduler();

		sem_destroy(&Sem);

		rx_debug("-----------------------\n");
		drv_cmt_delay_ms(4000);
	}

}

