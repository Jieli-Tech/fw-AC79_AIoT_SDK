/*
 * @Author: wls
 * @email: wuls@tuya.com
 * @Date: 2019-05-22 10:17:39
 * @LastEditors: wls
 * @LastEditTime: 2019-05-29 09:50:10
 * @file name: light_system.c
 * @Description: system adapter process
 * @Copyright: HANGZHOU TUYA INFORMATION TECHNOLOGY CO.,LTD
 * @Company: http://www.tuya.com
 */

#include "light_system.h"
#include "light_printf.h"
#include "uni_thread.h"
#include "tuya_os_adapter.h"


#include "user_flash.h"
#include "gpio_test.h"
#include "mf_test.h"

#include "uni_time_queue.h"
#include "tuya_iot_wifi_api.h"
#include "cJSON.h"
#include "gw_intf.h"

#include "light_tools.h"
#include "tuya_gpio.h"
#include "tuya_key.h"
#include "device/gpio.h"
//#include "tuya_ble_mutli_tsf_protocol.h"

#define APP_BIN_NAME "jl_ac7916_test"
#define USER_SW_VER "1.0.0"

typedef struct s_klv_node *p_klv_node_s;//klv_node_s

/**
 * @brief: light reset(re-distribute) proc(wifi 8710bn)
 * @param {none}
 * @retval: BOOL_T TRUE -> system reboot
 */
BOOL_T bLightSysHWRebootJudge(VOID)
{
    TY_RST_REASON_E cRstInf = tuya_hal_system_get_rst_info();

    PR_DEBUG("reset info -> reason is %d", cRstInf);
    if (TY_RST_POWER_OFF == cRstInf) {
        return TRUE;
    }
    return FALSE;
}

STATIC VOID gw_stata_cb(UINT_T timerID, PVOID_T pTimerArg)
{
    STATIC GW_WIFI_NW_STAT_E last_nw_stat = 0xff;
    GW_WIFI_NW_STAT_E cur_nw_stat = 0;

    OPERATE_RET op_ret = OPRT_OK;
    op_ret = get_wf_gw_nw_status(&cur_nw_stat);

    if (OPRT_OK != op_ret) {
        PR_NOTICE("get_wf_gw_nw_status error:%d", op_ret);
        return;
    }

    if (cur_nw_stat != last_nw_stat) {
        PR_NOTICE("WIFI STATUS now is %d", cur_nw_stat);
        last_nw_stat = cur_nw_stat;
    }
}


/**
 * @brief: light reset(re-distribute) proc(wifi 8710bn)
 * @param {none}
 * @retval: OPERATE_RET
 */
OPERATE_RET opLightSysResetCntOverCB(VOID)
{
    OPERATE_RET opRet = -1;
    UCHAR_T ucConnectMode = 0;

    ucConnectMode = GWCM_OLD;
    opRet = tuya_iot_wf_gw_unactive();

    return opRet;
}

void reset_tuya(void)
{
    opLightSysResetCntOverCB();
}

/**
 * @brief: light connect blink display proc(wifi 8710bn)
 * @param {IN CONST GW_WIFI_NW_STAT_E stat -> wifi connect mode}
 * @retval: none
 */
