#include "system/includes.h"
#include "app_config.h"
#include "asm/eq.h"
#include <math.h>

#define LOG_TAG     "[APP-EQ]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#include "database.h"
#include "fs/fs.h"
#include "asm/crc16.h"

#if (TCFG_EQ_ENABLE == 1)

#define SDK_NAME			"AC790N"
static const u8 audio_eq_ver[4] = {0, 8, 0, 1};

#if TCFG_LIMITER_ENABLE
extern int need_limiter_buf(void);
extern void limiter_init(void *work_buf, int *attackTime, int *releaseTime, int *threshold, int sample_rate, int channel);
extern int limiter_run_16(void *work_buf, short *in_buf, short *out_buf, int per_channel_npoint);
extern int limiter_run_32(void *work_buf, int *in_buf, int *out_buf, int per_channel_npoint);
#endif


/************************ sw drc *************************/
#define DRC_TYPE_LIMITER		1
#define DRC_TYPE_COMPRESSOR		2

struct drc_limiter {
    int attacktime;
    int releasetime;
    int threshold[2];//threshold[1]是固定值32768, threshold[0]为界面参数
};

struct drc_compressor {
    int attacktime;
    int releasetime;
    int threshold[3];//threshold[2]是固定值32768
    int ratio[3];// ratio[0]为固定值100， ratio[1] ratio[2]为界面参数
};

struct drc_ch {
    u8 nband;//max<=3，1：全带 2：两段 3：三段
    u8 type;//0:没有使能限幅和压缩器，1:限幅器   2:压缩器
    u8 reserved[2];//保留,未用
    int low_freq;//中低频分频点
    int high_freq;//中高频分频点
    int order;//分频器阶数, 2或者4
    union {
        struct drc_limiter		limiter[3];
        struct drc_compressor	compressor[3];
    } parm;
};
/*********************************************************/


/*-----------------------------------------------------------*/
/*eq type*/
typedef enum {
    EQ_MODE_NORMAL = 0,
    EQ_MODE_ROCK,
    EQ_MODE_POP,
    EQ_MODE_CLASSIC,
    EQ_MODE_JAZZ,
    EQ_MODE_COUNTRY,
    EQ_MODE_MAX,
} EQ_MODE;

/*eq type*/
typedef enum {
    EQ_TYPE_FILE = 0x01,
    EQ_TYPE_ONLINE,
    EQ_TYPE_MODE_TAB,
} EQ_TYPE;

/*eq IIR type*/
typedef enum {
    EQ_IIR_TYPE_HIGH_PASS = 0x00,
    EQ_IIR_TYPE_LOW_PASS,
    EQ_IIR_TYPE_BAND_PASS,
    EQ_IIR_TYPE_HIGH_SHELF,
    EQ_IIR_TYPE_LOW_SHELF,
} EQ_IIR_TYPE;

/*eq magic*/
typedef enum {
    MAGIC_EQ_COEFF = 0xA5A0,
    MAGIC_EQ_LIMITER,
    MAGIC_EQ_SOFT_SEC,
    MAGIC_EQ_SEG = 0xA6A1,
    MAGIC_DRC,
    MAGIC_EQ_MAX,
} EQ_MAGIC;

/*eq online cmd*/
typedef enum {
    EQ_ONLINE_CMD_SECTION       = 1,
    EQ_ONLINE_CMD_GLOBAL_GAIN,
    EQ_ONLINE_CMD_LIMITER,
    EQ_ONLINE_CMD_INQUIRE,
    EQ_ONLINE_CMD_GETVER,
    EQ_ONLINE_CMD_GET_SOFT_SECTION,
    EQ_ONLINE_CMD_GLOBAL_GAIN_SUPPORT_FLOAT = 0x8,

    EQ_ONLINE_CMD_PARAMETER_SEG = 0x11,
    EQ_ONLINE_CMD_PARAMETER_TOTAL_GAIN,
    EQ_ONLINE_CMD_PARAMETER_LIMITER,
    EQ_ONLINE_CMD_PARAMETER_DRC,
} EQ_ONLINE_CMD;

/*eq file seg head*/
typedef struct {
    unsigned short crc;
    unsigned short seg_num;
    float global_gain;
    int enable_section;
} _GNU_PACKED_ EQ_FILE_SEG_HEAD;

