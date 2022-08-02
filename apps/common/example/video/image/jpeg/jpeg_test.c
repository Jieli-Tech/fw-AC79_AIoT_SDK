#include "asm/jpeg_codec.h"
#include "asm/hwi.h"
#include "system/includes.h"
#include "fs/fs.h"
#include "os/os_api.h"
#include "video_ioctl.h"
#include "video.h"
#include "yuv_to_rgb.h"
#include "yuv_soft_scalling.h"
#include "app_config.h"


#define JPEG_SAVE_FILE_YUV		1//测试解码保存格式:0 RGB656, 1 YUV
#define JPEG_ENC_ONLY_Y         0//编码JPEG只编灰色的Y
#define RGB565_BE				0//rgb565大小端:0小端,1端

int yuv_enc_image(char *yuvdata, int yuvdata_size, char *jpg_buf, int jpg_buf_size, int width, int height, int q_val);
int yuv_enc_large_image(char *yuvdata, int yuvdata_size, char *jpg_buf, int jpg_buf_size, int width, int height, int out_width, int out_height, u8 q_val);
static void path_change_suf_name(char *name, const char *suf, const char *path)//在原始的名称下把后缀修改成suf的值
{
    int len = strlen(path);
    char *find;
    int i  = len - 1;
    while (i >= 0) { //找出第一个'.'
        if (path[i] == '.') {
            break;
        }
        i--;
    }
    find = (char *)&path[i];
    if (i > 0) {
        memcpy(name, path, find - path);
        name[find - path] = 0;
        strcat(name, suf);
    }
}

//解码测试函数
int jpeg_decoder_test(const char *path)//path:存在SD文件的jpeg图片
{
    FILE *fd = NULL;
    u32 lenth = 0;
    u8 *cy = NULL, *cb = NULL, *cr = NULL, *yuv = NULL, *rgb16 = NULL;
    u8 *buf = NULL;
    u32 pix = 0;
    u16 width, height;
    u8 ytype = 0;
    u32 len;
    char name[64];
    int err = 0;

    printf("open file : %s \n", path);

    //1.打开jpeg文件并读取jpeg数据到内存缓存区
    fd = fopen(path, "rb");
    if (fd == NULL) {
        printf("no file name : %s\n", path);
        goto exit;
    }
    lenth = flen(fd);//读取jpeg图片数据长度
    buf = malloc(lenth);
    if (!buf) {
        printf("buf malloc err ...\n");
        goto exit;
    }
    if (fread(buf, 1, lenth, fd) != lenth) {//读取jpeg图片数据
        printf("read file lenth err ...\n");
        goto exit;
    }
    fclose(fd);
    fd = NULL;

    //2.解析jpeg信息获取源JPEG YUV数据类型和分辨率
    struct jpeg_image_info info = {0};
    info.input.data.buf = buf;
    info.input.data.len = lenth;
    if (jpeg_decode_image_info(&info)) {
        printf("jpeg_decode_image_info err %s\n", path);
        goto exit;
    }
    width = info.width;
    height = info.height;
    pix = width * height;
    switch (info.sample_fmt) {
    case JPG_SAMP_FMT_YUV444:
        ytype = 1;
        printf("jpeg yuv444, pix : %dx%d \n", width, height);
        break;//444
    case JPG_SAMP_FMT_YUV420:
        ytype = 4;
        printf("jpeg yuv420, pix : %dx%d \n", width, height);
        break;//420
    default:
        ytype = 2;
        printf("jpeg yuv422, pix : %dx%d \n", width, height);
        break;//422
    }

    //3.申请YUV内存缓存
    len = pix + pix / ytype * 2;
    if (!yuv) {
        yuv = malloc(len);
        if (!yuv) {
            printf("yuv malloc err len : %d , width : %d , height : %d \n", width, height, len);
            goto exit;
        }
    }
#if !JPEG_SAVE_FILE_YUV
    if (!rgb16) { //使用YUV转RGB
        rgb16 = malloc(pix * 2);//rgb565总需要申请内存大小为:分辨率*2，rgb24为:分辨率*3
    }
#endif

    //4.配置解码相关参数
    cy = yuv;
    cb = cy + pix;
    cr = cb + pix / ytype;
    struct jpeg_decode_req req = {0};
    req.input_type = JPEG_INPUT_TYPE_DATA;
    req.input.data.buf = buf;
    req.input.data.len = lenth;
    req.buf_y = cy;//配置Y地址
    req.buf_u = cb;//配置U地址
    req.buf_v = cr;//配置V地址
    req.buf_width = width;
    req.buf_height = height;
    req.out_width = width;
    req.out_height = height;
    req.output_type = JPEG_DECODE_TYPE_DEFAULT;
    req.bits_mode = BITS_MODE_UNCACHE;

    //5.启动jpeg图片解码
    err = jpeg_decode_one_image(&req);
    if (err) {//非0，解码失败
        printf("jpeg_decode_one_image err \n");
        goto exit;
    }

    switch (ytype) {
    case 1:
#if JPEG_SAVE_FILE_YUV
        //YUV444p统一转YUV420
        YUV444pToYUV420p(yuv, yuv, width, height);
#else
        //YUV统一转rgb565
        if (rgb16) {
            yuv444p_quto_rgb565(yuv, rgb16, width, height, RGB565_BE);
        }
#endif
        break;//444
    case 4:
#if JPEG_SAVE_FILE_YUV
        //YUV420p统一转YUV420，次数数据源已经是YUV420，不用再转换
#else
        if (rgb16) {
            yuv420p_quto_rgb565(yuv, rgb16, width, height, RGB565_BE);
        }
#endif
        break;//420
    default:
#if JPEG_SAVE_FILE_YUV
        //YUV422p统一转YUV420
        YUV422pToYUV420p(yuv, yuv, width, height);
#else
        if (rgb16) {
            yuv422p_quto_rgb565(yuv, rgb16, width, height, RGB565_BE);
        }
#endif
        break;//422
    }

    //6.保存RGB/YUV文件
#if JPEG_SAVE_FILE_YUV
    path_change_suf_name(name, ".yuv", path);//源名字改后缀.yuv
    fd = fopen(name, "wb+");//可以自行指定保存的名字
    if (!fd) {
        printf("open file err : %s \n", name);
        goto exit;
    }
    fwrite(yuv, 1, pix * 3 / 2, fd);//YUV420p大小为:分辨率*1.5倍
#else
    path_change_suf_name(name, ".rgb", path);//源名字改后缀.rgb
    fd = fopen(name, "wb+");//可以自行指定保存的名字
    if (!fd) {
        printf("open file err : %s \n", name);
        goto exit;
    }
    fwrite(rgb16, 1, pix * 2, fd);//rgb565大小为:分辨率*2倍
#endif
    printf("file write ok\n");

    //7.关闭文件和释放内存
exit:
    if (yuv) {
        free(yuv);
        yuv = NULL;
    }
    if (rgb16) {
        free(rgb16);
        rgb16 = NULL;
    }
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    return  0;
}

