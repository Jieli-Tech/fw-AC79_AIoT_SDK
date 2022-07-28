#include "asm/jpeg_codec.h"
#include "fs/fs.h"
#include "os/os_api.h"
#include "yuv_soft_scalling.h"
#include "app_config.h"


/***********************yuv编码jpeg存放缩略图**************************
 *编码放大图片，图片存放小分辨率的缩略图信息
 *thumb_width、thumb_height为缩略图大小
 *jpeg_get_thumbnail函数获取缩略图图片
 *********************************************************************/

//应用层使用YUV编码JPG API，该函数：可缩小和放大图片
//yuvdata:YUV或Y数据地址，yuvdata_size:数据长度
//jpg_buf:JPG图片编码预先存放数据缓存区,jpg_buf_size:JPG数据缓存区长度(30K-100K:<=320*240->30K,480*320->40K,640*480->50K,1280*720*90K)
//width/height:YUV/Y数据源的分辨率宽高, out_width/out_height:输出的分辨率, q_val:图片编码质量:0-13
//返回值:<=0失败, >0:JPG实际编码的图片数据长度(用户调用该函数后保存JPG数据为jpg_buf，长度为返回值)
int yuv_enc_image(char *yuvdata, int yuvdata_size, char *jpg_buf, int jpg_buf_size, int width, int height, int out_width, int out_height, u8 q_val)
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
            input_frame.v = (u8 *)dst_img.pixels;
        } else {
            memset(out + out_width * YUV_ECN_IMG_LINE, 0x80, out_width * YUV_ECN_IMG_LINE / 4);

            //copy u v
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
exit:
    //7.关闭JPEG硬件和释放内存
    if (jpg_hdl) {
        mjpg_image_enc_close(jpg_hdl);
    }
    if (out_yuv) {
        free(out_yuv);
    }
    return err;
}