/*eq file limiter head*/
typedef struct {
    unsigned short crc;
    unsigned short re;
    float AttackTime;
    float ReleaseTime;
    float Threshold;
    int Enable;
} _GNU_PACKED_ LIMITER_CFG_HEAD;

/*eq online packet*/
typedef struct {
    u8 cmd;     			///<EQ_ONLINE_CMD
    u8 seg_num;          	///<which section select
    u16 seg_status;			///<selected section status
    int data[45];       	///<data
} _GNU_PACKED_ EQ_ONLINE_PACKET;

/*eq seg info*/
struct eq_seg_info {
    u16 index;
    u16 iir_type; ///<EQ_IIR_TYPE
    int freq;
    int gain;
    int q;
};

struct audio_eq_filter_info {
    u8 nsection;
    u8 soft_sec;
    int *L_coeff;
    int *R_coeff;
    float L_gain;
    float R_gain;
};

/*EQ_ONLINE_CMD_PARAMETER_SEG*/
typedef struct eq_seg_info EQ_ONLINE_PARAMETER_SEG;

/*EQ_ONLINE_CMD_PARAMETER_TOTAL_GAIN*/
typedef struct {
    float gain;
} _GNU_PACKED_ EQ_ONLINE_PARAMETER_TOTAL_GAIN;

/*EQ_ONLINE_CMD_PARAMETER_LIMITER*/
typedef struct {
    int enable;
    int attackTime;
    int releaseTime;
    int threshold;
} _GNU_PACKED_ EQ_ONLINE_PARAMETER_LIMITER;

typedef struct eq_seg_info EQ_CFG_SEG;
typedef struct {
    float global_gain;
    EQ_CFG_SEG seg[EQ_SECTION_MAX];
#if TCFG_DRC_ENABLE
    struct drc_ch drc;
#endif
#if TCFG_LIMITER_ENABLE
    EQ_ONLINE_PARAMETER_LIMITER limiter;
#endif
} EQ_CFG_PARAMETER;

typedef struct {
    u32 eq_mode : 3;
    u32 eq_type : 3;
    u32 soft_sec : 3;
    u32 soft_sec_max : 3;
    u32 seg_num : 6;
    u32 online_need_updata : 1;
    u32 online_updata : 1;
    u32 mode_updata : 1;
    u32 drc_updata : 1;
    u32 limiter_updata : 1;
    u32 reserved : 9;
    u32 cur_sr;
    u32 design_mask;
    spinlock_t lock;
    EQ_CFG_PARAMETER param;
    int EQ_Coeff_table[EQ_SECTION_MAX * 5];
#if TCFG_LIMITER_ENABLE
    void *limiter_buf;
#endif
} EQ_CFG;

static EQ_CFG           eq_cfg;

#define __this          (&eq_cfg)

static const EQ_CFG_SEG eq_seg_nor = {
    0, 2, 1000, 0 << 20, (int)(0.7f * (1 << 24))
};

extern void design_lp(int fc, int fs, int quality_factor, int *coeff);
extern void design_hp(int fc, int fs, int quality_factor, int *coeff);
extern void design_pe(int fc, int fs, int gain, int quality_factor, int *coeff);
extern void design_ls(int fc, int fs, int gain, int quality_factor, int *coeff);
extern void design_hs(int fc, int fs, int gain, int quality_factor, int *coeff);
extern int eq_stable_check(int *coeff);
extern int eq_cfg_sync(u8 priority);
extern void *get_eq_hdl(void);

static void eq_seg_design(EQ_CFG_SEG *seg, int sample_rate, int *coeff)
{
    /* printf("seg:0x%x, coeff:0x%x, rate:%d, ", seg, coeff, sample_rate); */
    /* printf("idx:%d, iir:%d, freq:%d, gain:%d, q:%d ", seg->index, seg->iir_type, seg->freq, seg->gain, seg->q); */
    if (seg->freq >= (((u32)sample_rate / 2 * 29491) >> 15)) {
        /* log_error("eq seg design freq:%d err", seg->freq); */
        eq_get_AllpassCoeff(coeff);
        return;
    }

    switch (seg->iir_type) {
    case EQ_IIR_TYPE_HIGH_PASS:
        design_hp(seg->freq, sample_rate, seg->q, coeff);
        break;
    case EQ_IIR_TYPE_LOW_PASS:
        design_lp(seg->freq, sample_rate, seg->q, coeff);
        break;
    case EQ_IIR_TYPE_BAND_PASS:
        design_pe(seg->freq, sample_rate, seg->gain, seg->q, coeff);
        break;
    case EQ_IIR_TYPE_HIGH_SHELF:
        design_hs(seg->freq, sample_rate, seg->gain, seg->q, coeff);
        break;
    case EQ_IIR_TYPE_LOW_SHELF:
        design_ls(seg->freq, sample_rate, seg->gain, seg->q, coeff);
        break;
    }

    int status = eq_stable_check(coeff);
    if (status) {
        log_error("eq_stable_check err:%d ", status);
        log_info("%d %d %d %d %d", coeff[0], coeff[1], coeff[2], coeff[3], coeff[4]);
        eq_get_AllpassCoeff(coeff);
    }
}