STATIC VOID vWifiStatusDisplayCB(IN CONST GW_WIFI_NW_STAT_E stat)
{
    OPERATE_RET opRet = -1;
    STATIC GW_WIFI_NW_STAT_E LastWifiStat = 0xFF;
    STATIC UCHAR_T ucConnectFlag = FALSE;

    if (LastWifiStat != stat) {
        PR_DEBUG("last wifi stat:%d, wifi stat %d", LastWifiStat, stat);
        PR_DEBUG("size:%d", tuya_hal_system_getheapsize());

        switch (stat) {
        case STAT_LOW_POWER:
            PR_NOTICE("start to lowpower display!");

            break;

        case STAT_UNPROVISION:
            ucConnectFlag = TRUE;                /* already distribution network */

            break;

        case STAT_AP_STA_UNCFG:
            ucConnectFlag = TRUE;                /* already distribution network */

            break;

        case STAT_AP_STA_DISC: /*  */
            /* do nothing */
            break;

        case STAT_AP_STA_CONN:  /* priority turn down */
            /* do nothing */
            break;


        case STAT_STA_DISC:
            if (ucConnectFlag != TRUE) { /* only distribution network, need to stop and run ctrl proc */
                break;
            }

            ucConnectFlag = FALSE;          /* to avoid disconnect, set default bright cfg */
            PR_DEBUG("Blink stop!!!!");

            break;

        case STAT_STA_CONN:     /* priority turn down */
            if (ucConnectFlag != TRUE) { /* only distribution network, need to stop and run ctrl proc */
                break;
            }

            ucConnectFlag = FALSE;


            break;

        case STAT_CLOUD_CONN:
        case STAT_AP_CLOUD_CONN:


            break;

        default:
            break;
        }
        LastWifiStat = stat;
    }
}

BOOL_T bLightDpProc(TY_OBJ_DP_S *root)
{
    OPERATE_LIGHT opRet = -1;
    UINT_T ucLen;
    UCHAR_T dpid;
    BOOL_T bActiveFlag = FALSE;
    BOOL_T bBtEnable = FALSE;

    dpid = root->dpid;

    PR_DEBUG("light_light_dp_proc dpid=%d", dpid);

    switch (dpid) {
    case DPID_SWITCH:
        PR_DEBUG("set switch %d", root->value.dp_bool);
        if (root->value.dp_bool == TRUE) {
            puts("LED ON\n");
            gpio_direction_output(IO_PORTC_01, 1);
        } else {
            puts("LED OFF\n");
            gpio_direction_output(IO_PORTC_01, 0);
        }
        break;
    case DPID_BRIGHT:
        printf("bright value :%d\n", root->value.dp_value);
        break;
    case DPID_TEMPR:
        printf("tempr value :%d\n", root->value.dp_value);
        break;
    }
    return bActiveFlag;
}

/**
 * @berief: light dp ctrl process callback
 * @param {IN CONST TY_RECV_OBJ_DP_S *dp -> dp ctrl data}
 * @retval: none
 */
STATIC VOID vDeviceCB(IN CONST TY_RECV_OBJ_DP_S *dp)
{
    PR_DEBUG_RAW("mem:%d\r\n", tuya_hal_system_getheapsize());
    dev_report_dp_json_async(NULL, dp->dps, dp->dps_cnt);

    UCHAR_T i = 0;

    if (NULL == dp) {
        PR_ERR("dp error");
        return;
    }

    UCHAR_T nxt = dp->dps_cnt;
    PR_DEBUG("dp_cnt:%d", nxt);

    for (i = 0; i < nxt; i++) {
        bLightDpProc(&(dp->dps[i]));
    }

#if 0
    OPERATE_RET op_ret = OPRT_OK;

    TY_OBJ_DP_S obj_dp;

    if (obj_dp.type == PROP_STR) {



    } else {

#if 1
        OPERATE_RET op_ret = OPRT_OK;
        INT_T ch_idx = 0, dp_idx = 0;
        p_klv_node_s dp_bluetooth_node = NULL;


        PR_NOTICE("upload_all_switch_dp_stat Malloc SIZE :%d", dp->dps_cnt * SIZEOF(TY_OBJ_DP_S));
        TY_OBJ_DP_S *dp_arr = (TY_OBJ_DP_S *)Malloc(dp->dps_cnt * SIZEOF(TY_OBJ_DP_S));
        if (NULL == dp_arr) {
            PR_ERR("Malloc failed");
            return OPRT_MALLOC_FAILED;
        }

        memset(dp_arr, 0, dp->dps_cnt * SIZEOF(TY_OBJ_DP_S));
        for (ch_idx = 0, dp_idx = 0; (ch_idx < dp->dps_cnt) && (dp_idx < dp->dps_cnt); \
             ch_idx++, dp_idx++) {
            dp_arr[dp_idx].dpid = dp->dps[ch_idx].dpid;
            dp_arr[dp_idx].type = PROP_BOOL;
            dp_arr[dp_idx].time_stamp = 0;
            dp_arr[dp_idx].value.dp_bool = dp->dps[ch_idx].value.dp_bool;
            dp_bluetooth_node = make_klv_list(dp_bluetooth_node, dp->dps[ch_idx].dpid, DT_BOOL, &(dp_arr[dp_idx].value.dp_bool), DT_BOOL_LEN);
        }

        ty_bt_klv_report(dp_bluetooth_node);
        free_klv_list(dp_bluetooth_node);


        op_ret = dev_report_dp_json_async(get_gw_cntl()->gw_if.id, dp_arr, dp->dps_cnt);
        Free(dp_arr);
        PR_NOTICE("Free ok :%d");
        dp_arr = NULL;
        if (OPRT_OK != op_ret) {
            PR_ERR("upload_all_switch_dp_stat op_ret:%d", op_ret);
            return op_ret;
        }
#endif
    }

#endif
    return ;
}

