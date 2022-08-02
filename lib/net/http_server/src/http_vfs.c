
#include "fs/fs.h"
#include "http_server.h"
/********************************************************************************************************************/

static void *http_flashfile_open(const char *path, const char *mode)
{
    void *fp = NULL;
    fp = (void *)fopen(path, mode);
    if (!fp) {
        return NULL;
    }
    return fp;
}

static void http_flashfile_close(void *stream)
{
    fclose(stream);
}

static int http_flashfile_read(void *stream, void *buffer, size_t len)
{
    return fread(buffer, 1, len, stream);
}


static int http_flashfile_flen(void *stream)
{
    return flen(stream);
}

static int http_flashfile_seek(void *stream, int offset, int origin)
{
    int ret = -1;

    switch (origin) {
    case SEEK_SET:
        ret = fseek(stream, offset, SEEK_SET);
        break;
    case SEEK_CUR:
        break;
    case SEEK_END:
        break;
    default:
        break;
    }

    return ret;
}

/********************************************************************************************************************/

static void *http_sdcardfile_open(const char *path, const char *mode)
{
    void *fp = NULL;
    fp = fopen(path, mode);
    if (fp == NULL) {
        return NULL;
    }
    return fp;
}


static int http_sdcardfile_flen(void *stream)
{
    return flen(stream);
}

static void http_sdcardfile_close(void *stream)
{
    if (stream != NULL) {
        fclose(stream);
    }

}

static int http_sdcardfile_seek(void *stream, int offset, int origin)
{
    int ret = -1;

    switch (origin) {
    case SEEK_SET:
        ret = fseek(stream, offset, SEEK_SET);
        break;
    case SEEK_CUR:
        break;
    case SEEK_END:
        break;
    default:
        break;
    }

    return ret;
}

static int http_sdcardfile_read(void *stream, void *buffer, size_t len)
{
    return fread(buffer, 1, len, stream);
}
int http_fattrib_isidr(const char *file_name, u8 *isdir)
{

    if (fdir_exist(file_name)) {
        *isdir = 1;
    } else {
        *isdir = 0;
    }
    return 0;
}

struct vfscan *http_f_opendir(const char *path, const char *arg)
{
    return fscan(path, arg, 3);
    //return mutex_f_opendir(path,dj,fno);
}

void *http_f_readdir(struct vfscan *fs, int set_mode, int arg)
{
    return fselect(fs, set_mode, arg);
}

void http_f_closedir(struct vfscan *fs)
{
    fscan_release(fs);
    //return mutex_f_closedir(dj);
}

/********************************************************************************************************************/


struct virtual_file {
    const char *fname;
    const char *contents;
    unsigned int len;
    struct virtual_file *next;
} *virfiles_head = NULL;

typedef struct {
    unsigned int pos;
    const char *contents;
    unsigned int len;
} http_virfile;

int http_virfile_reg(const char *path, const char *contents, unsigned long len)
{
    struct virtual_file *entry;

    entry = malloc(sizeof(struct virtual_file));
    if (entry == NULL) {
        return -1;
    }

    entry->len = len;
    entry->contents = contents;
    entry->fname = path;
    entry->next = virfiles_head;
    virfiles_head = entry;

    return 0;
}

static http_virfile *http_virfile_open(const char *filename, const char *mode)
{
    struct virtual_file *virtfile = virfiles_head;
    http_virfile *file = NULL;

    while (virtfile != NULL) {
        if (strcmp(filename, virtfile->fname) == 0) {
            file = malloc(sizeof(http_virfile));
            if (file == NULL) {
                puts("http_virfile_open malloc fail!\n");
                goto out;
            }
            file->pos = 0;
            file->len = virtfile->len;
            file->contents = virtfile->contents;
            goto out;
        }
        virtfile = virtfile->next;
    }

out:
    return file;
}

static void http_virfile_close(http_virfile *file)
{
    free(file);
}

static int http_virfile_flen(http_virfile *file)
{
    return file->len;
}