static void eq_coeff_set(int sr)
{
    if (!sr) {
        sr = 44100;
        log_error("sr is zero");
    }

    for (u8 i = 0; i < __this->seg_num; i++) {
        if (__this->design_mask & BIT(i)) {
            __this->design_mask &= ~BIT(i);
            eq_seg_design(&__this->param.seg[i], sr, &__this->EQ_Coeff_table[5 * i]);
            /* printf("cf0:%d, cf1:%d, cf2:%d, cf3:%d, cf4:%d ", __this->EQ_Coeff_table[5*i] */
            /* , __this->EQ_Coeff_table[5*i + 1] */
            /* , __this->EQ_Coeff_table[5*i + 2] */
            /* , __this->EQ_Coeff_table[5*i + 3] */
            /* , __this->EQ_Coeff_table[5*i + 4] */
            /* ); */
        }
    }
}

/*-----------------------------------------------------------*/
// eq file
#if TCFG_EQ_FILE_ENABLE

#define MAGIC_FLAG_SEG  0
#define MAGIC_FLAG_SOFT 1
#define MAGIC_FLAG_DRC  2
#define MAGIC_FLAG_LIMITER  3

static s32 eq_file_get_cfg(EQ_CFG *eq_cfg)
{
    int magic;
    int ret = 0;
    int read_size;
    FILE *file = NULL;
    u8 *file_data = NULL;
    u8 magic_flag = 0;

    if (eq_cfg == NULL) {
        return -EINVAL;
    }

    file = fopen(CONFIG_EQ_FILE_NAME, "r");
    if (file == NULL) {
        log_error("eq file open err\n");
        return -ENOENT;
    }
    log_info("eq file open ok \n");

    // eq ver
    u8 ver[4] = {0};
    if (4 != fread(ver, 1, 4, file)) {
        ret = -EIO;
        goto err_exit;
    }
    if (memcmp(ver, audio_eq_ver, sizeof(audio_eq_ver))) {
        log_info("eq ver err \n");
        put_buf(ver, 4);
        ret = -EINVAL;
        goto err_exit;
        /* fseek(file, 0, SEEK_SET); */
    }

    while (1) {
        if (sizeof(int) != fread(&magic, 1, sizeof(int), file)) {
            ret = 0;
            break;
        }
        log_info("eq magic 0x%x\n", magic);
        if ((magic >= MAGIC_EQ_COEFF) && (magic < MAGIC_EQ_MAX)) {
            if (magic == MAGIC_EQ_SOFT_SEC) {
#if TCFG_HW_SOFT_EQ_ENABLE
                int sec = 0;
                if (sizeof(int) != fread(&magic, 1, sizeof(int), file)) {
                    ret = -EIO;
                    break;
                }
                log_info("eq soft sec:%d \n", sec);
                if (sec > eq_cfg->soft_sec_max) {
                    log_error("eq max sec:%d \n", eq_cfg->soft_sec_max);
                    ret = -EINVAL;
                    break;
                }
                eq_cfg->soft_sec = sec;
                magic_flag |= BIT(MAGIC_FLAG_SOFT);
#else
                fseek(file, sizeof(int), SEEK_CUR);
#endif
            } else if (magic == MAGIC_EQ_SEG) {
                EQ_FILE_SEG_HEAD *eq_file_h;
                int cfg_zone_size = sizeof(EQ_FILE_SEG_HEAD) + EQ_SECTION_MAX * sizeof(EQ_CFG_SEG);
                file_data = malloc(cfg_zone_size);
                if (file_data == NULL) {
                    ret = -ENOMEM;
                    break;
                }

                if (sizeof(EQ_FILE_SEG_HEAD) != fread(file_data, 1, sizeof(EQ_FILE_SEG_HEAD), file)) {
                    ret = -EIO;
                    break;
                }
                eq_file_h = (EQ_FILE_SEG_HEAD *)file_data;
                log_info("cfg_seg_num:%d\n", eq_file_h->seg_num);
                log_info("cfg_global_gain:%f\n", eq_file_h->global_gain);
                log_info("cfg_enable_section:%x\n", eq_file_h->enable_section);

                if (eq_file_h->seg_num > EQ_SECTION_MAX) {
                    ret = -EINVAL;
                    break;
                }

                read_size = eq_file_h->seg_num * sizeof(EQ_CFG_SEG);
                if (read_size != fread(file_data + sizeof(EQ_FILE_SEG_HEAD), 1, read_size, file)) {
                    ret = -EIO;
                    break;
                }

                if (eq_file_h->crc == CRC16(&eq_file_h->seg_num, read_size + sizeof(EQ_FILE_SEG_HEAD) - 2)) {
                    log_info("eq_cfg_file crc ok\n");
                    spin_lock(&eq_cfg->lock);
                    eq_cfg->param.global_gain = eq_file_h->global_gain;
                    eq_cfg->seg_num = eq_file_h->seg_num;
                    memcpy(eq_cfg->param.seg, file_data + sizeof(EQ_FILE_SEG_HEAD), sizeof(EQ_CFG_SEG) * eq_file_h->seg_num);
                    eq_cfg->design_mask = (u32) - 1;
                    eq_coeff_set(eq_cfg->cur_sr);
                    spin_unlock(&eq_cfg->lock);
                    log_info("sr %d head size %d\n", eq_cfg->cur_sr, sizeof(EQ_FILE_SEG_HEAD));
                    /* printf_buf(eq_cfg->param.seg, sizeof(EQ_CFG_SEG) * eq_file_h->seg_num); */
                    ret = 0;
                } else {
                    log_error("eq_cfg_info crc err\n");
                    ret = -ENOEXEC;
                }

                free(file_data);
                file_data = NULL;
                magic_flag |= BIT(MAGIC_FLAG_SEG);
            } else if (magic == MAGIC_EQ_COEFF) {
                EQ_FILE_SEG_HEAD eq_file;
                if (sizeof(EQ_FILE_SEG_HEAD) != fread(&eq_file, 1, sizeof(EQ_FILE_SEG_HEAD), file)) {
                    ret = -EIO;
                    break;
                }
                if (eq_file.seg_num > EQ_SECTION_MAX) {
                    ret = -EINVAL;
                    break;
                }
                fseek(file, eq_file.seg_num * 5 * sizeof(int) * 9, SEEK_CUR);
            } else if (magic == MAGIC_EQ_LIMITER) {
#if TCFG_LIMITER_ENABLE
                LIMITER_CFG_HEAD eq_limiter;
                if (sizeof(LIMITER_CFG_HEAD) != fread(&eq_limiter, 1, sizeof(LIMITER_CFG_HEAD), file)) {
                    ret = -EIO;
                    break;
                }
                if (eq_limiter.crc == CRC16(&eq_limiter.AttackTime, sizeof(LIMITER_CFG_HEAD) - 4)) {
                    spin_lock(&eq_cfg->lock);
                    eq_cfg->param.limiter.attackTime = eq_limiter.AttackTime;
                    eq_cfg->param.limiter.releaseTime = eq_limiter.ReleaseTime;
                    eq_cfg->param.limiter.threshold = eq_limiter.Threshold;
                    eq_cfg->param.limiter.enable = eq_limiter.Enable;
                    spin_unlock(&eq_cfg->lock);
                    magic_flag |= BIT(MAGIC_FLAG_LIMITER);
                }
#else
                fseek(file, sizeof(LIMITER_CFG_HEAD), SEEK_CUR);
#endif
            } else if (magic == MAGIC_DRC) {
#if TCFG_DRC_ENABLE
                u16 crc16;
                u32 pam = 0;
                struct drc_ch drc = {0};
                if (sizeof(pam) != fread(&pam, 1, sizeof(pam), file)) {
                    ret = -EIO;
                    break;
                }
                crc16 = pam;
                if (sizeof(struct drc_ch) != fread(&drc, 1, sizeof(struct drc_ch), file)) {
                    ret = -EIO;
                    break;
                }
                if (crc16 == CRC16(&drc, sizeof(struct drc_ch))) {
                    memcpy(&eq_cfg->param.drc, &drc, sizeof(struct drc_ch));
                    magic_flag |= BIT(MAGIC_FLAG_DRC);
                }
#else
                fseek(file, sizeof(struct drc_ch) + 4, SEEK_CUR);
#endif
            }
        } else {
            /* log_error("cfg_info magic err\n"); */
            /* ret = -EINVAL; */
            ret = 0;
            break;
        }
    }

err_exit:
    if (file_data) {
        free(file_data);
    }
    fclose(file);
    if (ret == 0) {
        if (!(magic_flag & BIT(MAGIC_FLAG_SEG))) {
            log_error("cfg_info coeff err\n");
            ret = -EINVAL;
        }
#if TCFG_HW_SOFT_EQ_ENABLE
        if (!(magic_flag & BIT(MAGIC_FLAG_SOFT))) {
            log_error("cfg_info soft err\n");
            ret = -EINVAL;
        }
#endif
#if TCFG_DRC_ENABLE
        if (!(magic_flag & BIT(MAGIC_FLAG_DRC))) {
            log_error("cfg_info drc err\n");
            /* ret = -EINVAL; */
        }
#endif
#if TCFG_LIMITER_ENABLE
        if (!(magic_flag & BIT(MAGIC_FLAG_LIMITER))) {
            log_error("cfg_info limiter err\n");
            ret = -EINVAL;
        }
#endif
    }
    if (ret == 0) {
        log_info("cfg_info ok \n");
    }

    return ret;
}