/**
 * @berief: reset proc callback
 * @param {GW_RESET_TYPE_E type -> reset reason}
 * @retval: none
 */
STATIC VOID vResetCB(GW_RESET_TYPE_E type)
{
    /* attention: before restart ,need to save in flash */
    switch (type) {
    case GW_LOCAL_RESET_FACTORY:
    case GW_LOCAL_UNACTIVE:

        break;

    case GW_REMOTE_UNACTIVE:
    case GW_REMOTE_RESET_FACTORY:

        break;
    }
}

/**
 * @berief: query dp process
 * @param {none}
 * @retval: none
 */
STATIC VOID vQueryCB(IN CONST TY_DP_QUERY_S *dp_qry)
{

}

/***********************************************************
*  Function:dev_raw_dp_cb
*  Input: none
*  Output: none
*  Return: none
*  Note:
***********************************************************/
STATIC VOID dev_raw_dp_cb(IN CONST TY_RECV_RAW_DP_S *dp)
{
    dev_report_dp_raw_sync(NULL, dp->dpid, dp->data, dp->len, 5);
}



STATIC VOID key_process(TY_GPIO_PORT_E port, PUSH_KEY_TYPE_E type, INT_T cnt)
{
    PR_DEBUG("port: %d", port);
    PR_DEBUG("type: %d", type);
    PR_DEBUG("cnt: %d", cnt);
    PR_NOTICE("key process");

    if (LONG_KEY == type) {
        tuya_iot_wf_gw_unactive();//AP  smart cfg   切换
    }
}

STATIC VOID LightHalInit(VOID)
{
    OPERATE_RET op_ret = -1;

    op_ret = key_init(NULL, 0, 20);
    if (op_ret != OPRT_OK) {
        PR_ERR("key_init err:%d", op_ret);
        return;
    }

    KEY_USER_DEF_S rst_key = {TY_GPIOA_3, TRUE, LP_ONCE_TRIG, 3000, 50, key_process};

    op_ret = reg_proc_key(&rst_key);//初始化reset按键
    if (op_ret != OPRT_OK) {
        PR_ERR("reg_proc_key err:%d", op_ret);
        return;
    }
}

STATIC VOID mem_moniter_tiemout_cb(UINT_T timerID, PVOID_T pTimerArg)
{
    PR_NOTICE("mem:%d", tuya_hal_system_getheapsize());
}


/**
 * @berief: light smart frame init(wifi 8710bn)
 * @param {IN CHAR_T *sw_ver -> bin version}
 * @return: none
 * @retval: none
 */
