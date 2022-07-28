/*
 * @Author: jinlu
 * @email: jinlu@tuya.com
 * @LastEditors:
 * @file name: user_flash.h
 * @Description: light production flash read/write include file
 * @Copyright: HANGZHOU TUYA INFORMATION TECHNOLOGY CO.,LTD
 * @Company: http://www.tuya.com
 * @Date: 2019-04-26 13:55:40
 * @LastEditTime: 2019-08-10 13:01:32
 */

#ifndef __USER_FLASH_H__
#define __USER_FLASH_H__

#include "light_types.h"
#include "soc_flash.h"


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#define LIGHT_SCENE_MAX_LENGTH    210

#pragma pack(1)

typedef enum {
    LIGHT_MODE_WHITE = 0,
    LIGHT_MODE_COLOR,
    LIGHT_MODE_SCENE,
    LIGHT_MODE_MUSIC,
    LIGHT_MODE_MAX,
} LIGHT_MODE_FLASH_E;

#if 1
typedef struct {
    USHORT_T usRed;         /* color R setting */
    USHORT_T usGreen;
    USHORT_T usBlue;
} COLOR_RGB_FLASH_T;
#else

typedef struct {
    USHORT_T usHue;         /* color Hue setting */
    USHORT_T usSat;
    USHORT_T usValue;
} COLOR_HSV_FLASH_T;

#endif
typedef struct {
    USHORT_T usHue;
    USHORT_T usSat;
    USHORT_T usValue;
    CHAR_T   cColorStr[13];
} COLORT_ORIGIN_FLASH_T;

/**
 * @brief Light save data structure
 */
typedef struct {
    USHORT_T                usFlashVer;
    BOOL_T                  bPower;
    LIGHT_MODE_FLASH_E      eMode;
    USHORT_T                usBright;
    USHORT_T                usTemper;
    COLOR_RGB_FLASH_T       tColor;
    COLORT_ORIGIN_FLASH_T   tColorOrigin;
    CHAR_T                  cScene[LIGHT_SCENE_MAX_LENGTH + 1];
} LIGHT_APP_DATA_FLASH_T;

/**
 * @brief Light save data structure
 */
typedef enum {
    FUC_TEST1 = 0,
    AGING_TEST,
    FUC_TEST2,
} TEST_MODE_E;

/**
 * @brief Light prod test save structure
 */
typedef struct {
    TEST_MODE_E  eTestMode;
    USHORT_T     usAgingTestedTime;
} LIGHT_PROD_TEST_DATA_FLASH_T;


//@attention!!!

#pragma pack()

#define RESET_CNT_OFFSET          0
#define LIGHT_APP_DATA_OFFSET     (RESET_CNT_OFFSET + 2)
#define PROD_TEST_DATA_OFFSET     (LIGHT_APP_DATA_OFFSET + sizeof(LIGHT_APP_DATA_FLASH_T) + 1)


/**
 * @brief: save light reset cnt
 * @param {IN UCHAR_T ucData -> save reset cnt to flash}
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashWriteResetCnt(IN UCHAR_T ucData);

/**
 * @brief: save light application data
 * @param {IN LIGHT_APP_DATA_FLASH_T *pData -> save data}
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashWriteAppData(IN LIGHT_APP_DATA_FLASH_T *pData);

/**
 * @brief: save light product test data
 * @param {IN LIGHT_PROD_TEST_DATA_FLASH_T *pData -> prod test data}
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashWriteProdData(IN LIGHT_PROD_TEST_DATA_FLASH_T *pData);

/**
 * @brief: save light application data
 * @param {IN USHORT_T usLen -> read oem cfg data len }
 * @param {IN UCHAR_T *pData -> read oem cfg data }
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashWriteOemCfgData(IN USHORT_T usLen, IN UCHAR_T *pData);

/**
 * @brief: read light reset cnt
 * @param {OUT UCHAR_T *data -> reset cnt}
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashReadResetCnt(OUT UCHAR_T *pData);

/**
 * @brief: read light application data
 * @param {IN LIGHT_APP_DATE_FLASH_T *pData -> prod test data}
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashReadAppData(OUT LIGHT_APP_DATA_FLASH_T *pData);

/**
 * @brief: read light prod test data
 * @param {OUT LIGHT_PROD_TEST_DATA_FLASH_T *data -> prod test data}
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashReadProdData(OUT LIGHT_PROD_TEST_DATA_FLASH_T *pData);

/**
 * @brief: read oem cfg data
 * @param {OUT USHORT_T *len -> read data len}
 * @param {OUT UCHAR_T *data -> read data}
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashReadOemCfgData(OUT USHORT_T *pLen, OUT UCHAR_T *pData);

/**
 * @brief: erase all user flash data
 * @param {none}
 * @retval: OPERATE_LIGHT
 */
OPERATE_LIGHT opUserFlashDataErase(VOID);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __USER_FLASH_H__ */
