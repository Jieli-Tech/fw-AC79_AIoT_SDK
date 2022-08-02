/************************************************Copyright(c)***********************************
**                                   Quintic(Nanjing) Microelectronics Co,Ltd.
**
**                                   http://www.quinticcorp.com
**
**--------------------File Info----------------------------------------------------------------
** File Name:                  qndriver.h
** subversion number:   160
**----------------------------------------------------------------------------------------
************************************************************************************************/

#ifndef _QN_0835_H
#define _QN_0835_H

#if TCFG_FM_QN8035_ENABLE

#define PILOT_CCA                       1  // 0: disable stereo judgement for the country has many mono FM stations
#define CCA_PILOT_SNR_FILTER            35 //25

#define FREQ2CHREG(freq)   ((freq-6000)/5)

/**********************************QN8035's clock source selection**************
1.QN8035's default clock source is 32768HZ.
2.setting QN8035's clock source and clock source type(like sine-wave clock or digital clock).
3.user need to modify clock source according to actual hardware platform.
4.clock formula,the details please refer to the QN8035's datasheet
  XTAL_DIV = Round(Clock/32768);
  PLL_DLT = Round((28500000*512*XTAL_DIV)/Clock)-442368
*******************************************************************************/
#define QND_SINE_WAVE_CLOCK             0x00    //inject sine-wave clock
#define QND_DIGITAL_CLOCK               0x80    //inject digital clock,default is inject digital clock

#ifdef QND_SINE_WAVE_CLOCK

#define QND_XTAL_DIV0                   0xDC
#define QND_XTAL_DIV1                   0x02
#define QND_XTAL_DIV2                   0x54

#else

#define QND_XTAL_DIV0                   0x01
#define QND_XTAL_DIV1                   0x08
#define QND_XTAL_DIV2                   0x5C

#endif

/*
//crystal clock is 32768HZ
#define QND_XTAL_DIV0                   0x01
#define QND_XTAL_DIV1                   0x08
#define QND_XTAL_DIV2                   0x5C

//crystal clock is 1MHZ
#define QND_XTAL_DIV0                   0x1F
#define QND_XTAL_DIV1                   0x00
#define QND_XTAL_DIV2                   0x38

//crystal clock is 4MHZ
#define QND_XTAL_DIV0                   0x7A
#define QND_XTAL_DIV1                   0x00
#define QND_XTAL_DIV2                   0x54

//crystal clock is 6MHZ
#define QND_XTAL_DIV0                   0xB7
#define QND_XTAL_DIV1                   0x00
#define QND_XTAL_DIV2                   0x54

//crystal clock is 7.3728MHZ
#define QND_XTAL_DIV0                   0xE1
#define QND_XTAL_DIV1                   0x08
#define QND_XTAL_DIV2                   0x5C

//crystal clock is 8MHZ
#define QND_XTAL_DIV0                   0xF4
#define QND_XTAL_DIV1                   0x00
#define QND_XTAL_DIV2                   0x54

//crystal clock is 10MHZ
#define QND_XTAL_DIV0                   0x31
#define QND_XTAL_DIV1                   0x01
#define QND_XTAL_DIV2                   0x54

//crystal clock is 11.0592MHZ
#define QND_XTAL_DIV0                   0x52
#define QND_XTAL_DIV1                   0xA1
#define QND_XTAL_DIV2                   0x70

//crystal clock is 12MHZ
#define QND_XTAL_DIV0                   0x6E
#define QND_XTAL_DIV1                   0x01
#define QND_XTAL_DIV2                   0x54

//crystal clock is 16MHZ
#define QND_XTAL_DIV0                   0xE8
#define QND_XTAL_DIV1                   0x01
#define QND_XTAL_DIV2                   0x54

//crystal clock is 20MHZ
#define QND_XTAL_DIV0                   0x62
#define QND_XTAL_DIV1                   0x02
#define QND_XTAL_DIV2                   0x54

//crystal clock is 24MHZ
#define QND_XTAL_DIV0                   0xDC
#define QND_XTAL_DIV1                   0x02
#define QND_XTAL_DIV2                   0x54

//crystal clock is 26MHZ
#define QND_XTAL_DIV0                   0x19
#define QND_XTAL_DIV1                   0x03
#define QND_XTAL_DIV2                   0x54
*/



//following is definition step
#define QND_STEP_CONSTANT               0x40 //(QND_FSTEP_100KHZ << 6)
#define CHIPID_QN8035                   0x84

#define R_TXRX_MASK    0x30

#define QND_MODE_SLEEP      0
#define QND_MODE_WAKEUP     1
// RX / TX value is using upper 8 bit

