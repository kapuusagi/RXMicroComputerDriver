#ifndef DRV_S12AD_H_
#define DRV_S12AD_H_

#include "../../rx_utils/rx_types.h"

enum {
	AD_CHANNEL_1 = 1,
};

/**
 * 12bit A/Dコンバータドライバ
 */
void drv_s12ad_init(void);
void drv_s12ad_start(uint8_t channnel_no);
void drv_s12ad_stop(uint8_t channel_no);
void drv_s12ad_update(void);
uint16_t drv_s12ad_get(uint8_t channel_no);


#endif /* DRV_S12AD_H_ */
