#include "app_config.h"


sdfile_vfs_ops
nor_sdfile_vfs_ops
clock_timer1
clock_port

#if TCFG_UDISK_ENABLE || TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
clock_sdx
fat_vfs_ops
#endif

#if CONFIG_RAMFS_ENABLE
ramfs_vfs_ops
#endif

#if CONFIG_DEVFS_ENABLE
devfs_vfs_ops
#endif

#ifdef CONFIG_VIDEO_ENABLE
video_server_info
vpkg_server_info
vpkg_fsys_ops
avi_pkg_ops
camera_subdev
djpeg_subdev
mjpg_spec_stream_subdev
uvc_subdev

#ifdef CONFIG_NET_ENABLE
net_jpeg_pkg_ops
strm_pkg_ops
net_sys_ops
#endif

#ifdef CONFIG_VIDEO_DEC_ENABLE
video_dec_server_info
vunpkg_server_info
#endif

#ifdef CONFIG_CTP_ENABLE
ctp_server_info
#endif
#endif

#ifdef CONFIG_LED_UI_ENABLE
led_ui_server_info
#endif

#ifdef CONFIG_AUDIO_ENABLE
audio_server_info
#endif

#ifdef CONFIG_LC3_DEC_ENABLE
lc3_decoder
#endif
#ifdef CONFIG_LC3_ENC_ENABLE
subdev_lc3_enc
#endif

#ifdef CONFIG_PCM_DEC_ENABLE
pcm_decoder_ops
#endif
#ifdef CONFIG_PCM_ENC_ENABLE
subdev_pcm_enc
pcm_package_ops
#endif

#ifdef CONFIG_ADPCM_DEC_ENABLE
adpcm_decoder_ops
#endif
#ifdef CONFIG_ADPCM_ENC_ENABLE
subdev_adpcm_enc
adpcm_package_ops
#endif

#ifdef CONFIG_WAV_DEC_ENABLE
wav_decoder
#endif
#ifdef CONFIG_WAV_ENC_ENABLE
wav_package_ops
#endif

#ifdef CONFIG_MSBC_DEC_ENABLE
msbc_decoder
#endif
#ifdef CONFIG_MSBC_ENC_ENABLE
subdev_msbc_enc
msbc_package_ops
#endif
#ifdef CONFIG_SBC_DEC_ENABLE
sbc_decoder
#endif
#ifdef CONFIG_SBC_ENC_ENABLE
subdev_sbc_enc
sbc_package_ops
#endif
#ifdef CONFIG_CVSD_DEC_ENABLE
cvsd_decoder
#endif
#ifdef CONFIG_CVSD_ENC_ENABLE
subdev_cvsd_enc
cvsd_package_ops
#endif

#ifdef CONFIG_MP3_DEC_ENABLE
mp3_decoder
#endif
#ifdef CONFIG_MP3_ENC_ENABLE
subdev_mp3_enc
mp3_package_ops
#endif
#ifdef CONFIG_MP2_ENC_ENABLE
subdev_mp2_enc
#endif

#ifdef CONFIG_WMA_DEC_ENABLE
wma_decoder
#endif

#ifdef CONFIG_AAC_ENC_ENABLE
subdev_aac_enc
aac_package_ops
#endif
#ifdef CONFIG_AAC_DEC_ENABLE
aac_decoder
#endif

#ifdef CONFIG_M4A_DEC_ENABLE
m4a_decoder
#endif

#ifdef CONFIG_SPEEX_DEC_ENABLE
speex_decoder
#endif
#ifdef CONFIG_SPEEX_ENC_ENABLE
subdev_speex_enc
spx_package_ops
#endif

#ifdef CONFIG_AMR_DEC_ENABLE
amr_decoder
#endif
#ifdef CONFIG_AMR_ENC_ENABLE
subdev_amr_enc
amr_package_ops
#endif

#ifdef CONFIG_APE_DEC_ENABLE
ape_decoder
#endif

#ifdef CONFIG_FLAC_DEC_ENABLE
flac_decoder
#endif

#ifdef CONFIG_DTS_DEC_ENABLE
dts_decoder
#endif

#ifdef CONFIG_OPUS_DEC_ENABLE
opus_decoder
#endif
#ifdef CONFIG_OPUS_ENC_ENABLE
subdev_ogg_enc
ogg_package_ops

subdev_opus_enc
opus_package_ops
#endif

#ifdef CONFIG_VIRTUAL_DEV_ENC_ENABLE
vir_dev_sub
#endif

#ifdef CONFIG_VAD_ENC_ENABLE
subdev_vad_enc
#endif

#ifdef CONFIG_DNS_ENC_ENABLE
subdev_dns_enc
#endif

#ifdef CONFIG_AEC_ENC_ENABLE
subdev_aec_enc
#endif

#ifdef CONFIG_TURING_SDK_ENABLE
turing_sdk_api
wechat_sdk_api
#endif

#ifdef CONFIG_DEEPBRAIN_SDK_ENABLE
deepbrain_sdk_api
#endif

#ifdef CONFIG_ECHO_CLOUD_SDK_ENABLE
echo_cloud_sdk_api
#endif

#ifdef CONFIG_DLNA_SDK_ENABLE
dlna_sdk_api
#endif

#ifdef CONFIG_WT_SDK_ENABLE
wt_sdk_api
#endif

#ifdef CONFIG_DUER_SDK_ENABLE
duer_sdk_api
#endif

#ifdef CONFIG_DUI_SDK_ENABLE
dui_sdk_api
#endif

#ifdef CONFIG_ALI_SDK_ENABLE
ag_sdk_api
#endif

#ifdef CONFIG_TVS_SDK_ENABLE
tc_tvs_api
#endif

#ifdef CONFIG_TELECOM_SDK_ENABLE
#ifdef CONFIG_CTEI_DEVICE_ENABLE
psmarthome_ctei_device_api
#endif
#ifdef CONFIG_MC_DEVICE_ENABLE
CT_MC_api
#endif
#endif

#ifdef CONFIG_JL_CLOUD_SDK_ENABLE
jl_cloud_sdk_api
#endif

