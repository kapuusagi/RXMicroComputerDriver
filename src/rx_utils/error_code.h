/**
 * @file エラーコード定義
 * @author 
 */

#ifndef RX_UTILS_ERROR_CODE_H_
#define RX_UTILS_ERROR_CODE_H_

#define ERR_INVAL               1    /* 不正引数 */
#define ERR_OPERATION_STATE     2    /* 不正なステート */
#define ERR_NODEV               3    /* デバイスが存在しないか応答無し */
#define ERR_IO                  4    /* IO エラー */
#define ERR_NOMEM               5    /* メモリなしエラー */
#define ERR_NOT_OWNER           6    /* オブジェクトの所有権が無い */

/* ドライバ固有エラー */
#define ERR_DRV_FLASH_TIMEOUT   1000 /* 操作がタイムアウトした */
#define ERR_DRV_FLASH_WRITELOCK 1001 /* Program/Eraseロックアドレスが指定された */


#endif /* RX_UTILS_ERROR_CODE_H_ */
