#include "app_config.h"
#include "ui_api.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/pap.h"
#include "asm/emi.h"
#include "asm/spi.h"

#ifdef CONFIG_CPU_WL82
#include "asm/WL82_imd.h"
#endif

#include "lcd_drive.h"

#ifdef CONFIG_UI_ENABLE


/* 屏幕驱动的接口 */
static struct lcd_device *lcd_dev;
static const struct ui_lcd_platform_data *lcd_pdata = NULL;
static void *lcd_hdl = NULL;

// io口操作
void lcd_rst_pinstate(u8 state)
{
    if (lcd_pdata->rst_pin != (u32) - 1) {
        gpio_direction_output(lcd_pdata->rst_pin, state);
    }
}

void lcd_cs_pinstate(u8 state)
{
    if (lcd_pdata->cs_pin != (u32) - 1) {
        gpio_direction_output(lcd_pdata->cs_pin, state);
    }
}

void lcd_rs_pinstate(u8 state)
{
    if (lcd_pdata->rs_pin != (u32) - 1) {
        gpio_direction_output(lcd_pdata->rs_pin, state);
    }
}

void lcd_bl_pinstate(u8 state)
{
    if (lcd_pdata->bl_pin != (u32) - 1) {
        gpio_direction_output(lcd_pdata->bl_pin, state);
    }
}

void touch_int_pinstate(u8 state)
{
    if (lcd_pdata->touch_int_pin != (u32) - 1) {
        gpio_direction_output(lcd_pdata->touch_int_pin, state);
    }
}

void touch_reset_pinstate(u8 state)
{
    if (lcd_pdata->touch_reset_pin != (u32) - 1) {
        gpio_direction_output(lcd_pdata->touch_reset_pin, state);
    }
}

int lcd_send_byte(u8 data)
{
    if (lcd_hdl) {
        return dev_write(lcd_hdl, &data, 1);
    }

    return -1;
}

int lcd_send_map(u8 *map, u32 len)
{
    if (lcd_hdl) {
        return dev_write(lcd_hdl, map, len);
    }

    return -1;
}

int lcd_read_data(u8 *buf, u32 len)
{
    if (lcd_hdl) {
        return dev_read(lcd_hdl, buf, len);
    }

    return -1;
}
u8 *lcd_req_buf(u32 *size)
{
    if (lcd_pdata && lcd_pdata->lcd_if == LCD_IMD && lcd_hdl) {
#ifdef CONFIG_CPU_WL82
        u8 *buf = NULL;
        u32 buf_size = 0;
        dev_ioctl(lcd_hdl, IOCTL_IMD_GET_FRAME_BUFF, (u32)&buf);
        dev_ioctl(lcd_hdl, IOCTL_IMD_GET_FRAME_BUFF_SIZE, (u32)&buf_size);
        if (buf && buf_size) {
            *size = buf_size;
            return buf;
        }
#endif
    }
    *size = 0;
    return NULL;
}
void lcd_req_buf_update(u8 *buf, u32 *size)
{
    if (lcd_pdata && lcd_pdata->lcd_if == LCD_IMD && lcd_hdl) {
#ifdef CONFIG_CPU_WL82
        dev_ioctl(lcd_hdl, IOCTL_IMD_UPDATE_BUFF, (u32)buf);
#endif
    }
}

static int lcd_interface_init(void)
{
    void *priv = lcd_dev->lcd_priv;
    if (lcd_pdata->lcd_if == LCD_SPI) {
        lcd_hdl = dev_open(lcd_pdata->spi_id, NULL);
    } else if (lcd_pdata->lcd_if == LCD_IMD) {
        lcd_hdl = dev_open("imd", priv);
    } else if (lcd_pdata->lcd_if == LCD_PAP) {
        lcd_hdl = dev_open("pap", NULL);
    } else if (lcd_pdata->lcd_if == LCD_EMI) {
        lcd_hdl = dev_open("emi", NULL);
        if (lcd_hdl) {
            dev_ioctl(lcd_hdl, EMI_USE_SEND_SEM, 0);
        }
    }

    if (lcd_hdl == NULL) {
        lcd_e("open lcd interface failed.\n");
        return -1;
    }

    return 0;
}

static int lcd_init(void *p)
{
    if (!lcd_hdl) {
        struct ui_devices_cfg *cfg = (struct ui_devices_cfg *)p;
        u8 find_lcd = 0;
        lcd_pdata = (const struct ui_lcd_platform_data *)cfg->private_data;
        ASSERT(lcd_pdata, "Error! lcd io not config");

        list_for_each_lcd_driver(lcd_dev) {
            if (lcd_dev && lcd_dev->LCD_Init) {
                printf("LCD find %s\n", lcd_dev->name);
                find_lcd = true;
                break;
            }

        }
        if (!find_lcd) {
            printf("err no lcd_dev\n");
            return -1;
        }
        if (lcd_interface_init() != 0) {
            printf("err unknow lcd port \n\n\n");
            return -1;
        }
        if (lcd_dev && lcd_dev->LCD_Init) {
            return lcd_dev->LCD_Init();
        }
    }
    return 0;
}

