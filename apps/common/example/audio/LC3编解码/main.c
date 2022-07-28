#include "server/audio_server.h"
#include "server/server_core.h"
#include "system/includes.h"
#include "generic/circular_buf.h"
#include "os/os_api.h"
#include "app_config.h"
#include "fs/fs.h"


static int test_sr = 44100;
static int test_br = 64000;
static int test_ch = 2;

static void *cahe_buf = NULL;
static cbuffer_t save_cbuf;
static void *enc_server;


static OS_SEM r_sem;


#define TEST_DEC_LC3	// FOR TEST:测试解码LC3并且播放

//编码器输出LC3数据,这里拿到LC3数据data然后空中传输
static int recorder_vfs_fwrite(void *file, void *data, u32 len)
{
#if 1
	//e.g. 拿到数据填入cbuf
	cbuffer_t *cbuf = (cbuffer_t *)file;
	if(0 == cbuf_write(cbuf, data, len)){
		printf("-------%s-------%d\n\r",__func__,__LINE__);
		//如果写不进去了,则清空一下cbuf(这一步目的为了不让接收数据有延迟)
		cbuf_clear(cbuf);	
	}
	os_sem_set(&r_sem,0);
	os_sem_post(&r_sem);
#else
	//空中传输data
#endif

	return len;
}


#ifdef TEST_DEC_LC3
static void dec_test();
static void *dec_server;
static u8 run_flag;


//解码器读取数据,这里将数据传入LC3解码器,数据需要给足len的长度
static int recorder_vfs_fread(void *file, void *data, u32 len)
{

#if 1
	//e.g. 从cbuf取数据
	cbuffer_t *cbuf = (cbuffer_t *)file;
	u32 rlen;
    do {
        rlen = cbuf_get_data_size(cbuf);
        rlen = rlen > len ? len : rlen;
        if (cbuf_read(cbuf, data, rlen) > 0) {
            len = rlen;
            break;
        }
		os_sem_pend(&r_sem,0);
        if (!run_flag) {
            return 0;
        }
    } while (run_flag);
#else
	//空中传输接收数据然后写入data
#endif


    return len;
}
#endif

static int recorder_vfs_fclose(void *file)
{
    return 0;
}

static int recorder_vfs_flen(void *file)
{
    return 0;
}

static const struct audio_vfs_ops recorder_vfs_ops = {
	.fwrite  = recorder_vfs_fwrite,
	
#ifdef TEST_DEC_LC3
	.fread  = recorder_vfs_fread,
#endif
    .fclose = recorder_vfs_fclose,
    .flen   = recorder_vfs_flen,

};


//可自己调用该函数,也可设置编码时间,时间到回调AUDIO_SERVER_EVENT_END事件关闭
static int test_enc_close(void)
{
    union audio_req req = {0};

    if (enc_server) {
        req.enc.cmd = AUDIO_ENC_CLOSE;
        server_request(enc_server, AUDIO_REQ_ENC, &req);
	}
	
	if(cahe_buf){
		free(cahe_buf);
	}

    return 0;
}

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
		printf("-------%s-------%d\n\r",__func__,__LINE__);
        test_enc_close();
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        break;
    default:
        break;
    }
}




static void enc_test()
{
	int err;
	union audio_req req = {0};

    enc_server = server_open("audio_server", "enc");
	if(!enc_server){
		printf("failed-------%s-------%d\n\r",__func__,__LINE__);
		return ;	
	}
	//将事件回调注册到app_core
    server_register_event_handler_to_task(enc_server, NULL, enc_server_event_handler, "app_core");
	
	req.enc.cmd 			= AUDIO_ENC_OPEN;
	req.enc.frame_size 		= test_sr / 100 * 4 * test_ch;
	req.enc.output_buf_len 	= req.enc.frame_size * 3;
	req.enc.channel 		= test_ch;
	req.enc.volume 			= 50;
	req.enc.sample_rate 	= test_sr;
	req.enc.bitrate 		= test_br;
	req.enc.sample_source 	= "iis0";	//数据输入方式
	req.enc.file			= (FILE *)&save_cbuf;
	req.enc.vfs_ops 		= &recorder_vfs_ops;
	req.enc.format			= "lc3";
	//req.enc.msec			= 30 * 1000;	//单位ms,编码多少时间后关闭编码器

	err = server_request(enc_server, AUDIO_REQ_ENC, &req);
	if(err){
		printf("-------%s-------%d\n\r",__func__,__LINE__);
		return ;	
	}
		

}


