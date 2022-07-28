#include "fm_manage.h"
#include "device/iic.h"
#include "server/audio_server.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "fm_rw.h"
#include "system/timer.h"

#if CONFIG_FM_DEV_ENABLE

#define IIC_DELAY_US 1000

#define SCANE_DOWN        (0x01)
#define SCANE_UP          (0x02)


struct fm_info {
    u8 volume;
    u8 mute;
    u8 on;
    u8 scan_flag;
    int scan_fre;
    int pid;
    FM_INTERFACE *dev;
    void *enc_server;
    void *iic;
    void *pwm;
    u16 fm_freq_cur;		//  real_freq = fm_freq_cur + 875
    u16 fm_freq_channel_cur;
    u16 fm_total_channel;
};

static struct fm_info *fm_hdl;

extern void linein_to_fdac_mute(u8 enable);
extern int get_app_music_volume(void);

void fm_IIC_write(u8 w_chip_id, u8 register_address, u8 *buf, u32 data_len)
{
    if (!fm_hdl || !fm_hdl->iic) {
        return;
    }

    void *iic = fm_hdl->iic;

    dev_ioctl(iic, IIC_IOCTL_START, 0);

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, w_chip_id)) {
        printf("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }

    if (0xff != register_address) {
        if (dev_ioctl(iic, IIC_IOCTL_TX, register_address)) {
            printf("iic write err!!! line : %d \n", __LINE__);
            goto exit;
        }
    }

    while (data_len--) {
        if (dev_ioctl(iic, IIC_IOCTL_TX, *buf++)) {
            printf("iic write err!!! line : %d \n", __LINE__);
            goto exit;
        }
    }

    dev_ioctl(iic, IIC_IOCTL_TX_STOP_BIT, 0);

exit:
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    delay_us(IIC_DELAY_US);
}

u8 fm_IIC_readn(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    if (!fm_hdl || !fm_hdl->iic) {
        return 0;
    }

    void *iic = fm_hdl->iic;
    u8 read_len = 0;

    dev_ioctl(iic, IIC_IOCTL_START, 0);

    if (0xff != register_address) {
        if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, (r_chip_id & 0x01) ? (r_chip_id - 1) : (r_chip_id))) {
            printf("iic read err!!! line : %d \n", __LINE__);
            goto exit;
        }

#if 1
        if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, register_address)) {
            printf("iic read err!!! line : %d \n", __LINE__);
            goto exit;
        }

        delay_us(IIC_DELAY_US);
#else
        if (dev_ioctl(iic, IIC_IOCTL_TX, register_address)) {
            printf("iic read err!!! line : %d \n", __LINE__);
            goto exit;
        }
#endif
    }

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, r_chip_id)) {
        printf("iic read err!!! line : %d \n", __LINE__);
        goto exit;
    }

    while (data_len-- > 1) {
        dev_ioctl(iic, IIC_IOCTL_RX_WITH_ACK, (u32)buf);
        ++buf;
        ++read_len;
    }

    dev_ioctl(iic, IIC_IOCTL_RX_WITH_STOP_BIT, (u32)buf);

exit:
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    delay_us(IIC_DELAY_US);

    return read_len;
}

static FM_INTERFACE *fm_manage_check_online(void)
{
    FM_INTERFACE *t_fm_hdl = NULL;

    list_for_each_fm(t_fm_hdl) {
        if (t_fm_hdl->read_id(NULL) &&
            (memcmp(t_fm_hdl->logo, "fm_inside", strlen(t_fm_hdl->logo)))) {
            printf("fm find dev %s \n", t_fm_hdl->logo);
            return t_fm_hdl;
        }
    }

    return NULL;
}