//编码测试函数
int jpeg_encode_test(const char *path, int width, int height)//path:存在SD文件的YUV420p数据(本次测试使用的YUV为解码jpeg_decoder_test函数保存的YUV)
{
    int err = 0;
    char name[64];
    char ytype = 4;//该根据YUV的path的YUV数据格式配置(默认yuv420p): 1 YUV444p, 2 YUV422p, 4 YUV420p
    int pix = width * height;
    int yuv_size;
    int jpg_size = 100 * 1024;//指定分辨率编码的数据长度缓冲区(一般50K-100K，不够再加大)
    FILE *fd = NULL;
    char *yuvbuf = NULL;
    char *jpg_img = NULL;

    //1.打开YUV文件
    fd = fopen(path, "rb");
    if (fd == NULL) {
        printf("no file name : %s\n", path);
        goto exit;
    }
    yuv_size = flen(fd);//读取jYUV数据长度
    if (yuv_size == pix * 3 / 2) { //此处根据文件大小判断yuv类型，用户使用时候直接固定就行
        ytype = 4;//YUV420P
    } else if (yuv_size == pix * 2) {
        ytype = 2;//YUV422P
    } else if (yuv_size == pix * 3) {
        ytype = 1;//YUV444P
    } else if (yuv_size == pix) {//打开文件只有Y时候，由于硬件需要UV，需要申请buff，此时建议用yuv_enc_image()API函数来完成，本函数只是测试例子
        printf("please use function to do : yuv_enc_image() \n");
        goto exit;
    }

    //2.申请YUV内存块
    yuvbuf = malloc(yuv_size);//申请YUV内存块
    if (!yuvbuf) {
        printf("yuvbuf malloc err ...\n");
        goto exit;
    }

    //3.读取YUV到YUV内存块
    if (fread(yuvbuf, 1, yuv_size, fd) != yuv_size) {//读取YUV数据
        printf("read file yuv_size err ...\n");
        goto exit;
    }
    fclose(fd);
    fd = NULL;

    //4.申请编码的JPEG的内存缓冲区，一般50K-100K即可
    /* VGA图片大小说明：低等质量(小于20K)，中等质量(20K-40K)，高质量(大于40K，极限70K)
       720P图片大小说明：低等质量(小于50K)，中等质量(50k-100K)，高质量(大于100K，极限150K)
    */
    jpg_img = malloc(jpg_size);
    if (!jpg_img) {
        printf("jpg img malloc err!!!\n");
        goto exit;
    }
    memset(jpg_img, 0, jpg_size);

    //5.配置编码参数，此步骤可直接使用yuv_enc_image函数来完成
#if 1
    struct jpeg_encode_req req = {0};
    req.q = 5;//0-13 编码质量
    req.format = JPG_SAMP_FMT_YUV420;
    req.data.buf = (u8 *)jpg_img;
    req.data.len = jpg_size;
    req.width = width;
    req.height = height;
    req.y = (u8 *)yuvbuf;
    req.u = req.y + pix;
    req.v = req.u + pix / ytype;

#if JPEG_ENC_ONLY_Y
    memset(req.u, 0x80, pix / ytype);//只编码灰色,把UV全设置0x80
    req.v = req.u;
#endif

    //6.启动编码器
    err = jpeg_encode_one_image(&req);
    if (err) {//非0失败
        printf("jpeg_encode_one_image err \n\n");
        goto exit;
    }
    jpg_size = req.data.len;

#else /*yuv_enc_image使用例子*/
    err = yuv_enc_image((char *)yuvbuf, yuv_size, (char *)jpg_img, jpg_size, width, height, 5);
    if (!err) {
        printf("jpeg_encode_one_image err \n\n");
        goto exit;
    }
    jpg_size = err;
#endif
    printf("jpeg_encode_one_image ok size = %dKb\n", jpg_size / 1024);

    //7.保存jpeg图片
    path_change_suf_name(name, "_***.jpg", path);
    fd = fopen(name, "w+");//可以自行指定保存的名字
    if (!fd) {
        printf("open file err : %s\n", name);
        goto exit;
    }
    fwrite(jpg_img, 1, jpg_size, fd);//保存jpeg图片
    printf("file write ok\n");

    //8.关闭文件和释放内存
exit:
    if (yuvbuf) {
        free(yuvbuf);
        yuvbuf = NULL;
    }
    if (jpg_img) {
        free(jpg_img);
        jpg_img = NULL;
    }
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    return err;
}


