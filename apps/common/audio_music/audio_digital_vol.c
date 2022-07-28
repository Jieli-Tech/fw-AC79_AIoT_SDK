#include "audio_digital_vol.h"

#define DIGITAL_FADE_EN 	1
#define DIGITAL_FADE_STEP 	4

#define ASM_ENABLE			1
#define L_sat(b,a)          __asm__ volatile("%0=sat16(%1)(s)":"=&r"(b) : "r"(a));
#define L_sat32(b,a,n)      __asm__ volatile("%0=%1>>%2(s)":"=&r"(b) : "r"(a),"r"(n));

struct digital_volume {
    u16 *user_vol_tab;	/*自定义音量表*/
    u8 user_vol_max;            /*自定义音量表级数*/
    u8 ch_num;
    u8 toggle;					/*数字音量开关*/
    u8 fade;					/*淡入淡出标志*/
    u8 vol;						/*淡入淡出当前音量*/
    u8 vol_max;					/*淡入淡出最大音量*/
    s16 vol_fade;				/*淡入淡出对应的起始音量*/
    volatile s16 vol_target;	/*淡入淡出对应的目标音量*/
    volatile u16 fade_step;		/*淡入淡出的步进*/
    OS_MUTEX mutex;
    u32 fade_step_ms;           /*淡入淡出的步进对应的延迟*/
    u32 fade_offset;            /*淡入淡出的步进对应的缓存偏移*/
};

/*
 *数字音量级数 DIGITAL_VOL_MAX
 *数组长度 DIGITAL_VOL_MAX + 1
 */
#define DIGITAL_VOL_MAX		31
static const u16 dig_vol_table[DIGITAL_VOL_MAX + 1] = {
    0	, //0
    93	, //1
    111	, //2
    132	, //3
    158	, //4
    189	, //5
    226	, //6
    270	, //7
    323	, //8
    386	, //9
    462	, //10
    552	, //11
    660	, //12
    789	, //13
    943	, //14
    1127, //15
    1347, //16
    1610, //17
    1925, //18
    2301, //19
    2751, //20
    3288, //21
    3930, //22
    4698, //23
    5616, //24
    6713, //25
    8025, //26
    9592, //27
    11466,//28
    15200,//29
    16000,//30
    16384 //31
};

/*
 *fade_step一般不超过两级数字音量的最小差值
 *(1)通话如果用数字音量，一般步进小一点，音量调节的时候不会有杂音
 *(2)淡出的时候可以快一点，尽快淡出到0
 */
void *user_audio_digital_volume_open(u8 vol, u8 vol_max, u16 fade_step)
{
    struct digital_volume *d_volume = zalloc(sizeof(struct digital_volume));
    if (!d_volume) {
        return NULL;
    }

    u8 vol_level;
    d_volume->fade 		= 0;
    d_volume->vol 		= vol;
    d_volume->vol_max 	= vol_max;
    vol_level 			= vol * DIGITAL_VOL_MAX / vol_max;
    d_volume->vol_target = dig_vol_table[vol_level];
    //d_volume->vol_fade 	= 0;//d_volume->vol_target;//打开时，从0开始淡入
    d_volume->vol_fade 	= d_volume->vol_target;
    d_volume->fade_step 	= fade_step;
    d_volume->toggle 	= 1;
    d_volume->ch_num    = 2;//默认双声道
    d_volume->user_vol_tab = NULL;
    d_volume->user_vol_max = 0;
    d_volume->fade_step_ms = 10;

    os_mutex_create(&d_volume->mutex);
    printf("digital_vol_open:%d-%d-%d\n", vol, vol_max, fade_step);

    return d_volume;
}

int user_audio_digital_volume_close(void *_d_volume)
{
    struct digital_volume *d_volume = (struct digital_volume *)_d_volume;
    if (!d_volume) {
        return -1;
    }

    os_mutex_pend(&d_volume->mutex, 0);
    d_volume->toggle = 0;
    d_volume->user_vol_tab = NULL;
    d_volume->user_vol_max = 0;
    free(d_volume);
    printf("digital_vol_close\n");

    return 0;
}

