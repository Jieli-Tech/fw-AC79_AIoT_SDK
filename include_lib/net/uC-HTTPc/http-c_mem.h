/*
*********************************************************************************************************
*                                               uC/HTTP
*                                     Hypertext Transfer Protocol
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                     HTTP CLIENT MEMORY LIBRARY
*
* Filename : http-c_mem.h
* Version  : V3.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This main network protocol suite header file is protected from multiple pre-processor
*               inclusion through use of the HTTP memory module present pre-processor macro definition.
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  HTTPc_MEM_PRESENT                                          /* See Note #1.                                         */
#define  HTTPc_MEM_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "http-c.h"
#ifdef  HTTPc_WEBSOCK_MODULE_EN
#include  "http-c_websock.h"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef enum {
    HTTPc_MSG_TYPE_CONN_OPEN,
    HTTPc_MSG_TYPE_CONN_CLOSE,
    HTTPc_MSG_TYPE_REQ,
    HTTPc_MSG_TYPE_WEBSOCK_MSG,
} HTTPc_MSG_TYPE;

typedef struct httpc_task_msg {
    HTTPc_MSG_TYPE  Type;
    void           *DataPtr;
} HTTPc_TASK_MSG;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void               HTTPc_Mem_TaskMsgPoolInit(const  HTTPc_CFG           *p_cfg,
        MEM_SEG             *p_seg,
        HTTPc_ERR           *p_err);

HTTPc_TASK_MSG    *HTTPc_Mem_TaskMsgGet(void);

void               HTTPc_Mem_TaskMsgRelease(HTTPc_TASK_MSG      *p_msg);


#ifdef  HTTPc_WEBSOCK_MODULE_EN
void               HTTPc_Mem_WebSockReqPoolInit(const HTTPc_CFG           *p_cfg,
        MEM_SEG             *p_seg,
        HTTPc_ERR           *p_err);

HTTPc_WEBSOCK_REQ *HTTPc_Mem_WebSockReqGet(void);

void               HTTPc_Mem_WebSockReqRelease(HTTPc_WEBSOCK_REQ   *p_ws_req);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of HTTPc memory module include.                  */

