#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "fs/fs.h"
#define MAX_DEEPTH 3 //目录搜索最大深度

#ifdef USE_FS_TEST_DEMO
//测试例选择
#define FILENAME_TEST 1          //长短文件名打开和目录创建操作测试
#define FS_FSCAN_TEST 0          //扫描文件测试 fscan和fselect使用


//长短文件名打开和目录创建操作测试
void filename_test(void)
{
    u8 *test_data = "filename test123"; //写入文件数据
    u8 read_buf[32];//读取数据buf
    int len;//文件大小
    char path[256] = "storage/sd0/C";//路径
    FILE *fd = NULL;//文件句柄指针

    //文件名：longfilename.txt  通过字符转UTF16LE编码写入
    char file_name[128] = {'/', '\\', 'U', 0x6C, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x66, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x61, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x2E, 0x00, 0x74, 0x00, 0x78, 0x00, 0x74, 0x00};

    //文件名：hello/测试创建长文件名测试.txt
    //char file_name[128] = {'/','h', 'e','l','l','o','/','\\','U',0x4B,0x6D,0xD5,0x8B,0x1B,0x52,0xFA,0x5E,0x7F,0x95,0x87,0x65,0xF6,0x4E,0x0D,0x54,0x4B,0x6D,0xD5,0x8B,0x2E,0x00,0x74,0x00,0x78,0x00,0x74,0x00};

    //文件夹名：测试创建长文件名测试
    //char file_name[128] = { '/','\\','U',0x4B,0x6D,0xD5,0x8B,0x1B,0x52,0xFA,0x5E,0x7F,0x95,0x87,0x65,0xF6,0x4E,0x0D,0x54,0x4B,0x6D,0xD5,0x8B};


    //  测试长文件夹名测试/测试创建长文件名测试.txt
    /* char file_name[128] = {'/', '\\', 'U', 0x4B, 0x6D, 0xD5, 0x8B, 0x7F, 0x95, 0x87, 0x65, 0xF6, 0x4E, 0x39, 0x59, 0x0D, 0x54, 0x4B, 0x6D, 0xD5, 0x8B, '/', 0x00, '\\', 'U', 0x4B, 0x6D, 0xD5, 0x8B, 0x1B, 0x52, 0xFA, 0x5E, 0x7F, 0x95, 0x87, 0x65, 0xF6, 0x4E, 0x0D, 0x54, 0x4B, 0x6D, 0xD5, 0x8B, 0x2E, 0x00, 0x74, 0x00, 0x78, 0x00, 0x74, 0x00}; */



    memcpy(path + strlen(path), file_name, sizeof(file_name)); //将文件名file_name复制到path路径后


    //创建并写文件
    fd = fopen(path, "w+");//打开可 读/写文件，若文件存在则文件长度清为零，即该文件内容会消失；若文件不存在则创建该文件。

    //FILE *fd = fopen(CONFIG_ROOT_PATH"test/test.txt", "w+");

    if (fd) {
        fwrite(test_data, 1, strlen(test_data), fd);//写入文件数据
        fclose(fd);//关闭文件
        printf("write file : %s \n", read_buf);
    }

    //读文件
    fd = fopen(path, "r");//以只读方式打开文件，该文件必须存在。
//fd = fopen(CONFIG_ROOT_PATH"test/test.txt", "r");
    if (fd) {
        len = flen(fd);//获取整个文件大小
        fread(read_buf, 1, len, fd);//读取文件
        fclose(fd);//关闭文件
        printf("read file : %s \n", read_buf);
    }
}


//扫描文件测试  fscan和fselect使用
void fs_fscan_test(void)
{
    u8 name[128];
    struct vfscan *fs = NULL;//文件扫描结构体句柄
    FILE *fp = NULL;//文件句柄指针
    struct vfs_attr attr = {0};
    u32 first_clust;
    //扫描CONFIG_STORAGE_PATH"/C"路径中，搜索.txt格式文件-tTXT，包括子目录-r，按照按照文件号排序-sn
    fs = fscan(CONFIG_STORAGE_PATH"/C", "-tTXT -r -sn", MAX_DEEPTH);
    if (fs == NULL) {//判断是否扫描成功
        printf("fscan_faild\n");
        return;
    }

    printf("fscan: file_number = %d\n", fs->file_number);

    if (fs->file_number == 0) { //判断是否有被扫描格式文件存在
        printf("Please create some test files!");
        return;
    }

    //选择第一个文件
    fp = fselect(fs, FSEL_FIRST_FILE, 0);
    if (fp) {
        fget_name(fp, name, sizeof(name));//获取文件名 unicode(utf16le)编码格式名
        put_buf(name, sizeof(name)); //打印文件名
        fclose(fp);//关闭文件
        fp = NULL;//释放文件指针
    }

    //通过文件号选择文件
    fp = fselect(fs, FSEL_BY_NUMBER, 2);
    if (fp) {
        fget_name(fp, name, sizeof(name));
        put_buf(name, sizeof(name));
    }

    //获取文件首簇号
    fget_attrs(fp, &attr);
    first_clust = attr.sclust;

    //通过簇号选择文件
    fp = fselect(fs, FSEL_BY_SCLUST, first_clust);
    if (fp) {
        fget_name(fp, name, sizeof(name));
        put_buf(name, sizeof(name));
    }

    fclose(fp);
    fp = NULL;
}



static void sd_fs_test(void)
{
    extern int storage_device_ready(void);
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }


#if FILENAME_TEST
    filename_test();
#endif


#if FS_FSCAN_TEST
    fs_fscan_test();
#endif

    while (1) {
        os_time_dly(2);
    }

}


static int c_main(void)
{
    os_task_create(sd_fs_test, NULL, 12, 1000, 0, "sd_fs_test");
    return 0;
}
late_initcall(c_main);
#endif