u8 user_audio_digital_volume_get(void *_d_volume)
{
    struct digital_volume *d_volume = (struct digital_volume *)_d_volume;

    if (!d_volume) {
        return 0;
    }

    return d_volume->vol;
}

int user_audio_digital_volume_set(void *_d_volume, u8 vol)
{
    struct digital_volume *d_volume = (struct digital_volume *)_d_volume;

    if (!d_volume) {
        return -1;
    }

    u8 vol_level;
    if (d_volume->toggle == 0) {
        return 0;
    }

    os_mutex_pend(&d_volume->mutex, 0);
    d_volume->vol = vol;
    d_volume->fade = DIGITAL_FADE_EN;
    if (!d_volume->user_vol_tab) {
        vol_level = d_volume->vol * DIGITAL_VOL_MAX / d_volume->vol_max;
        d_volume->vol_target = dig_vol_table[vol_level];
    } else {
        u8 d_vol_max = d_volume->user_vol_max - 1;
        vol_level = d_volume->vol * d_vol_max / d_volume->vol_max;
        d_volume->vol_target = d_volume->user_vol_tab[vol_level];
    }
    os_mutex_post(&d_volume->mutex);

    /* printf("digital_vol:%d-%d-%d-%d\n", vol, vol_level, d_volume->vol_fade, d_volume->vol_target); */

    return 0;
}

int user_audio_digital_volume_reset_fade(void *_d_volume)
{
    struct digital_volume *d_volume = (struct digital_volume *)_d_volume;

    if (!d_volume) {
        return -1;
    }

    os_mutex_pend(&d_volume->mutex, 0);
    d_volume->vol_fade = 0;
    d_volume->fade_offset = 0;
    os_mutex_post(&d_volume->mutex);

    return 0;
}

#if ASM_ENABLE

#define  audio_vol_mix(data,len,ch_num,volumeNOW,volumeDest,step){  \
	int i, j;              \
	int fade = 0;             \
	if (volumeNOW != volumeDest)  \
	{                         \
		fade = 1;               \
	}            \
	if(d_volume->fade == 0)\
	{\
		fade = 0;\
		d_volume->vol_fade = d_volume->vol_target;\
	}\
	if (ch_num == 2)  \
	{                         \
		len = len >> 1;            \
	}                              \
	else if (ch_num == 3)         \
	{                                \
		len = (len*5462)>>14;             /*等效除3，因为5462向上取整得到的*/\
	}      \
	else if(ch_num == 4)               \
	{                \
		len = len >> 2; \
	}        \
	if (fade)            \
	{                           \
		short *in_ptr = data;              \
		for (i = 0; i < len; i++)               \
		{                                      \
			if (volumeNOW < volumeDest)            \
			{                                         \
				volumeNOW = volumeNOW + step;           \
				if (volumeNOW > volumeDest)              \
				{                                 \
					volumeNOW = volumeDest;          \
				}                                 \
			}         \
			else if (volumeNOW > volumeDest)    \
			{     \
				volumeNOW = volumeNOW - step; \
				if (volumeNOW < volumeDest)         \
				{      \
					volumeNOW = volumeDest; \
				} \
			}  \
			{                \
				int tmp;     \
				int reptime = ch_num;  \
				__asm__ volatile(  \
					" 1 : \n\t"         \
					" rep %0 {  \n\t"  \
					"   %1 = h[%2](s) \n\t" \
					"   %1 =%1* %3  \n\t "\
					"   %1 =%1 >>>14 \n\t"\
					"   h[%2++=2]= %1 \n\t"\
					" }\n\t" \
					" if(%0!=0 )goto 1b \n\t" \
					: "=&r"(reptime),      \
					"=&r"(tmp),        \
					"=&r"(in_ptr)  \
					:"r"(volumeNOW),  \
					"0"(reptime),\
					"2"(in_ptr)\
					: "cc", "memory");  \
			} \
		} \
	}  \
	else  \
	{  \
		for (i = 0; i < ch_num; i++) \
		{  \
			short *in_ptr = &data[i]; \
			{  \
				int tmp;  \
				int chnumv=ch_num*2;\
				int reptime = len;\
				__asm__ volatile(\
					" 1 : \n\t"\
					" rep %0 {  \n\t"\
					"   %1 = h[%2](s) \n\t"\
					"   %1 = %1 *%3  \n\t "\
					"   %1=  %1 >>>14 \n\t"\
					"   h[%2++=%4]= %1 \n\t"\
					" }\n\t"\
					" if(%0!=0 )goto 1b \n\t"\
					: "=&r"(reptime),\
					"=&r"(tmp),\
					"=&r"(in_ptr) \
					:"r"(volumeNOW),  \
					"r"(chnumv),\
					"0"(reptime),\
					"2"(in_ptr)\
					: "cc", "memory");\
			}  \
		}\
	}\
}