void user_lcd_init(void)
{
    if (!lcd_hdl) {
        extern const struct ui_devices_cfg ui_cfg_data;
        lcd_init(&ui_cfg_data);//ui_cfg_data参数可以在版籍配置
    }
}

void user_ui_lcd_init(void)
{
#if TCFG_USE_SD_ADD_UI_FILE
    int storage_device_ready(void);
    while (!storage_device_ready()) {
        os_time_dly(1);
    }
#endif
    extern const struct ui_devices_cfg ui_cfg_data;
    int lcd_ui_init(void *arg);
    lcd_ui_init(&ui_cfg_data);
}
static void lcd_get_screen_info(struct lcd_info *info)
{
    if (!lcd_dev) {
        return ;
    }
    info->width        = lcd_dev->lcd_width;
    info->height       = lcd_dev->lcd_height;
    info->color_format = lcd_dev->color_format;
    info->col_align    = lcd_dev->column_addr_align;
    info->row_align    = lcd_dev->row_addr_align;
}

static void lcd_buffer_malloc(u8 **buf, u32 *size)
{
    if (!lcd_dev) {
        return ;
    }
    *buf = malloc(lcd_dev->lcd_width * lcd_dev->lcd_height * 2);
    if (NULL == *buf) {
        *size = 0;
        return;
    }
    *size = lcd_dev->lcd_width * lcd_dev->lcd_height * 2;
}

static void lcd_buffer_free(u8 *buf)
{
    free(buf);
}

static void ui_draw(u8 *buf, u32 len, u8 wait)
{
    if (!lcd_dev) {
        return ;
    }
    if (lcd_dev->LCD_Draw) {
        lcd_dev->LCD_Draw(buf, len);
    }
}
static void lcd_draw(u8 *buf, u32 len, u8 wait)
{
    if (!lcd_dev) {
        return ;
    }
    if (lcd_dev->LCD_Draw_1) {
        lcd_dev->LCD_Draw_1(buf, len);
    }
}
static void lcd_draw_to_dev(u8 *buf, u32 len)
{
    if (!lcd_dev) {
        return ;
    }
    if (lcd_dev->LCD_DrawToDev) {
        lcd_dev->LCD_DrawToDev(buf, len);
    }
}
void lcd_set_draw_area(u16 xs, u16 xe, u16 ys, u16 ye)
{
    if (!lcd_dev) {
        return ;
    }
    if (lcd_dev->SetDrawArea) {
        lcd_dev->SetDrawArea(xs, xe, ys, ye);
    }
}

static void lcd_clear_screen(u16 color)
{
    if (!lcd_dev) {
        return ;
    }
    if (lcd_dev->LCD_ClearScreen) {
        lcd_dev->LCD_ClearScreen(color);
    }
}

static int lcd_backlight_power(u8 on)
{
    static u8 first_power_on = true;
    if (first_power_on) {
        lcd_clear_screen(0x0000);
        first_power_on = false;
    }

    if (!lcd_dev) {
        return -EINVAL;
    }
    if (lcd_dev->BackLightCtrl) {
        lcd_dev->BackLightCtrl(on);
    }

    return 0;
}

int lcd_backlight_ctrl(u8 on)
{
    return 0;
}


int lcd_sleep_control(u8 enter)
{
    return 0;
}

int lcd_backlight_status()
{
    return 1;
}

void lcd_get_width_height(int *width, int *height)
{
    if (!lcd_dev) {
        *width = 0;
        *height = 0;
        return ;
    }
    *width = lcd_dev->lcd_width;
    *height = lcd_dev->lcd_height;
}

struct lcd_interface *lcd_get_hdl(void)
{
    struct lcd_interface *p;

    ASSERT(lcd_interface_begin != lcd_interface_end, "don't find lcd interface!");
    for (p = lcd_interface_begin; p < lcd_interface_end; p++) {
        return p;
    }
    return NULL;
}
int lcd_get_color_format_rgb24(void)
{
    if (!lcd_dev) {
        return 0;
    }
    return (lcd_dev->color_format == LCD_COLOR_RGB888);
}

void lcd_show_frame_to_dev(u8 *buf, u32 len)//该接口显示数据直接推送数据到LCD设备接口，不分数据格式，慎用！
{
    lcd_draw_to_dev(buf, len);
}
void ui_show_frame(u8 *buf, u32 len)//该接口主要用于显示UI数据  传入数据RGB
{
    ui_draw(buf, len, 0);
}
void lcd_show_frame(u8 *buf, u32 len)//该接口主要用于用户显示数据 //列如摄像头数据 视频数据 YUV
{
    lcd_draw(buf, len, 0);
}

REGISTER_LCD_INTERFACE(lcd) = {
    .init = lcd_init,
    .get_screen_info = lcd_get_screen_info,
    .buffer_malloc = lcd_buffer_malloc,
    .buffer_free = lcd_buffer_free,
    .draw = ui_draw,
    .draw_1 = lcd_draw,
    .set_draw_area = lcd_set_draw_area,
    .backlight_ctrl = lcd_backlight_power,
};

#endif