//yuv420p YUV数据,width,height:YUV数据源宽高
//zoom_width,zoom_height:照片放大的宽高
//thumb_width,thumb_height:缩略图宽高
//jpg_img照片缓存，jpg_img_size照片缓存大小，q_val:编码质量:0-13
//返回值:>0为图片数据长度,<=0编码失败
int yuv420p_enc_jpeg_thumbnail(unsigned char *yuv420p, int width, int height,
                               int zoom_width, int zoom_height,
                               unsigned char *jpg_img, int jpg_img_size, int q_val,
                               int thumb_width, int thumb_height)
{
    int err;
    int i, j;
    int ret = 0;
    int thumbjpg_addr = 0;
    unsigned char thumbjpg_img_ok = 0;
    unsigned char ffe2_table[8];
    unsigned char *thumbjpg_img = NULL;
    int thumbjpg_img_size = 0;
    int thumbjpg_enc_size = 0;
    int jpg_len = 0;

    if (thumb_width && thumb_height) {
        thumb_width = (thumb_width + 15) / 16 * 16;
        thumb_height = (thumb_height + 15) / 16 * 16;
        if (thumb_width >= width || thumb_height >= height) {
            printf("no need to set thumbjpg\n");
            goto enc_jpg;
        }
        thumbjpg_img_size = thumb_width * thumb_height * 10 / 60;
        thumbjpg_img = malloc(thumbjpg_img_size);
        if (!thumbjpg_img) {
            printf("err no %d mem in %s \n", thumbjpg_img_size, __func__);
            ret = -ENOMEM;
            goto exit;
        }

        //1.编码缩略图
        err = yuv_enc_image(yuv420p, width * height * 3 / 2,
                            thumbjpg_img, thumbjpg_img_size,
                            width, height,
                            thumb_width, thumb_height,
                            q_val);
        if (!err) {
            printf("err in thumbjpg_img\n");
            ret = -EINVAL;
            goto exit;
        }
        thumbjpg_img_ok = TRUE;
        thumbjpg_enc_size = err;
    }
enc_jpg:
    //2.编码缩放需要的JPG图
    err = yuv_enc_image(yuv420p, width * height * 3 / 2,
                        jpg_img, jpg_img_size,
                        width, height,
                        zoom_width, zoom_height,
                        q_val);
    if (!err) {
        printf("err in yuv_enc_image \n");
        ret = -EINVAL;
        goto exit;
    }
    jpg_len = err;

    //3.检查buffer够不够存储缩略图
    if (thumbjpg_img_ok && (jpg_len + 8 + thumbjpg_enc_size) > jpg_img_size) {
        printf("jpg_img_size no enough, need size = %d , jpg_img_size = %d \n\n", (jpg_len + 8 + thumbjpg_enc_size), jpg_img_size);
        ret = -EINVAL;
        goto exit;
    }

    //4.存储缩略图在缩放JPEG图上
    if (thumbjpg_img_ok) {
        memset(ffe2_table, 0, 8);
        j = jpg_len;
        i = 0;
        while (j--) {
            if (jpg_img[i] == (unsigned char)0xFF && jpg_img[i + 1] == (unsigned char)0xDB) {
                break;
            }
            i++;
        }
        if (j > 0) {
            ffe2_table[0] = 0xFF;
            ffe2_table[1] = 0xE2;
            ffe2_table[2] = 0x0;
            ffe2_table[3] = 0x6;
            thumbjpg_addr = jpg_len + 8;
            ffe2_table[4] = (unsigned char)(thumbjpg_addr & 0xFF);
            ffe2_table[5] = (unsigned char)((thumbjpg_addr >> 8) & 0xFF);
            ffe2_table[6] = (unsigned char)((thumbjpg_addr >> 16) & 0xFF);
            ffe2_table[7] = (unsigned char)((thumbjpg_addr >> 24) & 0xFF);
            int end_new = jpg_len + 8;
            int end = jpg_len;
            j = jpg_len - i + 1;
            while (j--) {
                jpg_img[end_new] = jpg_img[end];
                end_new--;
                end--;
            }
            memcpy(jpg_img + i, ffe2_table, 8);
            jpg_len += 8;
            memcpy(jpg_img + jpg_len, thumbjpg_img, thumbjpg_enc_size);
            jpg_len += thumbjpg_enc_size;
        }
    }
exit:
    //5.释放内存
    if (thumbjpg_img) {
        free(thumbjpg_img);
    }
    if (!ret) {
        ret = jpg_len;
    }
    return ret;
}
//path:jpeg文件路径
//jpg_img:存放缩略图缓冲区
//jpg_img_size:存放缩略图缓冲区长度
//返回值:>0为缩略图图片数据长度,<=0失败
int jpeg_get_thumbnail(const char *path, unsigned char *jpg_img, int jpg_img_size)
{
    int ret = 0;
    int len;
    int rm_len;
    int i, j;
    int thumbjpg_addr = 0;
    FILE *fd = NULL;
    int buf_size = 512;
    unsigned char *buf = malloc(buf_size);
    if (!buf) {
        printf("err no %d mem in %s \n", buf_size, __func__);
        ret = -ENOMEM;
        goto exit;
    }
    fd = fopen(path, "r");
    if (!fd) {
        printf("open file %s err \n\n", path);
        ret = -EINVAL;
        goto exit;
    }
    len = flen(fd);
    rm_len = len;
    while (rm_len) {
        if (fread(buf, 1, buf_size, fd) != buf_size) {
            printf("read file err !!!\n");
            ret = -EINVAL;
            goto exit;
        }
        j = buf_size;
        i = 0;
        while (j--) {
            if (buf[i] == (unsigned char)0xFF && buf[i + 1] == (unsigned char)0xE2) {
                break;
            }
            if (buf[i] == (unsigned char)0xFF && buf[i + 1] == (unsigned char)0xDB) {
                printf("no found thumbnail_jpg\n");
                ret = -EINVAL;
                goto exit;
            }
            i++;
        }
        if (j > 0) {
            thumbjpg_addr |= (unsigned char)buf[i + 4] & 0xFF;
            thumbjpg_addr |= ((unsigned char)buf[i + 5] & 0xFF) << 8;
            thumbjpg_addr |= ((unsigned char)buf[i + 6] & 0xFF) << 16;
            thumbjpg_addr |= ((unsigned char)buf[i + 7] & 0xFF) << 24;
            break;
        }
        rm_len -= buf_size;
    }
    /*printf("read thumbjpg_addr = 0x%x, len = %d \n", thumbjpg_addr, len);*/
    if (thumbjpg_addr && thumbjpg_addr < len) {
        rm_len = len - thumbjpg_addr;
        if (jpg_img_size < rm_len) {
            printf("jpg_img_size no enough , need %d ,jpg_img_size = %d \n", rm_len, jpg_img_size);
        } else {
            fseek(fd, thumbjpg_addr, SEEK_SET);
            if (fread(jpg_img, 1, rm_len, fd) != rm_len) {
                printf("read file err !!!\n");
                ret = -EINVAL;
                goto exit;
            }
            ret = rm_len;
        }
    }
exit:
    if (fd) {
        fclose(fd);
    }
    if (buf) {
        free(buf);
    }
    return ret;
}