#else

#define  audio_vol_mix(data,len,ch_num,volumeNOW,volumeDest,step){  \
	int i, j;              \
	int fade = 0;             \
	if (volumeNOW != volumeDest)  \
	{                         \
		fade = 1;               \
	}            \
	if(d_volume->fade == 0)\
	{\
		fade = 0;\
		d_volume->vol_fade = d_volume->vol_target;\
	}\
	if (ch_num == 2)  \
	{                         \
		len = len >> 1;            \
	}                              \
	else if (ch_num == 3)         \
	{                                \
		len = (len*5462)>>14;             /*等效除3，因为5462向上取整得到的*/\
	}      \
	else if(ch_num == 4)               \
	{                \
		len = len >> 2; \
	}        \
	if (fade)            \
	{                           \
		short *in_ptr = data;              \
		for (i = 0; i < len; i++)               \
		{                                      \
			if (volumeNOW < volumeDest)            \
			{                                         \
				volumeNOW = volumeNOW + step;           \
				if (volumeNOW > volumeDest)              \
				{                                 \
					volumeNOW = volumeDest;          \
				}                                 \
			}         \
			else if (volumeNOW > volumeDest)    \
			{     \
				volumeNOW = volumeNOW - step; \
				if (volumeNOW < volumeDest)         \
				{      \
					volumeNOW = volumeDest; \
				} \
			}  \
			for (j = 0; j < ch_num; j++)  \
			{          \
				int tmp = (*in_ptr*volumeNOW) >> 14;  \
				L_sat(tmp, tmp);  \
				*in_ptr = tmp; \
				in_ptr++; \
			} \
		} \
	}  \
	else  \
	{  \
		for (i = 0; i < ch_num; i++) \
		{  \
			short *in_ptr = &data[i]; \
			for (j = 0; j < len; j++)\
			{\
				int tmp= (*in_ptr*volumeNOW) >> 14;  \
				L_sat(tmp, tmp);  \
				*in_ptr = tmp;\
				in_ptr += ch_num;\
			}\
		}\
	}\
}

#endif

