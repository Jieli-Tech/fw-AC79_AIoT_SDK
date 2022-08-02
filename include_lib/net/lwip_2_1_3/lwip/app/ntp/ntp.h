///  \cond DO_NOT_DOCUMENT
//----------------------------------------------------------------------------//
/**
 ******************************************************************************
 * @file    ntp.h
 * @author
 * @version
 * @brief   This file provides the api of ntp client.
 ******************************************************************************
 * @attention
 *
 * Copyright(c) 2017, ZhuHai JieLi Technology Co.Ltd. All rights reserved.
 ******************************************************************************
 */

#ifndef NTP_H_
#define NTP_H_


#include <time.h>

/*! \addtogroup NTP
 *  @ingroup NETWORK_LIB
 *  @brief	NTP client api
 *  @{
 */

//此处添加ntp服务器,请自行测试ntp服务器是否正常
static const char *ntp_host[] = {
    "s2a.time.edu.cn",
    "s2b.time.edu.cn",
    "s2c.time.edu.cn",
    // "s2d.time.edu.cn",
    // "s2e.time.edu.cn",
    // "s2f.time.edu.cn",
    // "s2g.time.edu.cn",
    // "s2h.time.edu.cn",
    // "s2j.time.edu.cn",
    // "s2k.time.edu.cn",
    // "s2m.time.edu.cn",
};

#define NTP_HOST_NUM ARRAY_SIZE(ntp_host)

#define NTP_DBUG_INFO_ON	0		/*!< NTP client debug information on/off */
#define NTP_ERR_INFO_ON		1		/*!< NTP client error information on/off */
/// \endcond


/*----------------------------------------------------------------------------*/
/**@brief  Get time from ntp server
   @param  host: NTP server host name
   @param  s_tm: Save time information
   @param  recv_to: The value of socket receive timeout(ms)
   @return 0: sucess
   @return -1: fail
*/
/*----------------------------------------------------------------------------*/
int ntp_client_get_time_once(const char *host, struct tm *s_tm, int recv_to);



/*----------------------------------------------------------------------------*/
/**@brief  Get time from all ntp_host list server
   @param  s_tm: Save time information
   @param  recv_to: The value of socket receive timeout(ms)
   @return 0: sucess
   @return -1: fail
*/
/*----------------------------------------------------------------------------*/
int ntp_client_get_time_all(struct tm *s_tm, int recv_to);



/*----------------------------------------------------------------------------*/
/**@brief  Get the status of the time from ntp server
   @return 0: sucess
   @return -1: fail or still getting
*/
/*----------------------------------------------------------------------------*/
u8 ntp_client_get_time_status(void);



/*----------------------------------------------------------------------------*/
/**@brief  	Get time from ntp server,and save to the rtc,if it has.
   @param  	host: NTP server host name, if NULL, it will visit all the ntp_host list.

   @note 	This api is a loop, is will exit when getting time successful or calling ntp_client_get_time_exit function.
	When it succeed, it will post the net_event NET_NTP_GET_TIME_SUCC and set the ntp_client_time_status to 1.
*/
/*----------------------------------------------------------------------------*/
void ntp_client_get_time(const char *host);




/*----------------------------------------------------------------------------*/
/**@brief  	Exit the ntp_client_get_time function.
*/
/*----------------------------------------------------------------------------*/
void ntp_client_get_time_exit(void);



/*----------------------------------------------------------------------------*/
/**@brief  clear the status of the time from ntp server
*/
/*----------------------------------------------------------------------------*/
void ntp_client_get_time_clear(void);

/*! @}*/










///  \cond DO_NOT_DOCUMENT
#if 0	//此API未用到

#define NTP_CLIENT_THREAD_PRIO	22		/*!< The priority of NTP client thread  */

#define NTP_CLIENT_THREAD_STACK_SIZE	2048	/*!< The stack size of NTP client thread */

/*----------------------------------------------------------------------------*/
/**@brief  Set the query interval time of the ntp client
   @param   min: The query interval time
   @note
*/
/*----------------------------------------------------------------------------*/
void ntp_set_query_interval_min(unsigned int min);

/*----------------------------------------------------------------------------*/
/**@brief  Set the time zone
   @param   zone: The time zone
   @note
*/
/*----------------------------------------------------------------------------*/
void ntp_set_timezone(unsigned int zone);

/*----------------------------------------------------------------------------*/
/**@brief  Set the callback function called after get the time from ntp server
   @param  cb: The callback function
   @note
*/
/*----------------------------------------------------------------------------*/
void ntp_set_time_cb(void (*cb)(struct tm *t));

/*----------------------------------------------------------------------------*/
/**@brief  Insert a ntp server host name to the host list
   @param   name: NTP server host name
   @return 0: Success
   @return -1: Memory not enough
   @note
*/
/*----------------------------------------------------------------------------*/
int ntp_add_host_name(char *name);

/*----------------------------------------------------------------------------*/
/**@brief  Remove a ntp server host name to the host list
   @return 0: Success
   @return -1: Not find host name
   @note
*/
/*----------------------------------------------------------------------------*/
int ntp_remove_host_name(char *name);

/*----------------------------------------------------------------------------*/
/**@brief  Initialize ntp client
   @note
*/
/*----------------------------------------------------------------------------*/
void ntp_client_init(void);

/*----------------------------------------------------------------------------*/
/**@brief  Start ntp client
   @return 0: Start ntp client sucessfully
   @return other: Create thread error
   @note
*/
/*----------------------------------------------------------------------------*/
int ntp_client_start(void);

/*----------------------------------------------------------------------------*/
/**@brief  Uninstall ntp client
   @note
*/
/*----------------------------------------------------------------------------*/
void ntp_client_uninit(void);

#endif
/// \endcond


#endif















