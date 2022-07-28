/*
 * Tencent is pleased to support the open source community by making IoT Hub
 available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file
 except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software
 distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND,
 * either express or implied. See the License for the specific language
 governing permissions and
 * limitations under the License.
 *
 */

#ifndef DATA_TEMPLATE_TVS_AUTH_H_
#define DATA_TEMPLATE_TVS_AUTH_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Functions for tvs system property handle init
 * @param pTemplate_client    data template client
 * @return              QCLOUD_RET_SUCCESS for success, or err code for failure
 */
int IOT_Tvs_Auth_Init(void *pTemplate_client);

/**
 * @brief Functions for tvs auth handle
 * @param pTemplate_client    data template client
 * @return              QCLOUD_RET_SUCCESS for success, or err code for failure
 */
int IOT_Tvs_Auth_Handle(void *pTemplate_client);

/**
 * @brief Functions for tvs_executor_start_authorize fail
 * @param result   boot auth result
 * @param error    err code
 */
void IOT_Tvs_Auth_Error_Cb(bool result, int error);


#ifdef __cplusplus
}
#endif

#endif  // QCLOUD_IOT_UTILS_TIMER_H_
