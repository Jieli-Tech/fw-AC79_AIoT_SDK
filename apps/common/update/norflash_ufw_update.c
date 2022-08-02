#include "app_config.h"
#include "typedef.h"
#include "fs.h"
#include "norflash.h"
#include "spi/nor_fs.h"
#include "rec_nor/nor_interface.h"
#include "update_loader_download.h"
#include "update.h"

typedef struct _nor_update_info {
    u32 src_file_addr;
    u32 src_file_len;
} nor_update_info_parm;

static nor_update_info_parm nor_update_info;
static void *update_fp = NULL;

//=============================================================//
//                  外挂NorFlash升级V2版本                     //
/* APP NorFlash 升级模型(APP_NORFLASH_UFW_UPDATA)
  ________                  ________                  ________
 |        |                |        |                |        |
 |        |     (1)        |        |    (2)         |        |
 |   APP  | --------->     |NorFlash|   -------->    |  CHIP  |
 |        | nor_update.ufw |        | nor_update.ufw |        |
 |________|                |________|                |________|

NOTE:
 关于流程(1): 该流程为从远端获取升级文件(nor_update.ufw)写到外置flash某个位置, nor_update.ufw文件在外置flash中需要保证在物理上连续存储, 注意该流程为用户在自己定义的ota中完成, 不在本文件流程中, 需要用户自己开发;
 关于流程(2): 该流程为本文件实现的升级流程, 改动较少(主要改动为跟读取外置flash的硬件相关接口), 在用户完成流程(1)后(nor_update.ufw已存在外置flash), 可随时调用norflash_update_ufw_init函数启动升级;

 */
//============================================================//

#define NORFLASH_UFW_UPDATE_VERIFY_ALL_FILE 		1 //1: 进升级前校验用户写到外置flash的nor_update.ufw文件; 0: 不校验

extern const int support_norflash_ufw_update_en;

//=========================================================//
// norflash 操作接口
//=========================================================//

static u32 user_remote_base_addr = 0; //
static u32 user_remote_cur_offset = 0;
static void *dev_ptr = NULL;
/*----------------------------------------------------------------------------*/
/**@brief  获取升级文件在外flash存放位置和长度
   @param  buf, 结构为nor_update_info
   @return void
   @note
*/
/*----------------------------------------------------------------------------*/
static void get_nor_update_info_param(void *buf)
{
    if (buf) {
        printf("src_file_addr = 0x%x", nor_update_info.src_file_addr); //外挂flash存放flash.bin文件物理地址
        printf("src_file_len = 0x%x", nor_update_info.src_file_len);   //外挂flash存放flash.bin文件物理地址
        memcpy(buf, (u8 *)&nor_update_info, sizeof(nor_update_info));
    }
}



static u32 user_remote_device_get_cur_addr(void)
{
    return (user_remote_base_addr + user_remote_cur_offset);
}
/*----------------------------------------------------------------------------*/
/**@brief   打开外置flash升级文件(初始化外置flash读接口)
   @param   void
   @return  0: 打开错误;
   		   非0: 打开成功
   @note
*/
/*----------------------------------------------------------------------------*/
static u16 norflash_ufw_update_f_open(void)
{
    printf("%s", __func__);

    dev_ptr = dev_open("res_nor", NULL);
    if (dev_ptr) {
        printf("open dev succ 0x%x", dev_ptr);
    } else {
        printf("open dev fail");
        return 0;
    }

    return 1;
}

/*----------------------------------------------------------------------------*/
/**@brief   读取外置flash升级文件内容
   @param   fp: NULL, 保留
   @param   buff: 读取数据buf
   @param   len: 读取数据长度
   @return  len: 读取成功
            (-1): 读取出错
   @note
*/
/*----------------------------------------------------------------------------*/
static u16 norflash_ufw_update_f_read(void *fp, u8 *buff, u16 len)
{
    //TODO:
    int rlen = 0;
    if (dev_ptr) {
        wdt_clear();
        rlen = dev_bulk_read(dev_ptr, buff, user_remote_device_get_cur_addr(), len);
        //printf("%s: addr = 0x%x, rlen = %d, dev_ptr = 0x%x", __func__, user_remote_device_get_cur_addr(), rlen, dev_ptr);
        user_remote_cur_offset += len;
        //put_buf(buff, 128);
    }

    return len;
}