static int fm_manage_init(struct fm_info *fm)
{
    fm->iic = dev_open("iic0", NULL);
    if (!fm->iic) {
        goto exit;
    }

#if 0
    fm->pwm = dev_open("pwm1", NULL);
    if (!fm->pwm) {
        goto exit;
    }
#endif

    os_time_dly(5);

    fm->dev = fm_manage_check_online();
    if (!fm->dev) {
        goto exit;
    }

    if (0 != fm->dev->init(fm)) {
        goto exit;
    }

    union audio_req req = {0};

    fm->enc_server = server_open("audio_server", "enc");
    if (!fm->enc_server) {
        goto exit;
    }

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    /* req.enc.volume = CONFIG_FM_LINEIN_ADC_GAIN; */
    req.enc.volume = get_app_music_volume();
    req.enc.format = "pcm";
    req.enc.sample_source = "linein";
    req.enc.sample_rate = 44100;
    req.enc.channel_bit_map = BIT(CONFIG_FM_LINEIN_ADC_CHANNEL);
    req.enc.direct2dac = 1;
    /* req.enc.high_gain = 1; */

    return server_request(fm->enc_server, AUDIO_REQ_ENC, &req);

exit:
    if (fm->iic) {
        dev_close(fm->iic);
        fm->iic = NULL;
    }
    if (fm->pwm) {
        dev_close(fm->pwm);
        fm->pwm = NULL;
    }
    if (fm->dev) {
        fm->dev->close(fm);
        fm->dev = NULL;
    }

    return -1;
}

static void fm_manage_close(struct fm_info *fm)
{
    union audio_req req = {0};

    if (fm->enc_server) {
        req.enc.cmd = AUDIO_ENC_STOP;
        server_request(fm->enc_server, AUDIO_REQ_ENC, &req);
        server_close(fm->enc_server);
        fm->enc_server = NULL;
    }
    if (fm->dev) {
        fm->dev->close(fm);
        fm->dev = NULL;
    }
    if (fm->iic) {
        dev_close(fm->iic);
        fm->iic = NULL;
    }
    if (fm->pwm) {
        dev_close(fm->pwm);
        fm->pwm = NULL;
    }
}

static void fm_app_mute(struct fm_info *fm, u8 mute)
{
    if (!fm->on && !mute) {
        return;
    }
    if (fm->mute != mute) {
        fm->dev->mute(fm, mute);
        fm->mute = mute;
    }
}

static int fm_manage_set_fre(struct fm_info *fm, u16 fre)
{
    return fm->dev->set_fre(fm, fre);
}

static void fm_read_info_init(struct fm_info *fm)
{
    FM_VM_INFO info = {0};

    fm_vm_check();
    fm_read_info(&info);

    fm->fm_freq_cur = info.curFreq;
    printf("fm->fm_freq_cur = %d\n", fm->fm_freq_cur);
    fm->fm_freq_channel_cur	= info.curChanel;
    printf("fm->fm_freq_channel_cur = %d\n", fm->fm_freq_channel_cur);
    fm->fm_total_channel = info.total_chanel;
    printf("fm->fm_total_channel = %d\n", fm->fm_total_channel);

    if (fm->fm_freq_cur == 0 && fm->fm_freq_channel_cur && fm->fm_total_channel) {
        fm->fm_freq_cur = get_fre_via_channel(fm->fm_freq_channel_cur);
        fm_manage_set_fre(fm, REAL_FREQ(fm->fm_freq_cur));
    } else {
        fm_manage_set_fre(fm, REAL_FREQ(fm->fm_freq_cur));
    }
}

/*******************fm msg deal*********************/
static void fm_volume_pp(struct fm_info *fm)
{
    if (fm->scan_flag) {
        return;
    }
    fm_app_mute(fm, !fm->mute);
}

static void fm_dec_onoff(struct fm_info *fm, u8 on)
{
    fm->on = on;
    linein_to_fdac_mute(!on);
    fm_app_mute(fm, !on);
}

static void fm_prev_freq(struct fm_info *fm)
{
    if (fm->scan_flag) {
        return;
    }

    if (fm->fm_freq_cur <= VIRTUAL_FREQ(REAL_FREQ_MIN)) {
        fm->fm_freq_cur = VIRTUAL_FREQ(REAL_FREQ_MAX);
    } else {
        fm->fm_freq_cur -= 1;
    }

    fm_app_mute(fm, 1);
    fm_manage_set_fre(fm, REAL_FREQ(fm->fm_freq_cur));
    fm_last_freq_save(REAL_FREQ(fm->fm_freq_cur));
    fm_app_mute(fm, 0);
}

