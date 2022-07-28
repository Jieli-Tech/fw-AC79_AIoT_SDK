/* *
 *  $Id$
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

#ifndef _JPEGH
#define _JPEGH

#include "mediainfo.h"
#include "types.h"

/*
   The following routine is used to illustrate the RTP/JPEG packet
   fragmentation and header creation.

   For clarity and brevity, the structure definitions are only valid for
   32-bit big-endian (most significant octet first) architectures. Bit
   fields are assumed to be packed tightly in big-endian bit order, with
   no additional padding. Modifications would be required to construct a
   portable implementation.
*/
#define RTP_HDR_SZ 12

/* The following definition is from RFC1890 */
#define RTP_PT_JPEG             26

struct jpeghdr {
    unsigned int tspec: 8;  /* type-specific field */
    unsigned int off: 24;   /* fragment byte offset */
    uint8 type;            /* id of jpeg decoder params */
    uint8 q;               /* quantization factor (or table id) */
    uint8 width;           /* frame width in 8 pixel blocks */
    uint8 height;          /* frame height in 8 pixel blocks */
};

struct jpeghdr_rst {
    uint16 dri;
    unsigned int f: 1;
    unsigned int l: 1;
    unsigned int count: 14;
};
struct jpeghdr_qtable {
    uint8  mbz;
    uint8  precision;
    uint16 length;
};

#define RTP_JPEG_RESTART           0x40


/* Procedure SendFrame:
 *
 *  Arguments:
 *    start_seq: The sequence number for the first packet of the current
 *               frame.
 *    ts: RTP timestamp for the current frame
 *    ssrc: RTP SSRC value
 *    jpeg_data: Huffman encoded JPEG scan data
 *    len: Length of the JPEG scan data
 *    type: The value the RTP/JPEG type field should be set to
 *    typespec: The value the RTP/JPEG type-specific field should be set
 *              to
 *    width: The width in pixels of the JPEG image
 *    height: The height in pixels of the JPEG image
 *    dri: The number of MCUs between restart markers (or 0 if there
 *         are no restart markers in the data
 *    q: The Q factor of the data, to be specified using the Independent
 *       JPEG group's algorithm if 1 <= q <= 99, specified explicitly
 *       with lqt and cqt if q >= 128, or undefined otherwise.
 *    lqt: The quantization table for the luminance channel if q >= 128
 *    cqt: The quantization table for the chrominance channels if
 *         q >= 128
 *
 *  Return value:
 *    the sequence number to be sent for the first packet of the next
 *    frame.
 *
 * The following are assumed to be defined:
 *
 * PACKET_SIZE                         - The size of the outgoing packet
 * send_packet(u_int8 *data, int len)  - Sends the packet to the network
 */


typedef struct {
    uint8 extractQTable1[64];
    uint8 extractQTable2[64];
    struct jpeghdr jpghdr;
    struct jpeghdr_rst rsthdr;
    struct jpeghdr_qtable qtblhdr;
    uint32 bytes_left;
    uint32 frame_start;
    uint32 frame_cnt1;
    uint32 frame_cnt2;
    uint32 off;
    uint32 s_mtime;
    uint32 byte_per_slice;
    uint32 stored;
} JPEG_WORK;

int load_JPEG(media_entry *me);
void reload_JPEG(media_entry *p, int frame_rate, int screen_width, int screen_height);
int read_JPEG(media_entry *me, uint8 *buffer, uint32 *buffer_size,
              uint64 *mtime, int *recallme, uint8 *marker);
int free_JPEG(void *stat);

#endif
