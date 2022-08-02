#ifndef _RF_FCC_DATATYPE_H_
#define _RF_FCC_DATATYPE_H_

enum {
    FCC_WIFI_MODE = 1,
    FCC_BT_MODE,
    FCC_WAIT_MODE,
};


enum {
    OP_FCC_INQ_RAND_NUM = 1,
    OP_FCC_RPT_RAND_NUM = 2,

    OP_IN_WIFI = 5,

    OP_FCC_START_TX = 10,
    OP_FCC_START_RX,
    OP_FCC_STOP_RX,
    OP_FCC_SET_MAC,
    OP_FCC_INQ_MAC,
    OP_FCC_RPT_MAC,
    OP_FCC_RPT_ST,
    OP_FCC_RPT_RX_RES,

    OP_STA_FREQ_ADJ,
    OP_IN_FREQ_ADJ,
    OP_FIN_FREQ_ADJ,

    OP_STA_PWR_ADJ,
    OP_IN_PWR_ADJ,
    OP_FIN_PWR_ADJ,

    OP_FCC_INQ_DEF_DATA,
    OP_FCC_RPT_DEF_DATA,

    OP_FCC_STOP_TX,

    OP_FCC_SET_PA,
    OP_FCC_SET_CH,
    OP_FCC_SET_TX_RATE,
    OP_FCC_SET_TX_GAIN,

    OP_FCC_START_RX_STAT,
    OP_FCC_STOP_RX_STAT,
    OP_FCC_RPT_RX_STAT,
    OP_FCC_ENTER_WIFI_SIGN,
    OP_FCC_INQ_RX_STAT,

    OP_OUT_WIFI,

    OP_FCC_EDR_TEST       = 0xFC01,
    OP_FCC_BLE_TX_TEST = 0x2034,
    // OP_FCC_BLE_TX_TEST    = 0x201E,
    OP_FCC_BLE_START_RX_TEST = 0x2033,
    // OP_FCC_BLE_RX_TEST    = 0x201D,
    OP_FCC_BLE_STOP_RX_TEST = 0x201F,
    OP_FCC_BLE_LE_RESET   = 0x0C03,
    OP_FCC_BLE_POWER_TEST = 0xF885,


};

enum FCC_RES {
    FCC_SUCC = 0,
    FCC_FAIL,
    FCC_NULL,
};

enum {
    ST_SUCC = 0,
    ST_FAIL,
};


struct fcc_mode {
    char str[8];
    u8 mode;
    u16 crc;
};


#pragma pack (1)
struct fcc_data {
    u8 mark[2];
    u16 opcode;
    u16 params_len;
    u8 *params;
    u16 crc;
};
#pragma pack ()


#pragma pack (1)
struct host_data {
    u32 flag;
    struct list_head entry;
    struct fcc_data fcc_data;
    struct host_data *self;
};
#pragma pack ()


#pragma pack (1)
struct FCC_TX {
    u8  pa[7];
    u8  xosc[2];
    u8	channel;
    u8	bandwidth;
    u8	short_gi;
    u8	antenna_x;
    u8	pathx_txpower;
    u32	rate;
    u32	npackets;
    u32	packet_len;
    u32	send_interval;
    u8 cw_flag;
};
#pragma pack ()


#pragma pack (1)
struct FCC_RX {
    u8 pa[7];
    u8 xosc[2];
    u8 channel;
    u8 bandwidth;
    u8 short_gi;
    u8 antenna_x;
    u8 filter_enable;
    u8 filter_mac[6];
};
#pragma pack ()

#pragma pack (1)
struct FCC_RX_STAT {
    u8 pa[7];
    u8 xosc[2];
    u8 channel;
    u8 bandwidth;
    u8 short_gi;
    u8 antenna_x;
    u8 filter_enable;
    u8 filter_mac[6];
    u8 stat_time;
};
#pragma pack ()


#pragma pack (1)
struct FREQ_ADJ {
    u8 channel;//信道
    u8 rate;   //速率
    u8 pathx_txpower; //数字增益
    float thr_max;//频偏阈值上限(kHz)
    float thr_min;//频偏阈值下限(kHz)
    u8 max_cnt;//最大校准次数
    u8 step;   //校准步长
};
#pragma pack ()


#pragma pack (1)
struct PWR_ADJ {
    u8 channel;//信道
    u8 rate;   //速率
    float thr_max;//功率阈值上限(dBm)
    float thr_min;//功率阈值下限(dBm)
    u8 max_cnt;//最大校准次数
    u8 step;   //校准步长
};
#pragma pack ()


#pragma pack (1)
struct WIFI_SIGN {
    u8 ssid[33];
    u8 pwd[65];
    u8 xosc[2];
    u8 pa[7];
    u8 gain[20];
};
#pragma pack ()


#pragma pack (1)
struct WIFI_DEF_DATA {
    u8 pa[7];
    u8 xosc[2];
    u8 mcs_dgain[20];
};
#pragma pack ()


#pragma pack (1)
struct FCC_HIS {
    u8 mode;
    u8 data[48];
};
#pragma pack ()


enum EDR_TEST_MODE {
    EDR_TX = 0,
    EDR_RX,
};


enum EDR_PACKET_TYPE {
    _1_DH1_ = 0,
    _1_DH3_,
    _1_DH5_,
    _2_DH1_,
    _2_DH3_,
    _2_DH5_,
    _3_DH1_,
    _3_DH3_,
    _3_DH5_,
};