void test()
{
	//e.g. 使用cbuf作为例子
	cahe_buf = zalloc(test_sr * test_ch*2 );
	if(!cahe_buf){
		printf("-------%s-------%d\n\r",__func__,__LINE__);
		return ;	
	}
	cbuf_init(&save_cbuf, cahe_buf, test_sr * test_ch*2 );
	os_sem_create(&r_sem,0);
	
#ifdef TEST_DEC_LC3
	dec_test();
#endif
	enc_test();	
}


late_initcall(test);

#ifdef TEST_DEC_LC3



static int test_dec_close(void)
{
    union audio_req req = {0};

    if (dec_server) {
        req.dec.cmd = AUDIO_DEC_STOP;
        server_request(dec_server, AUDIO_REQ_DEC, &req);
	}
	run_flag = 0;
    return 0;
}

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
		printf("-------%s-------%d\n\r",__func__,__LINE__);
        test_dec_close();
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        break;
    default:
        break;
    }
}




static void dec_test()
{
	int err;
	union audio_req req = {0};


    dec_server = server_open("audio_server", "dec");
    server_register_event_handler_to_task(dec_server, NULL, dec_server_event_handler, "app_core");
	
	req.dec.cmd 			= AUDIO_DEC_OPEN;
	req.dec.output_buf_len 	= 4* 1024;
	req.dec.channel 		= test_ch;
	req.dec.volume 			= 50;
	req.dec.dec_type 		= "lc3";
	req.dec.sample_rate 	= test_sr;
	req.dec.sample_source 	= "dac";	//e.g. 这里用dac播放作为测试, 若是通过iis发送,则填对应iis,比如iis0
	req.dec.file 			= &save_cbuf; 
	req.dec.vfs_ops 		= &recorder_vfs_ops;

	err = server_request(dec_server, AUDIO_REQ_DEC, &req);
	if(err){
		printf("-------%s-------%d\n\r",__func__,__LINE__);
		return ;	
	}
		
	run_flag = 1;
	req.dec.cmd = AUDIO_DEC_START;
	err = server_request(dec_server, AUDIO_REQ_DEC, &req);
	if(err){
		printf("-------%s-------%d\n\r",__func__,__LINE__);
		return ;	
	}
}


#endif

//******board.c板级iis配置参考*********//

/* // host data in */
/* #if 1 */
/* static const struct iis_platform_data iis0_data = { */
    /* .channel_in = BIT(0), //PA7 */
    /* .channel_out = 0, //通道0设为输出PA7 */
    /* .port_sel = IIS_PORTA, */
	/* .data_width = BIT(0) | BIT(0 + 4), //BIT(x)代表通道x使用24bit模式 (channel | channel+4) 使用32bit mode */
    /* .mclk_output = 1, //1:输出mclk 0:不输出mclk */
    /* .slave_mode = 0, //1:从机模式 0:主机模式 */
    /* .dump_points_num = 320, //丢弃刚打开硬件时的数据点数 */
/* }; */
/* #else // slave data out */

/* static const struct iis_platform_data iis0_data = { */
    /* .channel_in = 0,  */
    /* .channel_out = BIT(0), //通道0设为输出PA7 */
    /* .port_sel = IIS_PORTA, */
	/* .data_width = BIT(0) , //BIT(x)代表通道x使用24bit模式 32bit mode */
    /* .mclk_output = 0, //1:输出mclk 0:不输出mclk */
    /* .slave_mode = 1, //1:从机模式 0:主机模式 */
    /* .dump_points_num = 320, //丢弃刚打开硬件时的数据点数 */
/* }; */
/* #endif */

//******************************************//
