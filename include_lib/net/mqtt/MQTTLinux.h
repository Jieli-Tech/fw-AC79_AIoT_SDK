/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#ifndef __MQTT_LINUX_
#define __MQTT_LINUX_
#include "sock_api/sock_api.h"
typedef struct Timer Timer;

struct Timer {
    struct timeval end_time;
};

typedef struct Network Network;

struct Network {
    void *my_socket;
    int (*mqttread)(Network *, unsigned char *, int, unsigned long);
    int (*mqttwrite)(Network *, unsigned char *, int, unsigned long);
    void (*disconnect)(Network *);
    int (*cb_func)(enum sock_api_msg_type, void *);
    void *priv;
    char state;
    char used_tls1_2;

    const char *cas_pem_path;
    const char *cli_pem_path;
    const char *pkey_path;
    int cas_pem_len;
    int cli_pem_len;
    int pkey_len;
};

char mqtt_expired(Timer *);
void mqtt_countdown_ms(Timer *, unsigned int);
void mqtt_countdown(Timer *, unsigned int);
unsigned long mqtt_left_ms(Timer *);

void mqtt_InitTimer(Timer *);
void NewNetwork(Network *);
void SetNetworkCb(Network *n, int (*cb_func)(enum sock_api_msg_type, void *), void *priv);
void NetWorkSetTLS(Network *n);
void NetWorkSetTLS_key(Network *n, const char *cas_pem, int cas_pem_len, const char *cli_pem, int cli_pem_len, const char *pkey, int pkey_len);
int ConnectNetwork(Network *n, char *addr, int port);
void SetNetworkRecvTimeout(Network *n, u32 timeout_ms);
void SetNetworkSendTimeout(Network *n, u32 timeout_ms);

#ifdef timeradd
#undef timeradd
#endif

#ifdef timersub
#undef timersub
#endif

/* Convenience macros for operations on timevals.
   NOTE: `timercmp' does not work for >= or <=.  */
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timerclear(tvp)		((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define	timercmp(a, b, CMP) 						      \
  (((a)->tv_sec == (b)->tv_sec) ? 					      \
   ((a)->tv_usec CMP (b)->tv_usec) : 					      \
   ((a)->tv_sec CMP (b)->tv_sec))
#define	timeradd(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;			      \
    if ((result)->tv_usec >= 1000000)					      \
      {									      \
	++(result)->tv_sec;						      \
	(result)->tv_usec -= 1000000;					      \
      }									      \
  } while (0)
#define	timersub(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;			      \
    if ((result)->tv_usec < 0) {					      \
      --(result)->tv_sec;						      \
      (result)->tv_usec += 1000000;					      \
    }									      \
  } while (0)
#endif
