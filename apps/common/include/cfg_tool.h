#ifndef _APP_CFG_TOOL_
#define _APP_CFG_TOOL_

#include "generic/typedef.h"
#include "fs/sdfile.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"

/*T 表示包的类型*/
#define REPLY_STYLE 		0x00	//无论哪种模式下，回复包都用该种类型
#define INITIATIVE_STYLE 	0x12	//主动发送数据的包用该类型
#define SLAVE_ATIVE_SEND    0x01	//小机主动上发给PC

/*
 *0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |   VERSION |AT| X| X| X| X| X| X| X| X| X| X| X|
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
#define PROTOCOL_VER_AT_NEW		0x0001  //使用新模式(BR23, ATK 0x11)
#define PROTOCOL_VER_AT_OLD		0x0011  //使用旧模式(BR25, EQ 0x05, 音效 0x06)


/*新配置工具文件所属文件ID为0*/
#define CFG_TOOL_FILEID		0x00000000

#if (defined CONFIG_NEW_CFG_TOOL_ENABLE)
#define CFG_TOOL_FILE		SDFILE_RES_ROOT_PATH"cfg_tool.bin"
#else
#define CFG_TOOL_FILE		SDFILE_APP_ROOT_PATH"cfg_tool.bin"
#endif

/*旧调音工具eq文件所属文件ID为1*/
#define CFG_OLD_EQ_FILEID		0x00000001
#define CFG_OLD_EQ_FILE			SDFILE_RES_ROOT_PATH"cfg/eq_cfg_hw.bin"

/*旧调音工具混响文件所属文件ID为2*/
#define CFG_OLD_EFFECT_FILEID	0x00000002
#define CFG_OLD_EFFECT_FILE		SDFILE_RES_ROOT_PATH"cfg/effects_cfg.bin"

/*新调音工具eq文件所属文件ID为3*/
#define CFG_EQ_FILEID			0x00000003
#define CFG_EQ_FILE				SDFILE_RES_ROOT_PATH"cfg/eq_cfg_hw.bin"

/*****************************************************************/
/****PC与小机使用到的CMD，CMD包含在DATA中，为DATA的前4个Byte******/
/*****************************************************************/

#define ONLINE_SUB_OP_QUERY_BASIC_INFO 			0x00000023	//查询固件的基本信息
#define ONLINE_SUB_OP_QUERY_FILE_SIZE 			0x0000000B	//查询文件大小
#define ONLINE_SUB_OP_QUERY_FILE_CONTENT	 	0x0000000C	//读取文件内容
#define ONLINE_SUB_OP_PREPARE_WRITE_FILE	    0x00000022	//准备写入文件
#define ONLINE_SUB_OP_READ_ADDR_RANGE			0x00000027	//读取地址范围内容
#define ONLINE_SUB_OP_ERASE_ADDR_RANGE			0x00000024	//擦除地址范围内容
#define ONLINE_SUB_OP_WRITE_ADDR_RANGE			0x00000025	//写入地址范围内容
#define ONLINE_SUB_OP_ENTER_UPGRADE_MODE		0x00000026	//进入升级模式
#define DEFAULT_ACTION							0x000000FF	//其他工具的数据

/*****************************************************************/
/*****小机接收PC的DATA,具体携带的数据,依据命令不同而不同**********/
/*****************************************************************/

//查询固件的基本信息
typedef struct {
    u32 cmd_id; 		//命令号,为0x23
} R_QUERY_BASIC_INFO;

//查询文件大小
typedef struct {
    u32 cmd_id; 		//命令号,为0x0B
    u32 file_id; 		//查询的文件的ID,配置文件的ID为0
} R_QUERY_FILE_SIZE;

//读取文件内容
typedef struct {
    u32 cmd_id;  		//命令号,为0x0C
    u32 file_id; 		//文件ID
    u32 offset;  		//偏移
    u32 size;
} R_QUERY_FILE_CONTENT;

//准备写入文件
typedef struct {
    u32 cmd_id; 		//命令号,为0x22
    u32 file_id; 		//文件ID
    u32 size; 			//文件大小
} R_PREPARE_WRITE_FILE;

//读取地址范围内容
typedef struct {
    u32 cmd_id; 		//命令号,为0x23
    u32 addr;   		//flash的物理地址
    u32 size;   		//读取的范围大小
} R_READ_ADDR_RANGE;