static void fm_next_freq(struct fm_info *fm)
{
    if (fm->scan_flag) {
        return;
    }

    if (fm->fm_freq_cur >= VIRTUAL_FREQ(REAL_FREQ_MAX)) {
        fm->fm_freq_cur = VIRTUAL_FREQ(REAL_FREQ_MIN);
    } else {
        fm->fm_freq_cur += 1;
    }

    fm_app_mute(fm, 1);
    fm_manage_set_fre(fm, REAL_FREQ(fm->fm_freq_cur));
    fm_last_freq_save(REAL_FREQ(fm->fm_freq_cur));
    fm_app_mute(fm, 0);
}

static void fm_volume_set(struct fm_info *fm)
{
    union audio_req req = {0};

    if (fm->enc_server && fm->on) {
        req.enc.cmd = AUDIO_ENC_SET_VOLUME;
        req.enc.volume = get_app_music_volume();
        server_request(fm->enc_server, AUDIO_REQ_ENC, &req);
        linein_to_fdac_mute(0 == get_app_music_volume());
    }
}

static void fm_prev_station(struct fm_info *fm)
{
    if (fm->scan_flag || (!fm->fm_total_channel)) {
        return;
    }

    if (fm->fm_freq_channel_cur <= 1) {
        fm->fm_freq_channel_cur = fm->fm_total_channel;
    } else {
        fm->fm_freq_channel_cur -= 1;
    }

    fm_app_mute(fm, 1);
    fm->fm_freq_cur = get_fre_via_channel(fm->fm_freq_channel_cur);
    fm_manage_set_fre(fm, REAL_FREQ(fm->fm_freq_cur));
    fm_last_ch_save(fm->fm_freq_channel_cur);
    fm_app_mute(fm, 0);
}

static void fm_next_station(struct fm_info *fm)
{
    if (fm->scan_flag || (!fm->fm_total_channel)) {
        return;
    }

    if (fm->fm_freq_channel_cur >= fm->fm_total_channel) {
        fm->fm_freq_channel_cur = 1;
    } else {
        fm->fm_freq_channel_cur += 1;
    }

    fm_app_mute(fm, 1);
    fm->fm_freq_cur = get_fre_via_channel(fm->fm_freq_channel_cur);
    fm_manage_set_fre(fm, REAL_FREQ(fm->fm_freq_cur));
    fm_last_ch_save(fm->fm_freq_channel_cur);
    fm_app_mute(fm, 0);
}

static void fm_timer_handler_down(void *priv)
{
    struct fm_info *fm = (struct fm_info *)priv;

    if (fm->scan_flag != SCANE_DOWN) {
        return;
    }

    if (fm->scan_fre > VIRTUAL_FREQ(REAL_FREQ_MAX)) {
        fm->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MIN);
        fm_app_mute(fm, 1);
        fm_manage_set_fre(fm, REAL_FREQ(fm->fm_freq_cur));
        fm->scan_flag = 0;
        fm_app_mute(fm, 0);
        return;
    }

    fm_app_mute(fm, 1);

    if (fm_manage_set_fre(fm, REAL_FREQ(fm->scan_fre))) {
        printf("FM FIND %d %d\n", REAL_FREQ(fm->scan_fre), fm->scan_fre);
        fm->fm_freq_cur = fm->scan_fre;
        fm->fm_total_channel++;
        fm->fm_freq_channel_cur = fm->fm_total_channel;//++;
        save_fm_point(REAL_FREQ(fm->scan_fre));
        sys_timeout_add(fm, fm_timer_handler_down, 1500); //播放一秒
        fm_app_mute(fm, 0);
    } else {
        sys_timeout_add(fm, fm_timer_handler_down, 20);
    }
    fm->scan_fre++;
}

static void fm_timer_handler_up(void *priv)
{
    struct fm_info *fm = (struct fm_info *)priv;

    if (fm->scan_flag != SCANE_UP) {
        return;
    }

    if (fm->scan_fre < VIRTUAL_FREQ(REAL_FREQ_MIN)) {
        fm->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MAX);
        fm_app_mute(fm, 1);
        fm_manage_set_fre(fm, REAL_FREQ(fm->fm_freq_cur));
        fm_app_mute(fm, 0);
        fm->scan_flag = 0;
        return;
    }

    fm_app_mute(fm, 1);

    if (fm_manage_set_fre(fm, REAL_FREQ(fm->scan_fre))) {
        fm->fm_freq_cur = fm->scan_fre;
        fm->fm_total_channel++;
        fm->fm_freq_channel_cur = 1;//++;
        save_fm_point(REAL_FREQ(fm->scan_fre));
        sys_timeout_add(fm, fm_timer_handler_up, 1500); //播放一秒
        fm_app_mute(fm, 0);
    } else {
        sys_timeout_add(fm, fm_timer_handler_up, 20); //
    }
    fm->scan_fre--;
}