static int eq_file_get_filter_info(int sr, struct audio_eq_filter_info *info)
{
    if (!sr) {
        return -1;
    }

    spin_lock(&__this->lock);
    if (sr != __this->cur_sr) {
        //更新coeff
        /* if (eq_file_get_cfg(__this)) { */
        /* __this->cur_sr = 0; */
        /* return -1; */
        /* } */
        __this->cur_sr = sr;
        __this->design_mask = (u32) - 1;
        eq_coeff_set(sr);
    }
    spin_unlock(&__this->lock);

    info->L_coeff = info->R_coeff = (void *)__this->EQ_Coeff_table;
    info->L_gain = info->R_gain = __this->param.global_gain;
    info->nsection = __this->seg_num;
    info->soft_sec = __this->soft_sec;

    return 0;
}
#endif

/*-----------------------------------------------------------*/
// eq mode
#include "eq_tab.h"

static const EQ_CFG_SEG *const eq_mode_tab[EQ_MODE_MAX] = {
    eq_tab_normal, eq_tab_rock, eq_tab_pop, eq_tab_classic, eq_tab_jazz, eq_tab_country
};

static int eq_mode_get_filter_info(int sr, struct audio_eq_filter_info *info)
{
    if (!sr) {
        return -1;
    }

    spin_lock(&__this->lock);
    memcpy(__this->param.seg, eq_mode_tab[__this->eq_mode], sizeof(EQ_CFG_SEG) * EQ_SECTION_MAX);
    __this->design_mask = (u32) - 1;
    eq_coeff_set(sr);
    __this->cur_sr = 0;
    spin_unlock(&__this->lock);

    info->L_coeff = info->R_coeff = (void *)__this->EQ_Coeff_table;
    info->L_gain = info->R_gain = __this->param.global_gain;
    info->nsection = __this->seg_num;
    info->soft_sec = __this->soft_sec;

    return 0;
}

