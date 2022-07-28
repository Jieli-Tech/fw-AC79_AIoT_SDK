#include "server/audio_server.h"
#include "app_config.h"

#if CONFIG_DEC_DECRYPT_ENABLE

#ifndef CONFIG_DEC_DECRYPT_KEY
#define CONFIG_DEC_DECRYPT_KEY 0x12345678
#endif

typedef struct _CIPHER {
    u32 cipher_code;        ///>解密key
    void *file;
    u8  cipher_enable;      ///>解密读使能
} CIPHER;


static void cipher_ctl(CIPHER *pcipher, u8 ctl)
{
    pcipher->cipher_enable = ctl;
}

/*----------------------------------------------------------------------------*/
/**@brief  解密读文件数据的回调函数，用于底层的物理读
  @param  void* buf, u32 lba
  @return 无
  @note   void cryptanalysis_buff(void* buf, u32 faddr, u32 len)
 */
/*----------------------------------------------------------------------------*/
#define ALIN_SIZE	4

static void cryptanalysis_buff(CIPHER *pcipher, void *buf, u32 faddr, u32 len)
{
    u32 i;
    u8 j;
    u8 head_rem;//
    u8 tail_rem;//
    u32 len_ali;

    u8 *buf_1b_ali;
    u8 *cipher_code;

    cipher_code = (u8 *)&pcipher->cipher_code;

    if (!pcipher->cipher_enable) {
        return;
    }
    /* log_info("----faddr = %d \n",faddr); */
    /* put_buf(buf,len); */
    /* log_info("buf_addr = %d \n", buf); */

    head_rem = ALIN_SIZE - (faddr % ALIN_SIZE);
    if (head_rem == ALIN_SIZE) {
        head_rem = 0;
    }
    if (head_rem > len) {
        head_rem = len;
    }

    if (len - head_rem) {
        tail_rem = (faddr + len) % ALIN_SIZE;
    } else {
        tail_rem = 0;
    }
    /* log_info("head_rem = %d tail_rem = %d \n", head_rem, tail_rem); */
    /* log_info("deal_head_buf\n"); */
    buf_1b_ali = buf;
    j = 3;
    for (i = head_rem; i > 0; i--) {
        buf_1b_ali[i - 1] ^= cipher_code[j--];
        /* log_info("i = %d \n", i - 1); */
        /* log_info("buf_1b_ali[i] = %x \n", buf_1b_ali[i - 1]); */
    }
    /* log_info("\n\n-----------TEST_HEAD-----------------"); */
    /* put_buf(buf_1b_ali, head_rem); */
    /* log_info("deal_main_buf\n"); */
    buf_1b_ali = buf;
    buf_1b_ali = (u8 *)(buf_1b_ali + head_rem);
    len_ali = len - head_rem - tail_rem;
    /* log_info("len_ali = %d \n", len_ali); */
    /* log_info("buf_1b_ali = %d \n", buf_1b_ali); */
    for (i = 0; i < (len_ali / 4); i++) {
        buf_1b_ali[0 + i * 4] ^= cipher_code[0];
        buf_1b_ali[1 + i * 4] ^= cipher_code[1];
        buf_1b_ali[2 + i * 4] ^= cipher_code[2];
        buf_1b_ali[3 + i * 4] ^= cipher_code[3];
    }
    /* log_info("\n\n-----------TEST_MAIN-----------------"); */
    /* put_buf(buf_1b_ali, len_ali); */

    /* log_info("deal_tail_buf\n"); */
    buf_1b_ali = buf;
    buf_1b_ali += len - tail_rem;
    j = 0;
    for (i = 0 ; i < tail_rem; i++) {
        buf_1b_ali[i] ^= cipher_code[j++];
    }
    /* log_info("\n\n-----------TEST_TAIL-----------------"); */
    /* put_buf(buf_1b_ali, tail_rem); */

    /* log_info("\n\n-----------TEST-----------------"); */
    /* put_buf(buf,len); */
}

static void cipher_check_decode_file(CIPHER *pcipher, void *file)
{
    int rlen;
    u8 name[16] = {0};

    rlen = fget_name((FILE *)file, name, sizeof(name) - 1);
    /* printf("rlen:%d \n", rlen); */
    /* put_buf(name, sizeof(name)); */

    while (rlen--) {
        if ((name[rlen] >= 'a') && (name[rlen] <= 'z')) {
            name[rlen] = name[rlen] - 'a' + 'A';
        }
        if (name[rlen] != '.') {
            continue;
        }
        /* printf("file exname : %s \n", &name[rlen+1]); */
        /* printf("rlen:%d \n", rlen); */
        /* put_buf(name, sizeof(name)); */
        if (((name[rlen + 1] == 'S') && (name[rlen + 2] == 'M') && (name[rlen + 3] == 'P')) // assci
            || ((name[rlen + 2] == 'S') && (name[rlen + 4] == 'M') && (name[rlen + 6] == 'P')) // unicode
           ) {
            printf("\n----It's a SMP FILE---\n");
            cipher_ctl(pcipher, 1);
        }
        return;
    }

    cipher_ctl(pcipher, 0);
}

static void cipher_init(CIPHER *pcipher, u32 key)
{
    pcipher->cipher_code = key;
    cipher_ctl(pcipher, 0);
}

static void cipher_close(CIPHER *pcipher)
{
    cipher_ctl(pcipher, 0);
}

static void *decrypt_fopen(const char *file, const char *mode)
{
    if (!file) {
        return NULL;
    }

    CIPHER *pcipher = (CIPHER *)malloc(sizeof(CIPHER));
    if (!pcipher) {
        return NULL;
    }
    cipher_init(pcipher, CONFIG_DEC_DECRYPT_KEY);
    cipher_check_decode_file(pcipher, (void *)file);
    pcipher->file = (void *)file;

    return pcipher;
}

static int decrypt_fread(void *priv, void *buf, u32 len)
{
    CIPHER *pcipher = (CIPHER *)priv;

    if (!pcipher || !pcipher->file) {
        return -1;
    }

    FILE *file = pcipher->file;
    int rlen = fread(buf, len, 1, file);

    if (rlen && (rlen <= len)) {
        cryptanalysis_buff(pcipher, buf, fpos(file), rlen);
    }

    return rlen;
}

static int decrypt_fseek(void *priv, u32 offset, int orig)
{
    CIPHER *pcipher = (CIPHER *)priv;

    if (!pcipher || !pcipher->file) {
        return -1;
    }

    return fseek(pcipher->file, offset, orig);
}

static int decrypt_flen(void *priv)
{
    CIPHER *pcipher = (CIPHER *)priv;

    if (!pcipher || !pcipher->file) {
        return -1;
    }

    return flen(pcipher->file);
}

static int decrypt_fclose(void *pcipher)
{
    if (pcipher) {
        cipher_close((CIPHER *)pcipher);
        free(pcipher);
    }
    return 0;
}

static const struct audio_vfs_ops decrypt_vfs_ops = {
    .fopen  = decrypt_fopen,
    .fread  = decrypt_fread,
    .fseek  = decrypt_fseek,
    .flen   = decrypt_flen,
    .fclose = decrypt_fclose,
};

const struct audio_vfs_ops *get_decrypt_vfs_ops(void)
{
    return &decrypt_vfs_ops;
}

#endif