int user_audio_digital_volume_run(void *_d_volume, void *data, u32 len, u32 sample_rate, u8 ch_num)
{
    struct digital_volume *d_volume = (struct digital_volume *)_d_volume;

    if (!d_volume) {
        return -1;
    }

    s32 valuetemp;
    s16 *buf = (s16 *)data;
    len >>= 1; //byte to point

    if (d_volume->toggle == 0) {
        return -1;
    }
    if (ch_num > 4) {
        return -1;
    }

    /* os_mutex_pend(&d_volume->mutex, 0); */

    if (ch_num) {
        d_volume->ch_num = ch_num;
    }

    /* printf("d_volume->vol_target %d %d %d %d\n", d_volume->vol_target, ch_num, d_volume->vol_fade, d_volume->fade_step); */
#if 1

    u32 rdlen = 0, __rdlen = 0;
    u32 fade_step_len = d_volume->fade_step_ms * sample_rate * ch_num / 1000;

    for (u32 n = 0; n < len;) {
        rdlen = fade_step_len - d_volume->fade_offset;
        if (rdlen > len - n) {
            rdlen = len - n;
        }
        d_volume->fade_offset += rdlen;
        __rdlen = rdlen;

        if (d_volume->fade_offset == fade_step_len) {
            audio_vol_mix((buf + n), __rdlen, ch_num, d_volume->vol_fade, d_volume->vol_target, d_volume->fade_step);
            d_volume->fade_offset = 0;
        } else {
            audio_vol_mix((buf + n), __rdlen, ch_num, d_volume->vol_fade, d_volume->vol_fade, d_volume->fade_step);
        }

        n += rdlen;
    }

#else

    /* printf("d_volume->vol_target %d %d\n", d_volume->vol_target, ch_num); */
    for (u32 i = 0; i < len; i += d_volume->ch_num) {//ch_num 1/2/3/4
        ///FL channel
        if (d_volume->fade) {
            if (d_volume->vol_fade > d_volume->vol_target) {
                d_volume->vol_fade -= d_volume->fade_step;
                if (d_volume->vol_fade < d_volume->vol_target) {
                    d_volume->vol_fade = d_volume->vol_target;
                }
            } else if (d_volume->vol_fade < d_volume->vol_target) {
                d_volume->vol_fade += d_volume->fade_step;
                if (d_volume->vol_fade > d_volume->vol_target) {
                    d_volume->vol_fade = d_volume->vol_target;
                }
            }
        } else {
            d_volume->vol_fade = d_volume->vol_target;
        }

        valuetemp = buf[i];
        if (valuetemp < 0) {
            valuetemp = -valuetemp;
            valuetemp = (valuetemp * d_volume->vol_fade) >> 14 ;
            valuetemp = -valuetemp;
        } else {
            valuetemp = (valuetemp * d_volume->vol_fade) >> 14 ;
        }
        if (valuetemp < -32768) {
            valuetemp = -32768;
        } else if (valuetemp > 32767) {
            valuetemp = 32767;
        }
        buf[i] = (s16)valuetemp;

        if (d_volume->ch_num > 1) { //双声道
            ///FR channel
            valuetemp = buf[i + 1];
            if (valuetemp < 0) {
                valuetemp = -valuetemp;
                valuetemp = (valuetemp * d_volume->vol_fade) >> 14 ;
                valuetemp = -valuetemp;
            } else {
                valuetemp = (valuetemp * d_volume->vol_fade) >> 14 ;
            }

            if (valuetemp < -32768) {
                valuetemp = -32768;
            } else if (valuetemp > 32767) {
                valuetemp = 32767;
            }
            buf[i + 1] = (s16)valuetemp;

            if (d_volume->ch_num > 2) { //三声道
                //RL channel
                valuetemp = buf[i + 2];
                if (valuetemp < 0) {
                    valuetemp = -valuetemp;
                    valuetemp = (valuetemp * d_volume->vol_fade) >> 14 ;
                    valuetemp = -valuetemp;
                } else {
                    valuetemp = (valuetemp * d_volume->vol_fade) >> 14 ;
                }
                if (valuetemp < -32768) {
                    valuetemp = -32768;
                } else if (valuetemp > 32767) {
                    valuetemp = 32767;
                }
                buf[i + 2] = (s16)valuetemp;


                if (d_volume->ch_num > 3) { //四声道
                    ///RR channel
                    valuetemp = buf[i + 3];
                    if (valuetemp < 0) {
                        valuetemp = -valuetemp;
                        valuetemp = (valuetemp * d_volume->vol_fade) >> 14 ;
                        valuetemp = -valuetemp;
                    } else {
                        valuetemp = (valuetemp * d_volume->vol_fade) >> 14 ;
                    }
                    if (valuetemp < -32768) {
                        valuetemp = -32768;
                    } else if (valuetemp > 32767) {
                        valuetemp = 32767;
                    }
                    buf[i + 3] = (s16)valuetemp;
                }
            }
        }
    }
#endif

    /* os_mutex_post(&d_volume->mutex); */

    return 0;
}

/*
 *user_vol_max:音量级数
 *user_vol_tab:自定义音量表,自定义表长user_vol_max+1
 */
void user_audio_digital_set_volume_tab(void *_d_volume, u16 *user_vol_tab, u8 user_vol_max)
{
    struct digital_volume *d_volume = (struct digital_volume *)_d_volume;

    if (!d_volume) {
        return;
    }

    os_mutex_pend(&d_volume->mutex, 0);
    if (user_vol_tab) {
        d_volume->user_vol_tab = user_vol_tab;
        d_volume->user_vol_max = user_vol_max;
    }
    os_mutex_post(&d_volume->mutex);
}