int eq_set_mode(mode)
{
    if (mode >= EQ_MODE_MAX) {
        mode = 0;
    }
    __this->eq_mode = mode;
    log_info("set eq mode %d\n", __this->eq_mode);

    if (__this->eq_type == EQ_TYPE_MODE_TAB) {
        __this->mode_updata = 1;
        eq_cfg_sync(0);
    }
    return 0;
}

int eq_sw_mode(void)
{
    __this->eq_mode++;
    if (__this->eq_mode >= EQ_MODE_MAX) {
        __this->eq_mode = 0;
    }
    log_info("sw eq mode %d\n", __this->eq_mode);

    if (__this->eq_type == EQ_TYPE_MODE_TAB) {
        __this->mode_updata = 1;
        eq_cfg_sync(0);
    }
    return 0;
}

int eq_get_cur_mode(void)
{
    return __this->eq_mode;
}

#if (TCFG_EQ_ONLINE_ENABLE == 1)

#include "syscfg/config_interface.h"

static s32 eq_online_update(EQ_ONLINE_PACKET *packet)
{
    EQ_ONLINE_PARAMETER_SEG seg;
    EQ_ONLINE_PARAMETER_TOTAL_GAIN gain;

    if (__this->eq_type != EQ_TYPE_ONLINE) {
        return -EPERM;
    }
    printf("eq_cmd:0x%x ", packet->cmd);

    switch (packet->cmd) {
    case EQ_ONLINE_CMD_PARAMETER_SEG:
        memcpy(&seg, packet->data, sizeof(EQ_ONLINE_PARAMETER_SEG));
        if (seg.index >= EQ_SECTION_MAX) {
            log_error("index:%d ", seg.index);
            return -EINVAL;
        }
        spin_lock(&__this->lock);
        memcpy(&__this->param.seg[seg.index], &seg, sizeof(EQ_ONLINE_PARAMETER_SEG));
        __this->design_mask |= BIT(seg.index);
        spin_unlock(&__this->lock);
        log_info("idx:%d, iir:%d, frq:%d, gain:%d, q:%d \n", seg.index, seg.iir_type, seg.freq, seg.gain, seg.q);
        break;
    case EQ_ONLINE_CMD_PARAMETER_TOTAL_GAIN:
        memcpy(&gain, packet->data, sizeof(EQ_ONLINE_PARAMETER_TOTAL_GAIN));
        spin_lock(&__this->lock);
        __this->param.global_gain = gain.gain;
        spin_unlock(&__this->lock);
        log_info("global_gain:%f\n", __this->param.global_gain);
        break;
    case EQ_ONLINE_CMD_INQUIRE:
        if (__this->online_need_updata) {
            __this->online_need_updata = 0;
            log_info("updata eq table\n");
            return 0;
        }
        return -EINVAL;
    case EQ_ONLINE_CMD_PARAMETER_LIMITER:
        log_info("EQ_ONLINE_CMD_PARAMETER_LIMITER");
#if TCFG_LIMITER_ENABLE
        spin_lock(&__this->lock);
        memcpy(&__this->param.limiter, packet->data, sizeof(EQ_ONLINE_PARAMETER_LIMITER));
        __this->limiter_updata = 1;
        spin_unlock(&__this->lock);
        if (__this->param.limiter.enable) {
            log_info("limiter.attackTime:%d\n", __this->param.limiter.attackTime);
            log_info("limiter.releaseTime:%d\n", __this->param.limiter.releaseTime);
            log_info("limiter.threshold:%d\n", __this->param.limiter.threshold);
        }
#endif
        break;
    case EQ_ONLINE_CMD_PARAMETER_DRC:
        log_info("EQ_ONLINE_CMD_PARAMETER_DRC");
#if TCFG_DRC_ENABLE
        spin_lock(&__this->lock);
        memcpy(&__this->param.drc, packet->data, sizeof(struct drc_ch));
        spin_unlock(&__this->lock);
#endif
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int eq_online_nor_cmd(EQ_ONLINE_PACKET *packet)
{
    if (__this->eq_type != EQ_TYPE_ONLINE) {
        return -EPERM;
    }
    if (packet->cmd == EQ_ONLINE_CMD_GETVER) {
        struct eq_ver_info {
            char sdkname[16];
            u8 eqver[4];
        };
        struct eq_ver_info eq_ver_info = {0};
        memcpy(eq_ver_info.eqver, audio_eq_ver, sizeof(audio_eq_ver));
        strcpy(eq_ver_info.sdkname, SDK_NAME);
        memcpy(eq_ver_info.eqver, audio_eq_ver, 4);
        ci_send_packet(EQ_CONFIG_ID, (u8 *)&eq_ver_info, sizeof(struct eq_ver_info));
        return 0;
    } else if (packet->cmd == EQ_ONLINE_CMD_GET_SOFT_SECTION) {
#if TCFG_HW_SOFT_EQ_ENABLE
        uint8_t soft_sec = __this->soft_sec_max;
        ci_send_packet(EQ_CONFIG_ID, &soft_sec, sizeof(uint8_t));
        return 0;
#endif
    } else if (packet->cmd == EQ_ONLINE_CMD_GLOBAL_GAIN_SUPPORT_FLOAT) {
        uint8_t support_float = 1;
        u32 id = 0x8;
        ci_send_packet(id, (uint8_t *)&support_float, sizeof(uint8_t));
        return 0;
    }

    return -EINVAL;
}

static void eq_online_callback(uint8_t *packet, uint16_t size)
{
    s32 res;

    ASSERT(((int)packet & 0x3) == 0, "buf %x size %d\n", packet, size);
    res = eq_online_update((EQ_ONLINE_PACKET *)packet);
    /* log_info("EQ payload"); */
    /* log_info_hexdump(packet, sizeof(EQ_ONLINE_PACKET)); */

    u32 id = EQ_CONFIG_ID;

    if (res == 0) {
        log_info("Ack");
        ci_send_packet(id, (u8 *)"OK", 2);
        EQ_ONLINE_PACKET *pkt = (EQ_ONLINE_PACKET *)packet;
#if TCFG_LIMITER_ENABLE
        if (pkt->cmd == EQ_ONLINE_CMD_PARAMETER_LIMITER) {
            return;
        }
#endif
#if TCFG_DRC_ENABLE
        if (pkt->cmd == EQ_ONLINE_CMD_PARAMETER_DRC) {
            __this->drc_updata = 1;
            return;
        }
#endif
        __this->online_updata = 1;
        eq_cfg_sync(0);
    } else {
        res = eq_online_nor_cmd((EQ_ONLINE_PACKET *)packet);
        if (res == 0) {
            return ;
        }
        /* log_info("Nack"); */
        ci_send_packet(id, (u8 *)"ER", 2);
    }
}

static int eq_online_get_filter_info(int sr, struct audio_eq_filter_info *info)
{
    if (!sr) {
        return -1;
    }
    /* if (__this->eq_type != EQ_TYPE_ONLINE) { */
    /* return -1; */
    /* } */
    spin_lock(&__this->lock);
    if ((sr != __this->cur_sr) || (__this->online_updata)) {
        //在线请求coeff
        if (sr != __this->cur_sr) {
            __this->design_mask = (u32) - 1;
        }
        eq_coeff_set(sr);
        __this->cur_sr = sr;
        __this->online_updata = 0;
    }
    spin_unlock(&__this->lock);

    info->L_coeff = info->R_coeff = (void *)__this->EQ_Coeff_table;
    info->L_gain = info->R_gain = __this->param.global_gain;
    info->nsection = __this->seg_num;
    info->soft_sec = __this->soft_sec;

    return 0;
}

static int eq_online_open(void)
{
    spin_lock(&__this->lock);
    __this->eq_type = EQ_TYPE_ONLINE;
    __this->seg_num = EQ_SECTION_MAX;
    __this->soft_sec = __this->soft_sec_max;
    __this->param.global_gain = 0;
    for (u8 i = 0; i < EQ_SECTION_MAX; i++) {
        memcpy(&__this->param.seg[i], &eq_seg_nor, sizeof(EQ_CFG_SEG));
    }
    __this->design_mask = (u32) - 1;
    eq_coeff_set(__this->cur_sr);
    spin_unlock(&__this->lock);

    return 0;
}

static void eq_online_close(void)
{
}

REGISTER_CONFIG_TARGET(eq_config_target) = {
    .id         = EQ_CONFIG_ID,
    .callback   = eq_online_callback,
};

#endif

static int eq_set_sr_callback(int sr, void **L_coeff, void **R_coeff)
{
    struct audio_eq_filter_info info = {0};
    int ret = -1;

#if TCFG_LIMITER_ENABLE
    if (sr != __this->cur_sr) {
        __this->limiter_updata = 1;
    }
#endif

    switch (__this->eq_type) {
    case EQ_TYPE_MODE_TAB:
        ret = eq_mode_get_filter_info(sr, &info);
        break;
#if TCFG_EQ_FILE_ENABLE
    case EQ_TYPE_FILE:
        ret = eq_file_get_filter_info(sr, &info);
        break;
#endif
#if TCFG_EQ_ONLINE_ENABLE
    case EQ_TYPE_ONLINE:
        ret = eq_online_get_filter_info(sr, &info);
        break;
#endif
    }

    if (ret) {
        return -1;
    }

    if (L_coeff) {
        *L_coeff = info.L_coeff;
    }
    if (R_coeff) {
        *R_coeff = info.R_coeff;
    }

    return 0;
}

static int eq_out_cb(void *priv, void *buf, u32 byte_len, u32 sample_rate, u8 channel)
{
#if TCFG_LIMITER_ENABLE
    EQ_ONLINE_PARAMETER_LIMITER limiter;
    int attackTime[2], releaseTime[2], threshold[2];
    spin_lock(&__this->lock);
    memcpy(&limiter, &__this->param.limiter, sizeof(EQ_ONLINE_PARAMETER_LIMITER));
    if (limiter.enable && __this->limiter_updata && __this->limiter_buf) {
        threshold[1] = threshold[0] = round(pow(10.0, (float)limiter.threshold / 20.0) * 32768);
        attackTime[1] = attackTime[0] = limiter.attackTime;
        releaseTime[1] = releaseTime[0] = limiter.releaseTime;
        limiter_init(__this->limiter_buf, attackTime, releaseTime, threshold, sample_rate, channel);
        __this->limiter_updata = 0;
    }
    spin_unlock(&__this->lock);
    if (limiter.enable && __this->limiter_buf) {
#if 0
        limiter_run_16(__this->limiter_buf, (short *)buf, (short *)buf, byte_len / 2 / channel);
#else
        limiter_run_32(__this->limiter_buf, (int *)buf, (int *)buf, byte_len / 4 / channel);
#endif
    }
#endif

    return byte_len;
}

//同步EQ参数到audio dev
int eq_cfg_sync(u8 priority)
{
    void *hdl = get_eq_hdl(); //本地或网络EQ在线调试
    if (!hdl) {
        log_error("get eq hdl fail");
        return -1;
    }

    struct eq_s_attr eq_attr = {0};

    eq_attr.nsection = __this->seg_num;
    eq_attr.gain = __this->param.global_gain;
    eq_attr.set_sr_cb = eq_set_sr_callback;
    eq_attr.soft_sec_set = 1;
    eq_attr.soft_sec_num = __this->soft_sec;
#if TCFG_LIMITER_ENABLE
    eq_attr.eq_out_cb = eq_out_cb;
    eq_attr.out_32bit = 1;
#endif

    return eq_set_s_attr(hdl, &eq_attr);
}

void set_eq_req_attr_parm(struct eq_s_attr *eq_attr)
{
    memset(eq_attr, 0, sizeof(struct eq_s_attr));
    eq_attr->nsection = __this->seg_num;
    eq_attr->gain = __this->param.global_gain;
    eq_attr->set_sr_cb = eq_set_sr_callback;
    eq_attr->soft_sec_set = 1;
    eq_attr->soft_sec_num = __this->soft_sec;
#if TCFG_LIMITER_ENABLE
    eq_attr->eq_out_cb = eq_out_cb;
    eq_attr->out_32bit = 1;
#endif
}

static int eq_cfg_init(void)
{
    memset(__this, 0, sizeof(EQ_CFG));
    __this->design_mask = (u32) - 1;

    spin_lock_init(&__this->lock);
#if TCFG_HW_SOFT_EQ_ENABLE
    __this->soft_sec_max = 3;
#endif

#if TCFG_LIMITER_ENABLE
    __this->limiter_buf = malloc(need_limiter_buf());
#endif

#if TCFG_EQ_FILE_ENABLE
    __this->eq_type = EQ_TYPE_FILE;
    __this->cur_sr = 44100;

    if (eq_file_get_cfg(__this)) //获取EQ文件失败
#endif
    {
        __this->eq_type = EQ_TYPE_MODE_TAB;
        __this->seg_num = EQ_SECTION_MAX / 2; //使用默认表时，eq段数为10段
        __this->param.global_gain = 0;
    }

#if TCFG_EQ_ONLINE_ENABLE
    eq_online_open();
#endif

    return 0;
}
late_initcall(eq_cfg_init);

static int phone_call_eq_sr_cb(int sr, void **L_coeff, void **R_coeff)
{
    if (sr == 16000) {
        *L_coeff = *R_coeff = (void **)phone_eq_filt_16000;
    } else if (sr == 8000) {
        *L_coeff = *R_coeff = (void **)phone_eq_filt_08000;
    } else {
        return -1;
    }

    return 0;
}

#if 0
int phone_call_eq_open(void)
{
    struct eq_s_attr eq_attr = {0};
    eq_attr.nsection = 3;
    eq_attr.gain    = 0.0;
    /* eq_attr.gain    = 20.0; */
    eq_attr.set_sr_cb = phone_call_eq_sr_cb;

    return eq_set_s_attr(hdl, &eq_attr);
}
#endif

#endif