//编码测试函数
int jpeg_encode_large_test(const char *path, int width, int height, int out_width, int out_height)//path:存在SD文件的YUV420p数据(本次测试使用的YUV为解码jpeg_decoder_test函数保存的YUV)
{
    int err = 0;
    char name[64];
    char ytype = 4;//该根据YUV的path的YUV数据格式配置(默认yuv420p): 1 YUV444p, 2 YUV422p, 4 YUV420p
    int pix = width * height;
    int yuv_size;
    int jpg_size = 800 * 1024;//指定分辨率编码的数据长度缓冲区(一般50K-100K，不够再加大，大分辨率需要加大)
    FILE *fd = NULL;
    char *yuvbuf = NULL;
    char *jpg_img = NULL;

    //1.打开YUV文件
    fd = fopen(path, "rb");
    if (fd == NULL) {
        printf("no file name : %s\n", path);
        goto exit;
    }
    yuv_size = flen(fd);//读取jYUV数据长度
    if (yuv_size == pix * 3 / 2) { //此处根据文件大小判断yuv类型，用户使用时候直接固定就行
        ytype = 4;//YUV420P
    } else if (yuv_size == pix * 2) {
        ytype = 2;//YUV422P
    } else if (yuv_size == pix * 3) {
        ytype = 1;//YUV444P
    } else if (yuv_size == pix) {//打开文件只有Y时候，由于硬件需要UV，需要申请buff，此时建议用yuv_enc_image()API函数来完成，本函数只是测试例子
        printf("please use function to do : yuv_enc_image() \n");
        goto exit;
    }

    //2.申请YUV内存块
    yuvbuf = malloc(yuv_size);//申请YUV内存块
    if (!yuvbuf) {
        printf("yuvbuf malloc err ...\n");
        goto exit;
    }

    //3.读取YUV到YUV内存块
    if (fread(yuvbuf, 1, yuv_size, fd) != yuv_size) {//读取YUV数据
        printf("read file yuv_size err ...\n");
        goto exit;
    }
    fclose(fd);
    fd = NULL;

    //4.申请编码的JPEG的内存缓冲区，一般50K-100K即可
    /* VGA图片大小说明：低等质量(小于20K)，中等质量(20K-40K)，高质量(大于40K，极限70K)
       720P图片大小说明：低等质量(小于50K)，中等质量(50k-100K)，高质量(大于100K，极限150K)
    */
    jpg_img = malloc(jpg_size);
    if (!jpg_img) {
        printf("jpg img malloc err!!!\n");
        goto exit;
    }
    memset(jpg_img, 0, jpg_size);

    //6.启动编码器
    /*yuv_enc_large_image使用例子*/
    err = yuv_enc_large_image(yuvbuf, yuv_size, (char *)jpg_img, jpg_size, width, height, out_width, out_height, 10);
    if (err <= 0) {
        printf("jpeg_encode_one_image err \n\n");
        goto exit;
    }
    jpg_size = err;
    printf("jpeg_encode_one_image ok size = %dKb\n", jpg_size / 1024);

    //7.保存jpeg图片
    path_change_suf_name(name, "_***.jpg", path);
    fd = fopen(name, "w+");//可以自行指定保存的名字
    if (!fd) {
        printf("open file err : %s\n", name);
        goto exit;
    }
    fwrite(jpg_img, 1, jpg_size, fd);//保存jpeg图片
    printf("file write ok\n");

    //8.关闭文件和释放内存
exit:
    if (yuvbuf) {
        free(yuvbuf);
        yuvbuf = NULL;
    }
    if (jpg_img) {
        free(jpg_img);
        jpg_img = NULL;
    }
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    return err;
}


