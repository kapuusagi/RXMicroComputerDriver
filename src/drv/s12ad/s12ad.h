#ifndef DRV_S12AD_H_
#define DRV_S12AD_H_

#include "../../rx_utils/rx_types.h"

enum {
	AD_CHANNEL_1 = 1,
	AD_CHANNEL_2,
	AD_CHANNEL_3,
	AD_CHANNEL_4,
	AD_CHANNEL_5,
	AD_CHANNEL_6
};

enum {
    AD_DIAG_LOW = 1,
    AD_DIAG_MID = 2,
    AD_DIAG_HIGH = 3
};

void drv_s12ad_init(void);
void drv_s12ad_destroy(void);

void drv_s12ad_start_normal(void);
void drv_s12ad_start_diag(uint8_t ad_diag);
void drv_s12ad_request_update_temp(void);
void drv_s12ad_request_update_internal_base(void);
int drv_s12ad_is_busy(void);
void drv_s12ad_update(void);

uint16_t drv_s12ad_get_data(uint8_t ch);


#endif /* DRV_S12AD_H_ */