#if 0
void yuv2jpeg_test(void)
{
    int width = 640;
    int height = 480;
    int zoom_width = 960;
    int zoom_height = 640;
    int thumb_width = 320;
    int thumb_height = 240;
    int ret;
    int file_len;
    char name[64];
    char *jpg_img = NULL;

    //这个长度则应该是:正常图片大小+缩略图大小, 图片最大值:1280x720:130K, 960x640:90K, 640X480:50K, 320*240:20K, 160x128:10K
    //如果内存不够则可以适当减少，一般图片质量：1280x720:90K, 960x640:70K, 640X480:45K, 320*240:15K, 160x128:5K
    int jpg_img_size = 90 * 1024 + 20 * 1024;//按照最大计算:90K+20K，大于1280x720具体要看看实际大小
    FILE *fd = NULL;

    /***************编码带缩略图*********************/
    //1 malloc buffer
    char *yuv = malloc(width * height * 3 / 2);
    if (!yuv) {
        return ;
    }
    jpg_img = malloc(jpg_img_size);
    if (!jpg_img) {
        goto exit;
    }
    sprintf(name, "%sYUV_TEST.YUV", CONFIG_ROOT_PATH);
    printf("name : %s\n", name);

    //2 读取YUV
    fd = fopen(name, "r");
    if (!fd) {
        printf("file open err \n");
        goto exit;
    }
    file_len = flen(fd);
    fread(yuv, 1, file_len, fd);
    fclose(fd);

    //3 YUV编码带缩略图JPEG
    ret = yuv420p_enc_jpeg_thumbnail(yuv, width, height,
                                     zoom_width, zoom_height,
                                     jpg_img, jpg_img_size, 5,
                                     thumb_width, thumb_height);
    if (ret <= 0) {
        printf("yuv420p_enc_jpeg_thumbnail err = %d \n", ret);
        goto exit;
    }

    //4 保存JPEG照片
    sprintf(name, "%stest_.jpg", CONFIG_ROOT_PATH);
    printf("name : %s\n", name);
    fd = fopen(name, "wb+");
    if (!fd) {
        printf("file open err \n");
        goto exit;
    }
    fwrite(jpg_img, 1, ret, fd);
    fclose(fd);

    /***************打开带缩略图照片*********************/
    //1 打开打开带缩略图照片
    memset(jpg_img, 0, jpg_img_size);
    ret = jpeg_get_thumbnail(name, jpg_img, jpg_img_size);
    if (ret <= 0) {
        printf("jpeg_get_thumbnail err \n\n");
        goto exit;
    }

    //2 拿到缩略图可以做保存或者图片传输，此处保存到SD卡查看
    sprintf(name, "%stest_thumbnail.jpg", CONFIG_ROOT_PATH);
    printf("name : %s\n", name);
    fd = fopen(name, "wb+");
    if (!fd) {
        printf("file open err \n");
        goto exit;
    }
    fwrite(jpg_img, 1, ret, fd);
    fclose(fd);

exit:
    if (jpg_img) {
        free(jpg_img);
    }
    if (yuv) {
        free(yuv);
    }
}
#endif