//应用层使用YUV编码JPG API
//yuvdata:YUV或Y数据地址，yuvdata_size:数据长度
//jpg_buf:JPG图片编码预先存放数据缓存区,jpg_buf_size:JPG数据缓存区长度(30K-100K:<=320*240->30K,480*320->40K,640*480->50K,1280*720*90K)
//width/height:YUV/Y数据源的分辨率宽高, q_val:图片编码质量:0-13
//返回值:0失败,非0:JPG实际编码的图片数据长度(用户调用该函数后保存JPG数据为jpg_buf，长度为返回值)
int yuv_enc_image(char *yuvdata, int yuvdata_size, char *jpg_buf, int jpg_buf_size, int width, int height, int q_val)
{
    struct jpeg_encode_req req = {0};
    int err = 0;
    char only_y = 0;
    char yuv_alloc = 0;
    char *yuv = yuvdata;

    if (yuvdata_size == width * height * 3) {
        req.format = JPG_SAMP_FMT_YUV444;
    } else if (yuvdata_size == width * height * 2) {
        req.format = JPG_SAMP_FMT_YUV422;
    } else {
        req.format = JPG_SAMP_FMT_YUV420;
        only_y = (yuvdata_size == width * height) ? 1 : 0;
        if (only_y) {
            yuv = malloc(width * height + width * height / 4);
            yuv_alloc = 1;
        }
    }
    if (!yuv) {
        printf("err yuv buf not enough\n");
        return 0;
    }
    req.q = q_val;
    req.data.buf = (u8 *)jpg_buf;
    req.data.len = jpg_buf_size;
    req.width =  width;
    req.height = height;
    req.y = (u8 *)yuv;
    req.u = req.y + req.width * req.height;
    req.v = req.u + req.width * req.height / 4;

#if JPEG_ENC_ONLY_Y
    only_y = 1;
#endif
    if (only_y) {
        if (yuv != yuvdata) {
            memcpy(yuv, yuvdata, width * height);
        }
        memset(yuv + width * height, 0x80, width * height / 4);
        req.v = req.u;
    }
    if (jpeg_encode_one_image(&req) == 0) {
        printf("jpeg_encode_one_image ok\n");
        err = req.data.len;
    } else {
        printf("jpeg_encode_one_image err\n");
    }
    if (yuv_alloc) {
        free(yuv);
    }
    return err;
}
//应用层使用YUV编码JPG API
//yuvdata:YUV或Y数据地址，yuvdata_size:数据长度
//jpg_buf:JPG图片编码预先存放数据缓存区,jpg_buf_size:JPG数据缓存区长度(30K-100K:<=320*240->30K,480*320->40K,640*480->50K,1280*720*90K)
//width/height:YUV/Y数据源的分辨率宽高, out_width/out_height:输出的分辨率, q_val:图片编码质量:0-13
//返回值:<=0失败, >0:JPG实际编码的图片数据长度(用户调用该函数后保存JPG数据为jpg_buf，长度为返回值)
int yuv_enc_large_image(char *yuvdata, int yuvdata_size, char *jpg_buf, int jpg_buf_size, int width, int height, int out_width, int out_height, u8 q_val)
{
#define YUV_ECN_IMG_LINE	    16 //一次编码行数
#define YUV_SCAL_SAVE_SD	    0
    int err = 0;
    int i;
    int out_yuv_size = 0;
    char only_y = 0;
    char *src, *out;
    char *out_yuv = NULL;
    char *yuv = yuvdata;
    void *jpg_hdl = NULL;
    int in_yuv_enc_line = 0;
    int in_yuv_enc_line_reserv = 0;
    int all_in_line = 0;
    int all_out_line = 0;
    float tmp;
    float reserv = 0;
    char *out_y420 = NULL;
    char *out_u420 = NULL;
    char *out_v420 = NULL;
    int out_y420_offset = 0;
    int out_u420_offset = 0;
    int out_v420_offset = 0;
    void jlve_scale_bilinear(image_info * src, image_info * dst);

    if ((out_width % 16) != 0 || (out_height % 16) != 0) { //分辨率不是16对齐则警告
        printf("waning: out_width or out_height no align 16 bytes, %d x %d\n", out_width, out_height);
    }
    //1.分辨率对齐
    out_width = (out_width + 15) / 16 * 16;
    out_height = (out_height + 15) / 16 * 16;

    tmp = (float)YUV_ECN_IMG_LINE * height / out_height;
    in_yuv_enc_line = tmp;

    //2.YUV统一转YUV420
    if (yuvdata_size == width * height * 3) {
        YUV444pToYUV420p(yuv, yuv, width, height);//统一转YUV420
    } else if (yuvdata_size == width * height * 2) {
        YUV422pToYUV420p(yuv, yuv, width, height);//统一转YUV420
    } else {
        only_y = (yuvdata_size == width * height) ? 1 : 0;
    }
#if JPEG_ENC_ONLY_Y
    only_y = 1;
#endif

    if (only_y) {
        out_yuv_size = out_width * YUV_ECN_IMG_LINE + out_width * YUV_ECN_IMG_LINE / 4;
    } else {
        out_yuv_size = out_width * YUV_ECN_IMG_LINE * 3 / 2;
    }

    //3.申请YUV内存
    out_yuv = malloc(out_yuv_size);
    if (!yuv || !out_yuv) {
        printf("err yuv buf not enough\n");
        return 0;
    }

#if YUV_SCAL_SAVE_SD
    //保存到sd卡的yuv buffer
    out_y420 = malloc(out_width * out_height * 3 / 2);
    if (!out_y420) {
        printf("err yuv buf not enough\n");
        return 0;
    }
    memset(out_y420, 0, out_width * out_height * 3 / 2);
    out_u420 = out_y420 + out_width * out_height;
    out_v420 = out_u420 + out_width * out_height / 4;
#endif

    //4.打开JPEG硬件编码器
    jpg_hdl = mjpg_image_enc_open(NULL, JPEG_ENC_MANU_IMAGE);
    if (!jpg_hdl) {
        printf("mjpg_image_enc_open err\n");
        goto exit;
    }

    //5.完成1行MCU(16行像素点)缩放送进编码器编码
    image_info src_img;
    image_info dst_img;
    struct YUV_frame_data input_frame = {0};
    input_frame.width = out_width;
    input_frame.height = out_height;
    input_frame.data_height = YUV_ECN_IMG_LINE;
    src = yuv;
    out = out_yuv;
    for (i = 0, reserv = 0, input_frame.line_num = 0; i <= (out_height - YUV_ECN_IMG_LINE); i += YUV_ECN_IMG_LINE, input_frame.line_num += YUV_ECN_IMG_LINE) {
        reserv += (float)(tmp - in_yuv_enc_line);
        if (reserv >= 2.0) { //补偿，2对齐
            in_yuv_enc_line_reserv = in_yuv_enc_line + (int)reserv;
            reserv -= (int)reserv;
        } else {
            in_yuv_enc_line_reserv = in_yuv_enc_line;
        }

        //y
        src_img.w = width;
        src_img.h = in_yuv_enc_line_reserv;
        src_img.pixels = (unsigned char *)(src + all_in_line * width);
        dst_img.w = out_width;
        dst_img.h = YUV_ECN_IMG_LINE;
        dst_img.pixels = (unsigned char *)out;
        jlve_scale_bilinear(&src_img, &dst_img);

        //copy y
#if YUV_SCAL_SAVE_SD
        memcpy(out_y420 + out_y420_offset, out, out_width * YUV_ECN_IMG_LINE);
        out_y420_offset += out_width * YUV_ECN_IMG_LINE;
#endif
        input_frame.y = (u8 *)dst_img.pixels;

        if (!only_y) {
            //u
            src_img.w = width / 2;
            src_img.h = in_yuv_enc_line_reserv / 2;
            src_img.pixels = (unsigned char *)(src + width * height + all_in_line / 2 * width / 2);
            dst_img.w = out_width / 2;
            dst_img.h = YUV_ECN_IMG_LINE / 2;
            dst_img.pixels = (unsigned char *)(out + out_width * YUV_ECN_IMG_LINE);
            jlve_scale_bilinear(&src_img, &dst_img);
            //copy u
#if YUV_SCAL_SAVE_SD
            memcpy(out_u420 + out_u420_offset, out + out_width * YUV_ECN_IMG_LINE, out_width * YUV_ECN_IMG_LINE / 4);
            out_u420_offset += out_width * YUV_ECN_IMG_LINE / 4;
#endif
            input_frame.u = (u8 *)dst_img.pixels;

            //v
            src_img.w = width / 2;
            src_img.h = in_yuv_enc_line_reserv / 2;
            src_img.pixels = (unsigned char *)(src + width * height + width / 2 * height / 2 + all_in_line / 2 * width / 2);
            dst_img.w = out_width / 2;
            dst_img.h = YUV_ECN_IMG_LINE / 2;
            dst_img.pixels = (unsigned char *)(out + out_width * YUV_ECN_IMG_LINE + out_width / 2 * YUV_ECN_IMG_LINE / 2);
            jlve_scale_bilinear(&src_img, &dst_img);

            //copy v
#if YUV_SCAL_SAVE_SD
            memcpy(out_v420 + out_v420_offset, out + out_width * YUV_ECN_IMG_LINE + out_width * YUV_ECN_IMG_LINE / 4, out_width * YUV_ECN_IMG_LINE / 4);
            out_v420_offset += out_width * YUV_ECN_IMG_LINE / 4;
#endif
            input_frame.v = (u8 *)dst_img.pixels;
        } else {
            memset(out + out_width * YUV_ECN_IMG_LINE, 0x80, out_width * YUV_ECN_IMG_LINE / 4);

            //copy u v
#if YUV_SCAL_SAVE_SD
            memcpy(out_u420 + out_u420_offset, out + out_width * YUV_ECN_IMG_LINE, out_width * YUV_ECN_IMG_LINE / 4);
            memcpy(out_v420 + out_v420_offset, out + out_width * YUV_ECN_IMG_LINE, out_width * YUV_ECN_IMG_LINE / 4);
            out_u420_offset += out_width * YUV_ECN_IMG_LINE / 4;
            out_v420_offset += out_width * YUV_ECN_IMG_LINE / 4;
#endif
            input_frame.u = (u8 *)(out + out_width * YUV_ECN_IMG_LINE);
            input_frame.v = input_frame.u;
        }
        all_in_line += in_yuv_enc_line_reserv;
        all_out_line += YUV_ECN_IMG_LINE;

        //6.每1行MCU(16行像素点)编码一次
        err = mjpg_image_enc_start(jpg_hdl, &input_frame, jpg_buf, jpg_buf_size, q_val);
        if (err < 0 || (err <= 0 && all_out_line >= out_height)) {
            printf("jpg_image_enc err = %d, all_out_line = %d \n", err, all_out_line);
            goto exit;
        }
    }
    //保存到sd卡
#if YUV_SCAL_SAVE_SD
    void sdfile_save_test(char *buf, int len, char one_file, char close);
    sdfile_save_test(out_y420, out_width * out_height * 3 / 2, 1, 0);
    sdfile_save_test(out_y420, 0, 1, 1);
#endif

exit:
    //7.关闭JPEG硬件和释放内存
    if (jpg_hdl) {
        mjpg_image_enc_close(jpg_hdl);
    }
    if (out_yuv) {
        free(out_yuv);
    }
    if (out_y420) {
        free(out_y420);
    }
    return err;
}