#define QND_MODE_RX         0x8000
#define QND_MODE_TX         0x4000

#define QND_MODE_FM         0x0000



#define QND_CONFIG_VOLUME             0x07



#define CHIPSUBID_QN8035A0              0x01
#define CHIPSUBID_QN8035A1              0x02


enum {
    CCA_SENSITIVITY_LEVEL_0 = 0x1E06,
    CCA_SENSITIVITY_LEVEL_1 = 0x1E07,   //if using the pilot as CCA,reference this item.
    CCA_SENSITIVITY_LEVEL_2 = 0x1E08,
    CCA_SENSITIVITY_LEVEL_3 = 0x1E09,
    CCA_SENSITIVITY_LEVEL_4 = 0x1E0A,   //if not using the pilot as CCA,reference this item.
    CCA_SENSITIVITY_LEVEL_5 = 0x1E0B,
    CCA_SENSITIVITY_LEVEL_6 = 0x1E0C,
    CCA_SENSITIVITY_LEVEL_7 = 0x1E0D,
    CCA_SENSITIVITY_LEVEL_8 = 0x1E0E,
    CCA_SENSITIVITY_LEVEL_9 = 0x1E0F
};


/**********************************************************************************************
// Performance configuration
***********************************************************************************************/
#define SMSTART_VAL     12//19
#define HCCSTART_VAL    18//33
#define SNCSTART_VAL    51//55



/**********************************************************************************************
// limitation configuration
***********************************************************************************************/






//#define CCA_PILOT_SNR_FILTER            20//18
#define CCA_PILOT_READ_DELAY            80//60
#define CCA_PILOT_READ_COUNT            12//10
#define CCA_PILOT_READ_COUNT_DELAY      5// 2
#define CCA_PILOT_TH                    3



#define CCA_TIME_OUT					200


/**********************************************************************************************
 definition register
**********************************************************************************************/
#define SYSTEM1         				0x00
#define CCA             				0x01
#define SNR            					0x02
#define RSSISIG         				0x03
#define STATUS1         				0x04
#define CID1            				0x05
#define CID2            				0x06
#define	CH								0x07
#define	CH_START						0x08
#define	CH_STOP							0x09
#define	CH_STEP							0x0A
#define	RDSD0							0x0B
#define	RDSD1							0x0C
#define	RDSD2							0x0D
#define	RDSD3							0x0E
#define	RDSD4							0x0F
#define	RDSD5							0x10
#define	RDSD6							0x11
#define	RDSD7							0x12
#define	STATUS2							0x13
#define	VOL_CTL							0x14
#define	XTAL_DIV0						0x15
#define	XTAL_DIV1						0x16
#define	XTAL_DIV2						0x17
#define INT_CTRL						0x18
#define SMP_HLD_THRD					0x19
#define	RXAGC_GAIN						0x1A
#define GAIN_SEL						0x1B
#define	SYSTEM_CTL1						0x1C
#define	SYSTEM_CTL2						0x1D
#define RDSCOSTAS						0x1E
#define REG_TEST						0x1F
#define STATUS4							0x20
#define RDSAGC2							0x21
#define	CCA1							0x27
#define	CCA2							0x28
#define	CCA3							0x29
#define	CCA4							0x2A
#define	CCA5							0x2B
#define PLT1                            0X2F
#define	PLT2                            0x30
#define	SMSTART							0x34
#define	SNCSTART						0x35
#define	HCCSTART						0x36
#define	CCA_CNT1					    0x37
#define	CCA_CNT2					    0x38
#define	CCA_SNR_TH_1					0x39
#define	CCA_SNR_TH_2					0x3A
#define NCCFIR3         				0x40
#define REG_REF                         0x49
#define REG_DAC							0x4C
/*******************************************************************************
 definition operation bit of register
*******************************************************************************/
#define CCA_CH_DIS      				0x01
#define CHSC            				0x02
#define RDSEN           				0x08
#define CH_CH		    				0x03
#define CH_CH_START     				0x0c
#define CH_CH_STOP      				0x30
#define STNBY_MODE           		    0x20
#define RX_MODE         				0x10
#define IMR             				0x40
#define RDS_RXUPD       				0x80
#define ST_MO_RX                        0x01
#define STNBY_RX_MASK                   0x30
#define RXCCA_MASK                      0x03
#define RX_CCA                          0x02
#define RXCCA_FAIL                      0x08
#define RX_MONO                         0x04
#define ICPREF                          0x0F


#define QND_READ(adr)    QND_ReadReg(adr)
#define QND_WRITE(adr, value)  QND_WriteReg(adr, value)


#define QN_IIC_read(a)  \
  QND_ReadReg(a)

#endif

#endif

