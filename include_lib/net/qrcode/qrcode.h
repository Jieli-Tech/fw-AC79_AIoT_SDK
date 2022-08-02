#ifndef __QRCODE_H__
#define __QRCODE_H__


/*#ifdef __cplusplus*/
/*extern "C" {*/
/*#endif*/

#define QRCODE_MODE_FAST		0
#define QRCODE_MODE_NORMAL		1
#define QRCODE_MODE_ACCURATE	2


#define SYM_QRCODE	0
#define SYM_BARCODE	1

#define QR_FRAME_W 320
#define QR_FRAME_H 240

/////////////////////////////////////////////////////////////////////
// function:qrcode_init
// parameters:
//		width:  the width of input image
//		height: the height of input image
//		stride: the stride of input image
//      qrmode: qrcode detect mode, QRCODE_MODE_FAST, QRCODE_MODE_NORMAL, QRCODE_MODE_ACCURATE
//		md_thre:  运动侦测阈值,默认60
//		md_active: 运动侦测启动(1:启动，0：关闭)
//      TH: 过滤阈值， 建议设置为150，最好不要大于200
//      mirro_mode:镜像模式   0：不镜像   1：垂直镜像   2：水平镜像
//////////////////////////////////////////////////////////////////////

void *qrcode_init(int width, int height, int stride, int qrmode, int md_thre, int md_active, int mirro_mode);
int qrcode_deinit(void *decoder);

//md_detected: 是否检测到运动物体,1：检测到, 0:没有检测到
int qrcode_detectAndDecode(void *decoder, unsigned char *pixels, int *md_detected);


//*enc_type = 4:进行外部解码
int qrcode_get_result(void *decoder, char **result_buf, int *result_buf_size, int *enc_type);
int qrcode_get_symbol_type(void *decoder);

int qrcode_get_dimension(void *decoder);
int qrcode_get_qr_detect_success(void *decoder);
int qrcode_get_qr_decode_success(void *decoder);
int qrcode_get_bar_decode_success(void *decoder);

void qr_code_get_one_frame_YUV_420(u8 *buf, u16 buf_w, u16 buf_h);
void qr_get_ssid_pwd(u32 ssid, u32 pwd);

/*#ifdef __cplusplus*/
/*}*/
/*#endif*/

#endif


