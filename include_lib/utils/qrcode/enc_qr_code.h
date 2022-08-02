#ifndef _ENC_QR_CODE_H
#define _ENC_QR_CODE_H
enum code_result {
    success = 1,
    Input_mode_err,
    EAN_13_input_err,
    EAN_13_output_err,
    CODE_39_input_err,
    CODE_39_input_data_err,
    CODE_39_output_err,
    CODE_128_input_err,
    CODE_128_output_err,
    QR_CODE_input_err,
    QR_CODE_check_ecc_err,
    QR_CODE_version_err,
    QR_CODE_add_ecc_err,
};
typedef struct {
    int l_size;
} jl_code_param_t;
void jl_code_init(int code128_mode, unsigned char qr_version, unsigned char qr_max_version, unsigned char qr_ecc_level, int qr_code_max_input_len_, int qr_buf_size, int img_w);
int jl_code_process(unsigned char mode_t, char *src, int length, int *out_len, int *line_size);
void jl_code_set_info(jl_code_param_t *jl_code_param_);
void jl_code_get_data(int len, int row, unsigned char *outdata);
void jl_code_deinit();
#endif

/*
  EAN_13:只能编码0-9的数字，且输入数据长度必须是12。
  Code_39: "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ+- /%$.",Code_39只能编码如上的字符。
  Code_128:ASCII码的字符都可以编码
Qr_code:ASCII码的字符都可以编码
  条形码输入，输出长度说明：
EAN_13:输入长度12，输出长度113
  Code_39:输入长度设为l，输出长度为（l+2）*13。图像宽度为width ,则最大输入长度为(width  / 13) - 2;（一个输出至少占两个像素）
  Code_128:输入长度设为l，输出最大长度为（l+3）*11+2，图像宽度为width ,则最大输入长度为((width -2) / 11) - 3;（一个输出至少占两个像素）
  函数接口说明：void jl_code_init(int code128_mode,uint8_t qr_version, uint8_t qr_max_version,uint8_t qr_ecc_level,int qr_code_max_input_len_,int qr_buf_size,int img_w);
  描述：初始化函数
  code128_mode:code_128的编码方式。设置为60，则会把本该按照版本C编码的字符按照版本B去编；不想这么做则将其设为其它的任何值。
  qr_version：qr_code的版本号。该值设置为1-qr_max_version时，编码时会按照该版本号进行编码。设置为其他值时，编码时内部会计算自动的版本号。
  qr_max_version：qr_code的最大版本号。会根据最大版本号开辟内存。建议不超过版本12。
  qr_ecc_level：qr_code的纠错等级。该值设置为1-4时，编码时会按照该纠错等级进行编码。设置为其他值时，编码时内部会计算自动的纠错等级。
  qr_code_max_input_len_：qr_code的最大输入长度。建议不超过384。
  qr_buf_size：qr_code内部用的buffer大小。建议给4K。
  img_w：图像的宽度。
  int jl_code_process(unsigned char mode_t,char * src, int length, int *out_len,int *line_size)
描述：编码函数
mode_t:编码模式。0：EAN13；1：code_39;2:code_128;3:qr_code。
  src：待编码数据。
  length:待编码数据的实际长度（注意不能超过对应模式的最大长度）
out_len：码的实际长度
line_size：一个码元素最大占的像素个数
  返回值：若不为1，则说明没有编码，可能是输入数据格式不对或输入数据超过最大长度等。若为1，则代表编码成功。
  lvoid jl_code_set_info(jl_code_param_t *jl_code_param_)
  描述：设置一个码元素占的像素个数，注意不能超过一个码元素最大占的像素个数。
void jl_code_get_data(int len,int row, unsigned char *outdata)
  描述：获取编码数据
  len：码的实际长度
  row：当为二维码时，表示获取第几行的数据；否则，设置为0即可。
  outdata：编码数据buf
  void jl_code_deinit();
  描述：结束时调用该函数，用于释放内存。
  内存大小：
  EAN_13,code_39,code_128:  大约1831bit
  qr_code:	大约1826bit
  jl_code_init: qr_buf_size+4*(21+(qr_max_version-1))*(21+(qr_max_version-1))
  若qr_buf_size为4096，qr_max_version为12，则20996
  合计：24653bit，约为24K。
 */
