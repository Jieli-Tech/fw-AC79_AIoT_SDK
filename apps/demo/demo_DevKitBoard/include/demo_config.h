#ifndef DEMO_COFNIG_H
#define DEMO_COFNIG_H

//对应测试例子放在apps/common/example文件夹下
/****综合测试类*****/
#define USE_DevKitBoard_TEST_DEMO
#ifdef USE_DevKitBoard_TEST_DEMO
#define USE_DEMO_WIFI_TEST
#define USE_EDR_DEMO
#endif //USE_DevKitBoard_TEST_DEMO

/*****UI,LCD类******/
//#define USE_UI_TOUCH_DEMO
//#define USE_SD_PLAY_AVI
//#define USE_GIF_PLAY_DEMO

/*****video类******/
//#define USE_CAMERA_SPI_SHOW_TO_LCD_DEMO

/*****BT类******/
// #define USE_EDR_DEMO

/*****BLE类******/

/*****AUDIO类******/
// #define USE_AUDIO_DEMO

//#ifdef USE_AUDIO_DEMO

//#define CONFIG_LOCAL_MUSIC_MODE_ENABLE      //mode:本地播放模式使能
//#define CONFIG_RECORDER_MODE_ENABLE         //mode:录音模式使能

//#ifdef CONFIG_NET_ENABLE
//#define CONFIG_NET_MUSIC_MODE_ENABLE        //mode:网络播放模式使能
//#define CONFIG_ASR_ALGORITHM_ENABLE         //mode:打断唤醒模式使能
//#endif

//#endif

/*****VDIEO类******/
//#define USE_SD_PLAY_AVI_DEMO
//#define USE_CAMERA_DVP_SHOW_TO_LCD_DEMO
//#define USE_ENC_QR_CODE_DEMO


/*****网络协议栈类******/
//#define USE_TCP_CLIENT_TEST
//#define USE_TCP_SERVER_TEST
//#define USE_UDP_SERVER_TEST
//#define USE_HTTP_POST_GET_TEST
//#define USE_HTTP_DOWNLOAD_TEST
//#define USE_HTTP_POST_SD_DATA_TEST
//#define USE_HTTP_SERVER_TEST
//#define USE_MQTT_TEST
//#define USE_COAP_CLIENT_TEST
//#define USE_COAP_SERVER_TEST
//#define USE_WEBSOCKET_TEST

/**系统类**/
//#define USE_OS_API_TEST
//#define USE_PTHREAD_API_TEST
//#define USE_FS_TEST_DEMO
//#define USE_INIT_CALL_TEST_DEMO
//#define USE_LBUF_TEST_DEMO
//#define USE_CBUF_TEST_DEMO
//#define USE_INTERRUPT_TEST_DEMO
//#define USE_MALLOC_TEST_DEMO
//#define USE_HW_FLOAT_TEST_DEMO
//#define USE_APP_STATE_MACHINE_TEST_DEMO
//#define USE_CRYPTO_TEST_DEMO
//#define USE_EVENT_TEST_DEMO
//#define USE_LONG_PRESS_RESET_TEST_DEMO
//#define USE_WAIT_COMPLETION_TEST_DEMO
//#define USE_SYSTEM_RESET_REASON_TEST_DEMO
//#define USE_SOFT_POWER_OFF_TEST_DEMO
//#define USE_CPU_PERFORMANCE_TEST_DEMO
//#define USE_AI_SDK_DEMO
//#define USE_EXCEPTION_TEST_DEMO
// #define USE_MATH_TEST_DEMO
// #define USE_PRINTF_TEST_DEMO
// #define USE_MEMORY_TEST_DEMO
// #define USE_SYS_TIMEOUT_TEST_DEMO
// #define USE_SYS_TIMER_TEST_DEMO
// #define USE_USR_TIMEOUT_TEST_DEMO
// #define USE_USR_TIMER_TEST_DEMO
// #define USE_SERVER_TEST_DEMO

/**update**/
//#define USE_SD_UPGRADE_DEMO
//#define USE_HTTP_UPGRADE_DEMO1
//#define USE_HTTP_UPGRADE_DEMO2
//#define USE_FTP_UPGRADE_DEMO

/**WIFI类**/
// #define USE_DEMO_WIFI_TEST
//#define USE_WIFI_IPERF_TEST
//#define USE_WIFI_SCAN_TEST
//#define USE_LOW_POWER_TEST
//#define USE_WIFI_MUSIC_PLAY

/**外设类**/
// #define USE_GPIO_TEST_DEMO
// #define USE_ADC_TEST_DEMO
// #define USE_UART_TEST_DEMO
// #define USE_TIMER_TEST2_DEMO
// #define USE_TIMER_TEST1_DEMO
// #define USE_PWM_TEST_DEMO
// #define USE_SD_TEST_DEMO
// #define USE_UART_TX_RX_TEST_DEMO
// #define USE_PWM_LED_TEST_DEMO
// #define USE_GPCNT_TEST_DEMO
// #define USE_FLASH_TEST_DEMO
// #define USE_FLASH_OPT_TEST_DEMO
// #define USE_FLASH_USER_TEST_DEMO
// #define USE_FLASH_REGISTER_TEST_DEMO
// #define USE_FLASH_SYSCFG_TEST_DEMO

// #define USE_GSENSOR_TEST_DEMO
//#ifdef USE_GSENSOR_TEST_DEMO
//#define CONFIG_GSENSOR_ENABLE

//#endif

/****传感器类*****/
// #define USE_HC_SR04_DEMO //超声波传感器测试

/****第三方库*****/
//#define USE_FLASHDB_TEST_DEMO
//#define USE_FINSH_TEST_DEMO
//#define USE_NNOM_TEST_DEMO


#endif //DEMO_COFNIG_H
