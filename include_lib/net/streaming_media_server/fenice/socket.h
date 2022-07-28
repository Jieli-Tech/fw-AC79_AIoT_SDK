/* *
 *  $Id: socket.h 395 2006-10-05 16:08:10Z shawill $
 *
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
 *	- Dario Gallucci	<dario.gallucci@gmail.com>
 *
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * */

#ifndef _SOCKETH
#define _SOCKETH

#include "../fenice_config.h"

typedef unsigned short u_int16;

#if defined(__alpha)
typedef unsigned int u_int32;
#else
typedef unsigned long u_int32;
#endif

#ifndef BYTE_ORDER

#define LITTLE_ENDIAN	1234
#define BIG_ENDIAN	4321

#if defined(sun) || defined(__BIG_ENDIAN) || defined(NET_ENDIAN)
#define BYTE_ORDER	BIG_ENDIAN
#else
#define BYTE_ORDER	LITTLE_ENDIAN
#endif

#endif



#include "lwip/sockets.h"
#ifdef HAVE_SCTP_FENICE
#include <netinet/sctp.h>
#endif

typedef int tsocket;

#ifndef HAVE_STRUCT_SOCKADDR_STORAGE
#define MAXSOCKADDR 128		/*!< max socket address structure size */
struct sockaddr_storage {
    char padding[MAXSOCKADDR];
};
#endif				// HAVE_STRUCT_SOCKADDR_STORAGE

char *get_address();
char *sock_ntop_host(const struct sockaddr *, socklen_t, char *, size_t);
//TCP
tsocket fenice_tcp_listen(unsigned short port);
tsocket fenice_tcp_accept(tsocket fd, struct sockaddr_in *dest_addr);
int fenice_tcp_read(tsocket fd, void *buffer, int nbytes);
int fenice_tcp_write(tsocket fd, void *buffer, int nbytes);
void fenice_tcp_close(tsocket s);
tsocket fenice_tcp_connect(unsigned short port, char *addr);
//SCTP
#ifdef HAVE_SCTP_FENICE
#define MAX_SCTP_STREAMS 11
tsocket fenice_sctp_listen(unsigned short port);
tsocket fenice_sctp_accept(tsocket fd);
void fenice_sctp_close(tsocket s);
tsocket fenice_sctp_connect(unsigned short port, char *addr);
#endif				// HAVE_SCTP_FENICE
//UDP
int fenice_udp_connect(unsigned short to_port, struct sockaddr *s_addr, int addr,
                       tsocket *fd);
int fenice_udp_open(unsigned short port, struct sockaddr *s_addr, tsocket *fd);
int fenice_udp_close(tsocket fd);

#ifdef HAVE_SCTP_FENICE
#define MAX_INTERLVD_STMS MAX_SCTP_STREAMS
#else  // HAVE_SCTP_FENICE
#define MAX_INTERLVD_STMS 11
#endif // HAVE_SCTP_FENICE
#endif