//擦除地址范围内容
typedef struct {
    u32 cmd_id;  		//命令号,为0x24
    u32 addr;    		//起始地址
    u32 size;    		//擦除大小
} R_ERASE_ADDR_RANGE;

//写入地址范围内容
typedef struct {
    u32 cmd_id;  		//命令号,为0x25
    u32 addr;    		//物理地址
    u32 size;    		//内容大小
    /*uint8_t  body[0];*/ 	//具体内容,大小为size
} R_WRITE_ADDR_RANGE;

//进入升级模式
typedef struct {
    u32 cmd_id; 		//命令号,为0x26
} R_ENTER_UPGRADE_MODE;

/*****************************************************************/
/*******小机返回PC发送的DATA,具体的内容,依据命令不同而不同********/
/*****************************************************************/

//(1)查询固件的基本信息
typedef struct {
    u16 protocolVer; 	//协议版本，目前为1
    u8 progCrc[32]; 		//固件的CRC，返回字符串，\0 结尾
    u8 sdkName[32]; 		//SDK名字，\0 结尾
    u8 pid[16];     		//PID，\0 结尾（注意最长是16字节，16字节的时候，末尾不需要0）
    u8 vid[16];     		//VID，\0 结尾
} S_QUERY_BASIC_INFO;

//(2)查询文件大小
typedef struct {
    u32 file_size;		//文件大小
} S_QUERY_FILE_SIZE;

//(3)读取文件内容
/*
 *  返回：具体的文件内容,长度为命令中的size参数
 *	注：PC工具在获取配置文件的大小后,会自行决定拆分成几次命令读取
 */

//(4)准备写入文件
/*
 * 例如，cfg_tool.bin 本身的物理地址可能是 100，而擦除单元是 4K（4096），cfg_tool.bin 本身大小是 3999 字节，则
 * file_addr = 100, file_size = 3999, earse_unit = 4096。
 * 注意，接下来，PC 端会读取```[0, 8192)```这个范围的内容，修改掉```[100,4099]```的内容，然后再通过其他命令，
 * 把```[0,8192)```这范围的内容写回去。
 */
typedef struct {
    u32 file_addr;    //配置文件真实物理地址
    u32 file_size;    //配置文件（cfg_tool.bin）本身的大小
    u32 earse_unit;   //flash 擦除单元（如 4K 的时候，填 4096）
} S_PREPARE_WRITE_FILE;


//(5)读取地址范围内容
/*
 *  返回：具体的文件内容,长度为命令中的size参数
 *	注：PC工具在获取配置文件的大小后,会自行决定拆分成几次命令读取
 */

//(6)擦除地址范围内容
/*
 *返回两个字节的内容,"FA"或者"OK",表示失败或者成功
 */

//(7)写入地址范围内容
/*
 *返回两个字节的内容,"FA"或者"OK",表示失败或者成功
 */

//(8)进入升级模式
/*
 *小机收到命令后,进入升级模式
 */

/* --------------------------------------------------------------------------*/
/**
 * @brief usb或者串口收到的数据流通过这个接口传入进行数据解析
 *
 * @param buf 存储普通串口/USB收到的数据包
 * @param len buf的长度(byte)
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
void online_cfg_tool_data_deal(void *buf, u32 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief spp收到的数据流通过这个接口传入进行数据解析
 *
 * @param buf 存储普通spp收到的数据包
 * @param len buf的长度(byte)
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
int cfg_tool_effect_tool_spp_rx_data(u8 *buf, u32 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 设备组装数据包并发送给PC工具，支持普通串口/USB串口/蓝牙SPP
 *
 * @param id  表示包的类型（不同的数据通道）
 * @param sq  对应需要回应的包的序号
 * @param buf 要发送的数据爆DATA部分
 * @param len buf的长度(byte)
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
void all_assemble_package_send_to_pc(u8 id, u8 sq, u8 *buf, u32 len);

struct tool_interface {
    u8 id;//通道
    void (*tool_message_deal)(u8 *buf, u32 len);//数据处理接口
};

//注册通道及数据处理接口，支持spp/usb/uart的数据流分发
#define REGISTER_DETECT_TARGET(interface) \
	static struct tool_interface interface SEC_USED(.tool_interface)

extern struct tool_interface tool_interface_begin[];
extern struct tool_interface tool_interface_end[];

#define list_for_each_tool_interface(p) \
	    for (p = tool_interface_begin; p < tool_interface_end; p++)

#endif