static void fm_scan_all_up(struct fm_info *fm)
{
    if (fm->scan_flag) {
        fm->scan_flag = 0;
        os_time_dly(1);
        fm_app_mute(fm, 0);
        return;
    }

    clear_all_fm_point();
    fm->fm_freq_cur = 1;
    fm->fm_total_channel = 0;
    fm->fm_freq_channel_cur = 0;
    fm->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MAX);
    fm->scan_flag = SCANE_UP;

    fm_app_mute(fm, 1);
    sys_timeout_add(fm, fm_timer_handler_up, 20);
}

static void fm_scan_all_down(struct fm_info *fm)
{
    if (fm->scan_flag) {
        fm->scan_flag = 0;
        os_time_dly(1);
        fm_app_mute(fm, 0);
        return;
    }

    clear_all_fm_point();

    fm->fm_freq_cur = 1;//fm->scan_fre;
    fm->fm_total_channel = 0;//++;
    fm->fm_freq_channel_cur = 0;//++;
    fm->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MIN);
    fm->scan_flag = SCANE_DOWN;

    fm_app_mute(fm, 1);
    sys_timeout_add(fm, fm_timer_handler_down, 20);
}

static void fm_msg_deal(struct fm_info *fm, int msg)
{
    switch (msg) {
    case FM_DEC_ON:
        fm_dec_onoff(fm, 1);
        break;
    case FM_DEC_OFF:
        fm_dec_onoff(fm, 0);
        break;
    case FM_MUSIC_PP:
        fm_volume_pp(fm);
        break;
    case FM_PREV_FREQ:
        fm_prev_freq(fm);
        break;
    case FM_NEXT_FREQ:
        fm_next_freq(fm);
        break;
    case FM_VOLUME_UP:
    case FM_VOLUME_DOWN:
        fm_volume_set(fm);
        break;
    case FM_PREV_STATION:
        fm_prev_station(fm);
        break;
    case FM_NEXT_STATION:
        fm_next_station(fm);
        break;
    case FM_SCAN_ALL_DOWN:
        fm_scan_all_down(fm);
        break;
    case FM_SCAN_ALL_UP:
        fm_scan_all_up(fm);
        break;
    default:
        break;
    }
}

int fm_server_msg_post(int msg)
{
    return os_taskq_post("fm_task", 1, msg);
}
/****************************************************/

static void fm_task(void *priv)
{
    struct fm_info *fm = (struct fm_info *)priv;
    int err;
    int msg[32];

    if (0 != fm_manage_init(fm)) {
        return;
    }
    fm_app_mute(fm, 1);
    fm_read_info_init(fm);
    os_time_dly(1);
    fm_app_mute(fm, 0);
    fm_dec_onoff(fm, 1);

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case FM_MSG_EXIT:
            fm_manage_close(fm);
            return;
        default:
            fm_msg_deal(fm, msg[1]);
            break;
        }
    }
}

int fm_server_init(void)
{
    if (fm_hdl) {
        return 1;
    }

    fm_hdl = (struct fm_info *)zalloc(sizeof(struct fm_info));
    if (!fm_hdl) {
        return -1;
    }

    return thread_fork("fm_task", 13, 512, 64, &fm_hdl->pid, fm_task, fm_hdl);
}

void fm_sever_kill(void)
{
    if (!fm_hdl) {
        return;
    }

    do {
        if (OS_Q_FULL != os_taskq_post("fm_task", 1, FM_MSG_EXIT)) {
            break;
        }
        os_time_dly(5);
    } while (1);

    thread_kill(&fm_hdl->pid, KILL_WAIT);
    free(fm_hdl);
    fm_hdl = NULL;
}

#endif