/*----------------------------------------------------------------------------*/
/**@brief   偏移升级文件地址
   @param   fp: NULL, 保留
   @param   type: SEEK_SET, SEEK_CUR, SEEK_END
   @param   offset: 偏移长度
   @return  0: 操作成功
            非0: 操作出错
   @note
*/
/*----------------------------------------------------------------------------*/
static int norflash_ufw_update_f_seek(void *fp, u8 type, u32 offset)
{
    //TODO:
    //printf("%s", __func__);
    if (type == SEEK_SET) {
        user_remote_cur_offset = offset;
    } else {
        ASSERT(0, "ONLY SUPPORT SEEK_SET");
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   关闭外置flash读操作
   @param   err: 传入升级状态
   @return  0: 操作成功
            非0: 操作出错
   @note
*/
/*----------------------------------------------------------------------------*/
static u16 norflash_ufw_update_f_stop(u8 err)
{
    //TODO:
    printf("%s, err = 0x%x", __func__, err);

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   通知升级文件大小
   @param   priv: NULL, 保留
   @param   size: 待升级文件大小
   @return  0: 操作成功
            非0: 操作出错
   @note
*/
/*----------------------------------------------------------------------------*/
static int norflash_ufw_update_notify_update_content_size(void *priv, u32 size)
{
    //TODO:
    printf("%s: size: 0x%x", __func__, size);

    return 0;
}


static const update_op_api_t norflash_ufw_update_op_api = {
    .ch_init = NULL,
    .f_open = norflash_ufw_update_f_open,
    .f_read = norflash_ufw_update_f_read,
    .f_seek = norflash_ufw_update_f_seek,
    .f_stop = norflash_ufw_update_f_stop,
    .notify_update_content_size = norflash_ufw_update_notify_update_content_size,
};

/*----------------------------------------------------------------------------*/
/**@brief   外置flash升级文件校验完成, 会调用该函数
   @param   priv: 回调传入参数
   @param   type: 升级模式
   @param   cmd: 1) UPDATE_LOADER_OK 外置flash升级文件校验成功, cpu复位, 启动内置flash固件升级流程
   				 2) UPDATE_LOADER_ERR 外置flash升级文件校验失败
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void norflash_ufw_update_end_callback(void *priv, int type, u8 cmd)
{
    //TODO:
    printf("%s: type: 0x%x, cmd = 0x%x", __func__, type, cmd);

    if (type == USER_NORFLASH_UFW_UPDATA) {
        if (cmd == UPDATE_LOADER_OK) {
            printf("soft reset to update >>>");
            cpu_reset(); //复位让主控进入升级内置flash
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   填充升级结构体私有参数
   @param   p: 升级结构体指针(UPDATA_PARM)
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void norflash_ufw_update_private_param_fill(UPDATA_PARM *p)
{
    get_nor_update_info_param(p->parm_priv);
}

/*----------------------------------------------------------------------------*/
/**@brief   固件升级校验流程完成, cpu reset跳转升级新的固件
   @param   type: 升级类型
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void norflash_ufw_update_before_jump_handle(int type)
{
    printf("soft reset to update >>>");
    cpu_reset(); //复位让主控进入升级内置flash
}

/*----------------------------------------------------------------------------*/
/**@brief   外置norflash升级流程状态处理
   @param   type: 升级类型
   @param   state: 当前升级状态
   @param   priv: 跟状态相关的私有参数指针
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void norflash_ufw_update_state_cbk(int type, u32 state, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;

    printf("state:%x err:%x\n", ret_code->stu, ret_code->err_code);

    switch (state) {
    case UPDATE_CH_EXIT:
        if ((0 == ret_code->stu) && (0 == ret_code->err_code)) {
            //update_mode_api(BT_UPDATA);
            update_mode_api_v2(USER_NORFLASH_UFW_UPDATA,
                               norflash_ufw_update_private_param_fill,
                               norflash_ufw_update_before_jump_handle);
        }

        break;

    default:
        break;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  通过地址读取升级文件内容
   @param  buf, 读取数据缓存
   @param  addr, 读取外置flash数据的地址
   @param  len, 读取外置flash数据的长度
   @return u16 读取长度
   @note
*/
/*----------------------------------------------------------------------------*/
static u16 norflash_update_file_read(void *buf, u32 addr, u32 len)
{
    int rlen = 0;

    if (dev_ptr) {
        wdt_clear();
        //printf("%s: addr = 0x%x, rlen = 0x%x", __func__, addr, len);
        rlen = dev_bulk_read(dev_ptr, buf, addr, len);
        //put_buf(buff, 128);
    }

    return rlen;
}

/*----------------------------------------------------------------------------*/
/**@brief  启动外置flash升级接口, 该接口为用户在保证将nor_update.ufw烧写到外置flash之后, 调用该接口进行内置flash升级
   @param  norflash_addr: 升级文件在外置flash的物理地址
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
//使用ufw文件格式
void norflash_update_ufw_init(u32 norflash_addr)
{
    if (support_norflash_ufw_update_en == 0) {
        return;
    }
    user_remote_base_addr = norflash_addr;
    nor_update_info.src_file_addr = norflash_addr;

#if NORFLASH_UFW_UPDATE_VERIFY_ALL_FILE
    if (norflash_ufw_update_f_open()) {
        if (update_file_verify(norflash_addr, norflash_update_file_read) != 0) {
            //校验失败
            return;
        }
    }
#endif /* #if NORFLASH_UFW_UPDATE_VERIFY_ALL_FILE */

    update_mode_info_t nofrflash_ufw_update_info = {
        .type = USER_NORFLASH_UFW_UPDATA,
        .state_cbk = norflash_ufw_update_state_cbk,
        .p_op_api = &norflash_ufw_update_op_api,
        .task_en = 1,
    };

    app_active_update_task_init(&nofrflash_ufw_update_info);
}