STATIC OPERATE_RET opLightSysSmartFrameInit(IN CHAR_T *sw_ver)
{
    OPERATE_RET opRet = -1;
    UCHAR_T ucConnectMode = 0;

    TY_IOT_CBS_S wf_cbs = {
        NULL, \
        NULL, \
        vResetCB, \
        vDeviceCB, \
        dev_raw_dp_cb, \
        vQueryCB, \
        NULL,
    };

    PR_NOTICE("frame goto init!");

    /****/
    ucConnectMode = GWCM_OLD;
    /* wifi smart fram inits */
    opRet = tuya_iot_wf_mcu_dev_init(ucConnectMode, WF_START_AP_ONLY, &wf_cbs, FIRMWARE_KEY, PRODUCT_KEY, sw_ver, "1.0.0");
    if (OPRT_OK != opRet) {
        PR_ERR("tuya_iot_wf_soc_dev_init err:%02x", opRet);
        return opRet;
    }

    LightHalInit();

    PR_NOTICE("frame init out!");
    opRet = tuya_iot_reg_get_wf_nw_stat_cb(vWifiStatusDisplayCB);   /* register wifi status callback */
    if (OPRT_OK != opRet) {
        PR_ERR("tuya_iot_reg_get_wf_nw_stat_cb err:%02x", opRet);
        return opRet;
    }

    PR_NOTICE("frame init ok!");


    TIMER_ID mem_moniter;
    opRet = sys_add_timer(mem_moniter_tiemout_cb, NULL, &mem_moniter);
    if (OPRT_OK != opRet) {
        PR_ERR("mem_moniter_tiemout_cb timer add err:%d", opRet);
    } else {
        sys_start_timer(mem_moniter, 1000, TIMER_CYCLE);
    }

    TIMER_ID gw_stata;
    opRet = sys_add_timer(gw_stata_cb, NULL, &gw_stata);
    if (OPRT_OK != opRet) {
        PR_ERR("gw_stata_cb timer add err:%d", opRet);
    } else {
        sys_start_timer(gw_stata, 50, TIMER_CYCLE);
    }

    return OPRT_OK;
}

STATIC VOID light_rst_cnt_tiemout_cb(UINT_T timerID, PVOID_T pTimerArg)
{

    opUserFlashWriteResetCnt(0);    /* write cnt = 0 to flash!! */
    PR_DEBUG("set reset cnt -> 0");
}


/**
 * @berief: Light hardware reboot judge & proc
 *          process detail:
 *                  1. hardware reset judge;
 *                  2. load reboot cnt data;
 *                  3. reboot cnt data increase;
 *                  4. start software time to clear reboot cnt;
 * @param {none}
 * @attention: this function need bLightSysHWRebootJudge();
 * @retval: none
 */
VOID vLightCtrlHWRebootProc(VOID)
{
    OPERATE_RET opRet = -1;
    BOOL_T bHWRebootFlag = FALSE;
    UCHAR_T ucCnt = 0;

    bHWRebootFlag = bLightSysHWRebootJudge();
    if (TRUE != bHWRebootFlag) {
        return;
    }

    PR_DEBUG("Light hardware reboot, turn on light!");

    opRet = opUserFlashReadResetCnt(&ucCnt);     /* read cnt from flash */
    if (opRet != OPRT_OK) {
        PR_ERR("Read reset cnt error!");
    }
    PR_DEBUG("read reset cnt %d", ucCnt);

    ucCnt++;
    opRet = opUserFlashWriteResetCnt(ucCnt);     /* Reset cnt ++ &save to flash */
    if (opRet != OPRT_OK) {
        PR_ERR("Reset cnt add write error!");
        return ;
    }

    TIMER_ID timer_rst_cnt;
    opRet = sys_add_timer(light_rst_cnt_tiemout_cb, NULL, &timer_rst_cnt);
    if (OPRT_OK != opRet) {
        PR_ERR("reset_fsw_cnt timer add err:%d", opRet);
    } else {
        sys_start_timer(timer_rst_cnt, 5000, TIMER_ONCE);        /* 启动一个timer，定时满足后清除cnt值 */
    }

    PR_DEBUG("start reset cnt clear timer!!!!!");

}

