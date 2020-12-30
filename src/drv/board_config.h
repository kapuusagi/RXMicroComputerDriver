/**
 * @file ボード固有定義。
 * @author 
 * @note
 * BSPとかにまとめた方がいいのかもしれない。
 */

#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

#define PLL_CLOCK 240000000 /* 24MHz x 10 = 240MHz */
#define PCLKA_CLOCK 120000000 /* PLL_CLOCK / 2 = 120MHz */
#define PCLKB_CLOCK 60000000 /* PLL_CLOCK / 4 = 60MHz */
#define FCLK_CLOCK 60000000 /* PLL_CLOCK / 4 = 60MHz */


#endif /* DRV_BOARD_CONFIG_H_ */