static int http_virfile_read(http_virfile *file, char *buf, unsigned int buflen)
{
    int len;

    len = (buflen < file->len - file->pos) ? buflen : file->len - file->pos;

    memcpy(buf, file->contents + file->pos, len);

    file->pos += len;

    return len;
}

static int http_virfile_write(http_virfile *file, char *buf, unsigned int buflen)
{
    return -1;
}

static int http_virfile_seek(http_virfile *file, int offset, int origin)
{
    int newpos = -1;

    switch (origin) {
    case SEEK_SET:
        newpos = offset;
        break;
    case SEEK_CUR:
        newpos = file->pos + offset;
        break;
    case SEEK_END:
        newpos = file->len + offset;
        break;
    default:
        break;
    }

    if (newpos < 0 || ((unsigned int)newpos > file->len)) {
        puts("http_virfile_seek failed!\n");
        return -1;
    }

    file->pos = newpos;

    return 0;
}

/********************************************************************************************************************/


http_file_hdl *http_fopen(const char *path, const char *mode)
{
    http_file_hdl *file_hdl = malloc(sizeof(http_file_hdl));

    if (file_hdl == NULL) {
        return NULL;
    }

    file_hdl->file = http_virfile_open(path, mode);
    if (file_hdl->file) {
        puts("http_fopen virfile.\n");
        file_hdl->type = VIR_FILE;
        return file_hdl;
    }

    file_hdl->file = http_flashfile_open(path, mode);
    if (file_hdl->file) {
        /* puts("http_fopen flashfile.\n"); */
        file_hdl->type = FLASH_FILE;
        return file_hdl;
    }

    file_hdl->file = http_sdcardfile_open(path, mode);
    if (file_hdl->file) {
        /* puts("http_fopen sdcardfile.\n"); */
        file_hdl->type = SD_CARD_FILE;
        return file_hdl;
    }

    printf("http_fopen fail->%s.\n", path);
    free(file_hdl);

    return NULL;
}

void http_fclose(http_file_hdl *file_hdl)
{
    if (file_hdl == NULL) {
        return;
    }

    if (file_hdl->type == VIR_FILE) {
        http_virfile_close(file_hdl->file);
    } else if (file_hdl->type == FLASH_FILE) {
        http_flashfile_close(file_hdl->file);
    } else if (file_hdl->type == SD_CARD_FILE) {
        http_sdcardfile_close(file_hdl->file);
    }
    free(file_hdl);
}

int http_fread(void *buffer, size_t size, size_t count, http_file_hdl *file_hdl)
{
    int ret = -1;

    if (file_hdl->type == VIR_FILE) {
        ret = http_virfile_read(file_hdl->file, buffer, size * count);
    } else if (file_hdl->type == FLASH_FILE) {
        ret = http_flashfile_read(file_hdl->file, buffer, size * count);
    } else if (file_hdl->type == SD_CARD_FILE) {
        ret = http_sdcardfile_read(file_hdl->file, buffer, size * count);
    }

    return ret;
}


int http_flen(http_file_hdl *file_hdl)
{
    int ret = 0;

    if (file_hdl->type == VIR_FILE) {
        ret = http_virfile_flen(file_hdl->file);
    } else if (file_hdl->type == FLASH_FILE) {
        ret = http_flashfile_flen(file_hdl->file);
    } else if (file_hdl->type == SD_CARD_FILE) {
        ret = http_sdcardfile_flen(file_hdl->file);
    }
    return ret;
}



int http_fseek(http_file_hdl *file_hdl, int offset, int origin)
{
    int ret = 0;

    if (file_hdl->type == VIR_FILE) {
        ret = http_virfile_seek(file_hdl->file, offset, origin);
    } else if (file_hdl->type == FLASH_FILE) {
        ret = http_flashfile_seek(file_hdl->file, offset, origin);
    } else if (file_hdl->type == SD_CARD_FILE) {
        ret = http_sdcardfile_seek(file_hdl->file, offset, origin);
    }
    return ret;
}