/**
 * @brief: Light reset to re-distribute proc
 * @param {none}
 * @attention: this func will call opLightSysResetCntOverCB()
 *              opLightSysResetCntOverCB() need to implement by system
 * @retval: OPERATE_RET
 */
OPERATE_RET opLightCtrlResetCntProcess(VOID)
{
    OPERATE_RET opRet = -1;
    UCHAR_T ucCnt = 0;

    opRet = opUserFlashReadResetCnt(&ucCnt);     /* read cnt from flash */
    if (opRet != OPRT_OK) {
        PR_ERR("Read reset cnt error!");
        return OPRT_COM_ERROR;
    }

    if (ucCnt < 3) {
        PR_DEBUG("Don't reset ctrl data!");
        return OPRT_OK;
    }

    opRet = opUserFlashWriteResetCnt(0);    /* write cnt = 0 to flash!! */
    if (opRet != OPRT_OK) {
        PR_ERR("reset cnt set error!");
    }

    PR_NOTICE("Light will reset!");
    opRet = opLightSysResetCntOverCB();     /* system reset deal proc */
    if (opRet != OPRT_OK) {
        PR_ERR("Light reset proc error!");
    }

    return opRet;
}

VOID_T pre_app_init(VOID_T)
{
    return;
}

OPERATE_RET mf_user_product_test_cb(USHORT_T cmd, UCHAR_T *data, UINT_T len, OUT UCHAR_T **ret_data, OUT USHORT_T *ret_len)
{
    return OPRT_OK;
}

VOID_T mf_user_enter_callback(VOID_T)
{
    return ;
}

VOID_T mf_user_pre_gpio_test_cb(VOID_T)
{
    return;
}


/**
 * @berief: wifi(realtek 8710bn) fast initlize process
 * @param {none}
 * @attention: this partion can't operate kv flash
                and other wifi system service
 * @retval: none
 */
VOID pre_device_init(VOID)
{
    mf_test_ignore_close_flag();

    /* attention: to make sure light up in 500ms! */
    vLightCtrlHWRebootProc();                       /* write recnt count! reload ctrl data! */
    /* write cnt into flash will take 220ms time */

    SetLogManageAttr(TY_LOG_LEVEL_DEBUG);
}


/**
 * @berief: wifi(realtek 8710bn) normal initlize process
 * @param {none}
 * @retval: none
 */
VOID app_init(VOID)
{
    OPERATE_RET opRet = -1;

    //sys_log_uart_on();                                /* 打开打印 */
    PR_NOTICE("%s", tuya_iot_get_sdk_info());           /* output sdk information */
    PR_NOTICE("%s:%s", APP_BIN_NAME, USER_SW_VER);      /* output bin information */

#ifdef _IS_OEM
    tuya_iot_oem_set(TRUE);
#endif
}


/**
 * @brief: device init
 * @param {none}
 * @retval: none
 */
OPERATE_RET device_init(VOID)
{
    OPERATE_RET opRet = -1;

    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>%s, %d\n", __FUNCTION__, __LINE__);
    /** gpio init*/
    gpio_direction_output(IO_PORTC_01, 0);   //GPIO输出

    PR_NOTICE("goto device_init!!!");
    opRet = opLightSysSmartFrameInit(USER_SW_VER);      /* wifi frame init */
    if (opRet != OPRT_OK) {
        PR_ERR("smart fram init error");
    }

    opRet = opLightCtrlResetCntProcess();
    if (opRet != OPRT_OK) {
        PR_ERR("Light Reset proc error!");
        return opRet;
    }

    return opRet;
}

/**
 * @brief: 8710 wifi gpio test
 * @param {none}
 * @retval: gpio_test_cb
 */
BOOL_T gpio_test(VOID)
{
    return TRUE;
}

/**
 * @berief: erase user data when authorization
 * @param {none}
 * @attention:
 * @retval: none
 */
VOID mf_user_callback(VOID)
{
    OPERATE_RET opRet = -1;

    opRet = opUserFlashDataErase();
    if (opRet != OPRT_OK) {
        PR_ERR("Erase user flash error!");
    }
}


