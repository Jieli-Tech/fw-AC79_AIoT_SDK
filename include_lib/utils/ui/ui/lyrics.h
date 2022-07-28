#ifndef _LYRICS_H_
#define _LYRICS_H_

#include "typedef.h"
#include "fs/fs.h"

#define LRC_SIZEOF_ALIN(var, al)	((((var)+(al)-1)/(al))*(al))

//类型声明区
typedef struct _TIME_LABEL { /*时间标签信息[mm:ss.ms]*/
    u16 dbtime_s;	 //time of label,unit:s
    u8  btime_100ms; //time of label,unit:ms
    u8  btext_len;	 //the length of lrc content, 占用的实际字节数
    u32 wline_pos; 	 //for record next (byte addr)  after the (real label of time)
} TIME_LABEL;

typedef struct __LRC_FILE_IO {
    u32(*seek)(FILE *file, int offset, int orig);
    u32(*read)(FILE *file, void *buf, u32 len);
} LRC_FILE_IO;

typedef struct _LABEL_INFO { /*标签处理后信息*/
    u16 dbtime_base;    //first of time
    u16 dbtime_limit;   //end of time
    u8 base_100ms;      //first of time
    u8 limit_100ms;     //first of time
    u16 dblabel_cnt;    //(real label of time) count
    TIME_LABEL *g_plabel_buf;   //解析过程中时间标签存储buf
    u8 *plabel_buf_tmp;
    u16 plabel_buf_len; //解析过程中时间标签存储buf长度
} LABEL_INFO;



typedef struct _SORTING_INFO { /*文件解析信息*/
    u16 dbnow_fp_addr;//current addr of file pointer
    u8  bdata_len;    //the length of lrc content
    u8  bis_next_file;//jump to next file
} SORTING_INFO;


typedef struct __LRC_FILE {
    void *hdl;
    LRC_FILE_IO *_io;
} LRC_FILE;



typedef struct _LRC_INFO { ///</*lrc显示信息*/
    u8    coding_type;         ///<歌词unicode编码格式
    ///<显示相关的
    u8    bis_lrc_update;      ///<lrc显示歌词更新标志
    u8    broll_speed_control; ///<lrc显示滚动速度控制标志
    u8    blcd_roll_speed;     ///<lcd显示歌词内容滚动速度
    u8    content_len;         ///<当前歌词内容长度
    u8    lrc_data_len;        ///<实际歌词显示内容长度

    //CFG
    u16   blrc_buf_len;        ///<lrc显示内容buf长度,配置
    u8   *blrc_buf;            ///<lrc显示内容buf地址

    ///<file相关的,CFG
    u8   *lrc_read_buf;         ///<lrc读取数据buffer
    LRC_FILE file;				///lrc文件操作控制
    /* void *lrc_file_hdl;         ///<lrc文件句柄 */
    u32   cur_faddr;			///当前文件文件位置
    u16   once_read_len;        ///<一次读取的长度，需要配置
    u16   real_len;             ///<真实一次读取到的数据长度
    u16   data_len_count;       ///<缓存中已分析的数据offset
    u16   label_id;             ///<时间标签读取id（0~）
    u16   lrc_label_len;        ///<解析后标签数据长度


    bool  bfirst_lable;         ///<第一个时间标签标记
    bool  blast_lable;          ///<最后一个时间标签标记
    u8    lrc_text_id;          ///text id
    u8    read_next_lrc_flag;   ///是否预读下一条歌词lrc
    u8 	  save_flash;
    SORTING_INFO   *sorting;    ///<歌词文件解析信息
    LABEL_INFO     *lab_info;   ///<标签处理后信息

    void (*roll_speed_ctrl_cb)(u8 lrc_len, u32 time_gap, u8 *roll_speed);///翻页速度控制
    void (*clr_lrc_disp_cb)(void);

} LRC_INFO;



typedef struct __LRC_CFG {
    u16 once_read_len;///一次读取长度配置
    u16 once_disp_len;///一次显示缓存长度配置
    u16 label_temp_buf_len;///时间标签缓存总长度配置
    u8  lrc_text_id;         ///text id
    u8  read_next_lrc_flag;    ///是否预读下一条歌词lrc
    u8	enable_save_lable_to_flash;//使能保存时间标签到flash，主要是解决长歌词文件不支持问题
    void (*roll_speed_ctrl_cb)(u8 lrc_len, u32 time_gap, u8 *roll_speed);///速度控制配置
    void (*clr_lrc_disp_cb)(void);///清屏回调
} LRC_CFG;


extern void lrc_destroy(void);
extern int lrc_param_init(LRC_CFG *cfg, u8 *lrc_info_buf);
extern bool lrc_analysis(void *lrc_handle, const LRC_FILE_IO *file_io);
extern bool lrc_get(u16 dbtime_s, u8 btime_100ms);
extern bool lrc_show(int text_id, u16 dbtime_s, u8 btime_100ms);

enum {
    LRC_GBK = 0, //内码
    LRC_UTF16_S, //unicode小端码
    LRC_UTF16_B, //unicode大端码
    LRC_UTF8,
};


#endif