enum EDR_DATA_TYPE {
    EDR_PRBS9 = 0,
    EDR_SINGLE_CARRIER,
};


enum BLE_PACKET_TYPE {
    BLE_PRBS9 = 0,
    _11110000_,
    _10101010_,
    BLE_PRBS15,
    _11111111_,
    _00000000_,
    _00001111_,
    _01010101_,
    BLE_SINGLE_CARRIER = 0xF0,
};


enum LE_PHY_TYPE {
    LE_1M_PHY = 1,
    LE_2M_PHY,
    LE_CODED_PHY_S8,
    LE_CODED_PHY_S2,
};


enum MODULATION_INDEX {
    STANDARD = 0,
    STABLE,
};


#pragma pack (1)
struct FCC_EDR_TX {
    u8 mode;
    u8 channel;
    u8 packet_type;
    u8 data_type;
    u8 tx_pwr;
    u8 hopping;

};
#pragma pack ()


#pragma pack (1)
struct FCC_EDR_RX {
    u8 mode;
    u8 channel;

};
#pragma pack ()


#pragma pack (1)
struct FCC_BLE_TX {
    u8 channel;
    u8 packet_len;
    u8 packet_type;
    u8 phy_type;
};
#pragma pack ()


#pragma pack (1)
struct FCC_BLE_RX {
    u8 channel;
    u8 phy_type;
    u8 modulation;
};
#pragma pack ()


typedef unsigned int UINT32;
typedef void (*usb_complete_t)(struct urb *);

typedef	struct {
    /* Word0 */ //modify by shunjian
//	UINT32		WIV:1;	/* Wireless Info Valid. 1 if Driver already fill WI,  o if DMA needs to copy WI to correctposition */
//	UINT32		QSEL:2;	/* select on-chip FIFO ID for 2nd-stage output scheduler.0:MGMT, 1:HCCA 2:EDCA */
//	UINT32		reserv:29;

    /* Word1 */
    /* ex: 00 03 00 40 means txop = 3, PHYMODE = 1 */
    UINT32		FRAG: 1;		/* 1 to inform TKIP engine this is a fragment. */
    UINT32		MIMOps: 1;	/* the remote peer is in dynamic MIMO-PS mode */
    UINT32		CFACK: 1;
    UINT32		TS: 1;

    UINT32		AMPDU: 1;
    UINT32		MpduDensity: 3;
    UINT32		txop: 2;	/*FOR "THIS" frame. 0:HT TXOP rule , 1:PIFS TX ,2:Backoff, 3:sifs only when previous frame exchange is successful. */
    UINT32		rsv: 6;

    UINT32		MCS: 7;
    UINT32		BW: 1;	/*channel bandwidth 20MHz or 40 MHz */
    UINT32		ShortGI: 1;
    UINT32		STBC: 2;	/* 1: STBC support MCS =0-7,   2,3 : RESERVE */
    UINT32		Ifs: 1;
    UINT32		rsv2: 2;	/*channel bandwidth 20MHz or 40 MHz */
    UINT32		PHYMODE: 2;
    /* Word2 */
    /* ex:  1c ff 38 00 means ACK=0, BAWinSize=7, MPDUtotalByteCount = 0x38 */
    UINT32		ACK: 1;
    UINT32		NSEQ: 1;
    UINT32		BAWinSize: 6;
    UINT32		WirelessCliID: 8;
    UINT32		MPDUtotalByteCount: 12;
    UINT32		PacketId: 4;
    /*Word3 */
    UINT32		IV;
    /*Word4 */
    UINT32		EIV;
}	TXWI_STRUC, *PTXWI_STRUC;


typedef	struct {
    /* Word	0 */
    UINT32		WirelessCliID: 8;
    UINT32		KeyIndex: 2;
    UINT32		BSSID: 3;
    UINT32		UDF: 3;
    UINT32		MPDUtotalByteCount: 12;
    UINT32		TID: 4;
    /* Word	1 */
    UINT32		FRAG: 4;
    UINT32		SEQUENCE: 12;
    UINT32		MCS: 7;
    UINT32		BW: 1;
    UINT32		ShortGI: 1;
    UINT32		STBC: 2;
    UINT32		rsv: 3;
    UINT32		PHYMODE: 2;             /* 1: this RX frame is unicast to me */
    /*Word2 */
    UINT32		RSSI0: 8;
    UINT32		RSSI1: 8;
    UINT32		RSSI2: 8;
    UINT32		rsv1: 8;
    /*Word3 */
    UINT32		SNR0: 8;
    UINT32		SNR1: 8;
    UINT32		FOFFSET: 8;
    UINT32		rsv2: 8;
    /*UINT32		rsv2:16;*/
}	RXWI_STRUC, *PRXWI_STRUC;


struct urb {
    /* private: usb core and host controller only fields in the urb */
    unsigned int pipe;		/* (in) pipe information */
    int status;			/* (return) non-ISO status */
    void *transfer_buffer;		/* (in) associated data buffer */
    u32 transfer_buffer_length;	/* (in) data buffer length */
    u32 actual_length;		/* (return) actual transfer length */
    void *context;			/* (in) context for completion */
    usb_complete_t complete;	/* (in) completion routine */
};


#endif



