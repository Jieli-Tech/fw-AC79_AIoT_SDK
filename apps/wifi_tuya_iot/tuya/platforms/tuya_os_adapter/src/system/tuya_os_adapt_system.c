/**
 * @file tuya_os_adapt_system.c
 * @brief 操作系统相关接口
 *
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */
#define _UNI_SYSTEM_GLOBAL

#include "FreeRTOS/FreeRTOS.h"
#include "task.h"
#include "FreeRTOS/semphr.h"


#include "system/includes.h"
#include "tuya_os_adapt_system.h"
#include "tuya_os_adapter_error_code.h"


/***********************************************************
*************************micro define***********************
***********************************************************/
unsigned int tuya_os_adapt_watchdog_init_start(const unsigned int timeval);
void tuya_os_adapt_watchdog_refresh(void);
void tuya_os_adapt_watchdog_stop(void);


#define SERIAL_NUM_LEN 32

//realtek supports 0,1,5
typedef enum {
    REASON_CPU_RESET_HAPPEN = 0,/*!< &&&&&&& 0 hardware reset or system reset (distinguish from 'REASON_SYS_RESET_HAPPEN' by Software) &&&&&&&77*/
    REASON_BOR2_RESET_HAPPEN,/*!<&&&&&&&& 1 watchdog reset **^&&&&&&&*/
    REASON_RTC_RESTORE, /*!< 2 this is SW set bit after rtc init */
    REASON_UARTBURN_BOOT,/*!< 3 this is SW set bit before reboot, for uart download */
    REASON_UARTBURN_DEBUG,/*!< 4 this is SW set bit before reboot, for uart download debug */
    REASON_SYS_RESET_HAPPEN,/*!<  &&&&&&&& 5 this is SW set bit before reboot, for distinguish 'REASON_CPU_RESET_HAPPEN'&&&&&&&&&&& */
    REASON_BOR2_RESET_TEMP, /*!<  BOR2 HW temp bit */
    REASON_SYS_BOR_DETECION /*!<  1: enable bor2 detection;  0: disable */
} RST_REASON_E;

/***********************************************************
*************************variable define********************
***********************************************************/
bool is_lp_enable = false;

static const TUYA_OS_SYSTEM_INTF m_tuya_os_system_intfs = {
    .get_systemtickcount   = tuya_os_adapt_get_systemtickcount,
    .get_tickratems        = tuya_os_adapt_get_tickratems,
    .system_sleep          = tuya_os_adapt_system_sleep,
    .system_reset          = tuya_os_adapt_system_reset,
    .watchdog_init_start   = tuya_os_adapt_watchdog_init_start,
    .watchdog_refresh      = tuya_os_adapt_watchdog_refresh,
    .watchdog_stop         = tuya_os_adapt_watchdog_stop,
    .system_getheapsize    = tuya_os_adapt_system_getheapsize,
    .system_get_rst_info   = tuya_os_adapt_system_get_rst_info,
    .system_get_rst_ext_info = NULL,
    .get_random_data       = tuya_os_adapt_get_random_data,
    .set_cpu_lp_mode       = tuya_os_adapt_set_cpu_lp_mode,
};

/***********************************************************
*************************extern define********************
***********************************************************/
extern void ota_platform_reset(void);
/* extern void srand(unsigned int seed); */
/* extern int rand(void); */
extern unsigned int OSGetTime();
/***********************************************************
*************************function define********************
***********************************************************/
/**
 * @brief tuya_os_adapt_get_systemtickcount用于获取系统运行ticket
 * @return SYS_TICK_T
 */
SYS_TICK_T tuya_os_adapt_get_systemtickcount(void)
{
    return (SYS_TICK_T)xTaskGetTickCount();
}


/**
 * @brief tuya_os_adapt_get_tickratems用于获取系统ticket是多少个ms
 *
 * @return the time is ms of a system ticket
 */
unsigned int tuya_os_adapt_get_tickratems(void)
{
    return (unsigned int)portTICK_RATE_MS;
}


/**
 * @brief tuya_os_adapt_system_sleep用于系统sleep
 *
 * @param[in] msTime sleep time is ms
 */
void tuya_os_adapt_system_sleep(const unsigned long msTime)
{
    vTaskDelay((msTime) / (portTICK_RATE_MS));
}


/**
 * @brief tuya_os_adapt_system_isrstatus用于检查系统是否处于中断上下文
 *
 * @return true
 * @return false
 */
bool tuya_os_adapt_system_isrstatus(void)
{
    return FALSE;
}


/**
 * @brief tuya_os_adapt_system_reset用于重启系统
 *
 */
void tuya_os_adapt_system_reset(void)
{
    system_reset();
}

/**
 * @brief tuya_os_adapt_system_getheapsize用于获取堆大小/剩余内存大小
 *
 * @return int <0: don't support  >=0: current heap size/free memory
 */
int tuya_os_adapt_system_getheapsize(void)
{

    return 0;
}

/**
 * @brief tuya_os_adapt_system_getMiniheapsize/最小剩余内存大小
 *
 * @return int <0: don't support  >=0: mini heap size/free memory
 */
int tuya_os_adapt_system_getMiniheapsize(void)
{
    return 0;
}

/**
 * @brief tuya_os_adapt_system_get_rst_info用于获取硬件重启原因
 *
 * @return 硬件重启原因
 */
TY_RST_REASON_E tuya_os_adapt_system_get_rst_info(void)
{
    return TY_RST_POWER_OFF;
}

/**
 * @brief init random
 *
 * @param  void
 * @retval void
 */
void tuya_os_adapt_srandom(void)
{
}

/**
 * @brief tuya_os_adapt_get_random_data用于获取指定条件下的随机数
 *
 * @param[in] range
 * @return 随机值
 */
int tuya_os_adapt_get_random_data(const unsigned int range)
{
    unsigned int trange = range;

    if (range == 0) {
        trange = 0xFF;
    }

    static char exec_flag = FALSE;

    if (!exec_flag) {
        exec_flag = TRUE;
    }

    return ((rand32() + OSGetTime()) % trange);
}

/**
 * @brief tuya_os_adapt_set_cpu_lp_mode用于设置cpu的低功耗模式
 *
 * @param[in] en
 * @param[in] mode
 * @return int 0=成功，非0=失败
 */
int tuya_os_adapt_set_cpu_lp_mode(const bool_t en, const TY_CPU_SLEEP_MODE_E mode)
{
    // to do
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief 用于初始化并运行watchdog
 *
 * @param[in] timeval watch dog检测时间间隔：如果timeval大于看门狗的
 * 最大可设置时间，则使用平台可设置时间的最大值，并且返回该最大值
 * @return int [out] 实际设置的看门狗时间
 */
unsigned int tuya_os_adapt_watchdog_init_start(const unsigned int timeval)
{
    // to do
    return 0;
}

/**
 * @brief 用于刷新watch dog
 *
 */
void tuya_os_adapt_watchdog_refresh(void)
{
    // to do
}

/**
 * @brief 用于停止watch dog
 *
 */
void tuya_os_adapt_watchdog_stop(void)
{
    //to do
}

int tuya_os_adapt_reg_system_intf(void)
{
    return tuya_os_adapt_reg_intf(INTF_SYSTEM, (void *)&m_tuya_os_system_intfs);
}

