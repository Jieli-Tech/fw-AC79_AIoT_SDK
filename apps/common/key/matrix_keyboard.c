#include "key/key_driver.h"
#include "device/gpio.h"
#include "system/timer.h"
#include "app_config.h"

#if TCFG_MATRIX_KEY_ENABLE

#include "key/matrix_keyboard.h"
#include "asm/power_interface.h"
#include "asm/power/p33.h"

#define MATRIX_NO_KEY           0x1         //没有按键时的IO电平
#define MATRIX_LONG_TIME        35
#define MATRIX_HOLD_TIME        35+10
#define MATRIX_KEY_FILTER_TIME  2

int (*matrix_key_data_send)(u8 report_id, u8 *data, u16 len) = NULL;

static volatile u8 is_key_active = 0;
static volatile u8 keep_wakeup_hold = 0;
static int matrix_key_wakeup_hold_timer = 0;
static int matrix_key_timer = 0;
static matrix_key_st key_st[ROW_MAX][COL_MAX];
static matrix_key_param *this = NULL;

u8 notify_key_array[8] = {0};
u8 key_map[COL_MAX] = {0};
static u16 key_status_array[6] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};


static void full_key_array(u8 row, u8 col, u8 st)
{
    u8 offset = 0;
    u8 mark = 0, mark_offset = 0;
    //最多推6个按键出来，如果需要推多个按键需要自行修改，每个u16 低八位标识row 高八位标识col
    u16 key_value = (row | col << 8);
    for (offset = 0; offset < sizeof(key_status_array) / sizeof(u16); offset++) {
        if (key_status_array[offset] == key_value && st == MATRIX_KEY_UP) {         //找到列表中有当前按键且按键状态抬起,则将按键移除列表
            mark = 1;                                                               //记录相同键值所在位置
            mark_offset = offset;
            if (mark_offset == sizeof(key_status_array) / sizeof(u16) - 1) {
                key_status_array[mark_offset] = 0xffff;
                break;
            }
        } else if (key_status_array[offset] == 0xffff) {
            if (mark) {
                memcpy(key_status_array + mark_offset, key_status_array + mark_offset + 1, (offset - mark_offset - 1) * 2);
                key_status_array[offset - 1] = 0xffff;
            } else if (st == MATRIX_KEY_SHORT) {                                    //需要状态为短按才会把键值填充到数组中，防止按键满了之后把抬起也填充进去
                key_status_array[offset] = key_value;
            }
            break;
        } else if (mark && (offset == sizeof(key_status_array) / sizeof(u16) - 1)) {
            memcpy(key_status_array + mark_offset, key_status_array + mark_offset + 1, (sizeof(key_status_array) / sizeof(u16) - mark_offset - 1) * 2);
            key_status_array[sizeof(key_status_array) / sizeof(u16) - 1] = 0xffff;
            break;
        }
    }
}


void full_key_map(u8 row, u8 col, u8 st)
{
    if (st) {
        key_map[col] |= BIT(row);
    } else {
        key_map[col] &= ~BIT(row);
    }
}

void P33_AND_WKUP_EDGE(u8 data);
void P33_OR_WKUP_EDGE(u8 data);

enum {
    IO_STATUS_HIGH_DRIVER = 0,
    IO_STATUS_OUTPUT_HIGH,
    IO_STATUS_OUTPUT_LOW,
    IO_STATUS_INPUT_PULL_DOWN,
    IO_STATUS_INPUT_PULL_UP,
};

void matrix_key_set_io_state(u8 state, u32 *io_table, u8 len)
{
    u8 i = 0;
    u32 porta_value = 0;
    u32 portb_value = 0;
    u32 portc_value = 0;
    u32 portd_value = 0;

    for (; i < len; i++) {
        switch (io_table[i] / IO_GROUP_NUM) {
        case 0: //JL_PORTA
            porta_value |= BIT(io_table[i] % IO_GROUP_NUM);
            break;
        case 1: //JL_PORTB
            portb_value |= BIT(io_table[i] % IO_GROUP_NUM);
            break;
        case 2: //JL_PORTC
            portc_value |= BIT(io_table[i] % IO_GROUP_NUM);
            break;
        case 3: //JL_PORTD
            portd_value |= BIT(io_table[i] % IO_GROUP_NUM);
            break;
        default://USB

            break;
        }
    }
    switch (state) {
    case IO_STATUS_HIGH_DRIVER:
        JL_PORTA->DIR |= porta_value;
        JL_PORTA->DIE |= porta_value;
        JL_PORTA->PU &= ~porta_value;
        JL_PORTA->PD &= ~porta_value;
        JL_PORTB->DIR |= portb_value;
        JL_PORTB->DIE |= portb_value;
        JL_PORTB->PU &= ~portb_value;
        JL_PORTB->PD &= ~portb_value;
        JL_PORTC->DIR |= portc_value;
        JL_PORTC->DIE |= portc_value;
        JL_PORTC->PU &= ~portc_value;
        JL_PORTC->PD &= ~portc_value;
        JL_PORTD->DIR |= portd_value;
        JL_PORTD->DIE |= portd_value;
        JL_PORTD->PU &= ~portd_value;
        JL_PORTD->PD &= ~portd_value;
        break;
    case IO_STATUS_OUTPUT_HIGH:
        JL_PORTA->DIR &= ~porta_value;
        JL_PORTA->OUT |= porta_value;
        JL_PORTB->DIR &= ~portb_value;
        JL_PORTB->OUT |= portb_value;
        JL_PORTC->DIR &= ~portc_value;
        JL_PORTC->OUT |= portc_value;
        JL_PORTD->DIR &= ~portd_value;
        JL_PORTD->OUT |= portd_value;
        break;
    case IO_STATUS_OUTPUT_LOW:
        JL_PORTA->DIR &= ~porta_value;
        JL_PORTA->OUT &= ~porta_value;
        JL_PORTB->DIR &= ~portb_value;
        JL_PORTB->OUT &= ~portb_value;
        JL_PORTC->DIR &= ~portc_value;
        JL_PORTC->OUT &= ~portc_value;
        JL_PORTD->DIR &= ~portd_value;
        JL_PORTD->OUT &= ~portd_value;
        break;
    case IO_STATUS_INPUT_PULL_DOWN:
        JL_PORTA->DIR |= porta_value;
        JL_PORTA->DIE |= porta_value;
        JL_PORTA->PU  &= ~porta_value;
        JL_PORTA->PD  |= porta_value;
        JL_PORTB->DIR |= portb_value;
        JL_PORTB->DIE |= portb_value;
        JL_PORTB->PU  &= ~portb_value;
        JL_PORTB->PD  |= portb_value;
        JL_PORTC->DIR |= portc_value;
        JL_PORTC->DIE |= portc_value;
        JL_PORTC->PU  &= ~portc_value;
        JL_PORTC->PD  |= portc_value;
        JL_PORTD->DIR |= portd_value;
        JL_PORTD->DIE |= portd_value;
        JL_PORTD->PU  &= ~portd_value;
        JL_PORTD->PD  |= portd_value;
        break;
    case IO_STATUS_INPUT_PULL_UP:
        JL_PORTA->DIR |= porta_value;
        JL_PORTA->DIE |= porta_value;
        JL_PORTA->PU  |= porta_value;
        JL_PORTA->PD  &= ~porta_value;
        JL_PORTB->DIR |= portb_value;
        JL_PORTB->DIE |= portb_value;
        JL_PORTB->PU  |= portb_value;
        JL_PORTB->PD  &= ~portb_value;
        JL_PORTC->DIR |= portc_value;
        JL_PORTC->DIE |= portc_value;
        JL_PORTC->PU  |= portc_value;
        JL_PORTC->PD  &= ~portc_value;
        JL_PORTD->DIR |= portd_value;
        JL_PORTD->DIE |= portd_value;
        JL_PORTD->PU  |= portd_value;
        JL_PORTD->PD  &= ~portd_value;
        break;
    }
}

#if 1
void port_edge_wakeup_control(u16 data);
void matrix_key_scan(void)
{

    u8 row, col;
    u8 cur_key_value = MATRIX_NO_KEY, notify = 0;
    static u8 wk_toggle = 0;

    //g_printf("scan...\n");
    //P33_LEVEL_WKUP_EN(0);
    port_edge_wakeup_control(0);
    //p33_tx_1byte(P3_WKUP_EN, 0);        //关掉wakeup防止扫描过程一直唤醒

    //不直接调用gpio.c的接口主要是由于想控制扫描时间。这样能把时间从1.8ms缩减到800us
    matrix_key_set_io_state(IO_STATUS_HIGH_DRIVER, this->col_pin_list, this->col_num);

    is_key_active = 0;
    for (col = 0; col < this->col_num; col ++) {
        matrix_key_set_io_state((MATRIX_NO_KEY) ? IO_STATUS_OUTPUT_LOW : IO_STATUS_OUTPUT_HIGH, &(this->col_pin_list[col]), 1);
        //gpio_direction_output(this->col_pin_list[col], !MATRIX_NO_KEY);
        for (row = 0; row < this->row_num; row ++) {
            cur_key_value = gpio_read(this->row_pin_list[row]);
            if (cur_key_value != MATRIX_NO_KEY) {
                is_key_active = 1;
            }
            if (cur_key_value != (key_st[row][col]).last_st) {
                if (cur_key_value == MATRIX_NO_KEY) { //按键抬起判断
                    (key_st[row][col]).press_cnt = 0;
                    //printf("row:%d  col:%d   [UP]\n", row, col);
                    (MATRIX_NO_KEY) ? P33_OR_WKUP_EDGE(row) : P33_AND_WKUP_EDGE(row);
                    //P33_AND_WKUP_EDGE(row);
                    full_key_map(row, col, 0);
                    notify = 1;
                } else {            //按键初次按下
                    printf("row:%d  col:%d   [SHORT]\n", row, col);
                    full_key_map(row, col, 1);
                    (MATRIX_NO_KEY) ? P33_AND_WKUP_EDGE(row) : P33_OR_WKUP_EDGE(row);
                    //P33_OR_WKUP_EDGE(row);
                    notify = 1;
                }
            } else {        //判断是否按键抬起
                if (cur_key_value != MATRIX_NO_KEY) {
                    (key_st[row][col]).press_cnt ++;
                    if ((key_st[row][col]).press_cnt == MATRIX_LONG_TIME) {
                        /* printf("row:%d  col:%d   [LONG]\n", row, col);     */
                    } else if ((key_st[row][col]).press_cnt == MATRIX_HOLD_TIME) {
                        /* printf("row:%d  col:%d   [HOLD]\n", row, col);     */
                        (key_st[row][col]).press_cnt = MATRIX_LONG_TIME;
                    } else {
                        //press_cnt还未累加够
                    }
                }
            }
            (key_st[row][col]).last_st = cur_key_value;
        }
        //gpio_direction_input(this->col_pin_list[col]);
        matrix_key_set_io_state(IO_STATUS_HIGH_DRIVER, &(this->col_pin_list[col]), 1);
    }

    matrix_key_set_io_state((MATRIX_NO_KEY == 1) ? IO_STATUS_OUTPUT_LOW : IO_STATUS_OUTPUT_HIGH, this->col_pin_list, this->col_num);

    port_edge_wakeup_control(0xff);
    //p33_tx_1byte(P3_WKUP_EN, 0xff);
    //P33_LEVEL_WKUP_EN(1);

    JL_PORTC->OUT &=  ~(BIT(4));

    if (notify) {
        struct sys_event e;
        e.type = SYS_MATRIX_KEY_EVENT;
        e.u.matrix_key.map = key_map;
        e.arg  = (void *)DEVICE_EVENT_FROM_KEY;
        sys_event_notify(&e);
    }
}
#else

void matrix_key_scan(void)
{
    u8 row, col;
    u8 cur_key_value = MATRIX_NO_KEY, notify = 0;
    for (row = 0; row < this->row_num; row ++) {
        gpio_direction_input(this->row_pin_list[row]);
        gpio_set_die(this->row_pin_list[row], 1);
        gpio_set_pull_up(this->row_pin_list[row], 1);
        gpio_set_pull_down(this->row_pin_list[row], 0);
    }

    for (col = 0; col < this->col_num; col ++) {
        gpio_direction_output(this->col_pin_list[col], 0);
        gpio_set_die(this->col_pin_list[col], 1);
        for (row = 0; row < this->row_num; row ++) {
            if (gpio_read(this->row_pin_list[row]) == 0) {
                printf("col:%x  row:%x\n", col, row);
            }

        }
        gpio_direction_input(this->col_pin_list[col]);
        gpio_set_pull_up(this->col_pin_list[col], 0);
        gpio_set_pull_up(this->col_pin_list[col], 0);
        gpio_set_die(this->col_pin_list[col], 0);
    }
}
#endif

void matrix_key_wakeup_timeout_handler(void *arg)
{
    is_key_active = 1;
}

void matrix_key_wakeup_keep(void)
{
    is_key_active = 1;
    if (matrix_key_wakeup_hold_timer) {
        sys_s_hi_timeout_del(matrix_key_wakeup_hold_timer);
    }
    matrix_key_wakeup_hold_timer = sys_s_hi_timerout_add(NULL, matrix_key_wakeup_timeout_handler, 10);
}

void matrix_key_wakeup(u8 idx, u32 gpio)
{
    r_printf("matrix_key wkup!\n");
    matrix_key_wakeup_keep();
}

int matrix_key_init(matrix_key_param *param)
{
    if (!param) {
        return -1;      //参数无效
    }
    this = param;
    u8 row, col;
    for (row = 0; row < this->row_num; row ++) {
        (MATRIX_NO_KEY) ? P33_OR_WKUP_EDGE(row) : P33_AND_WKUP_EDGE(row);
        for (col = 0; col < this->col_num; col ++) {
            (key_st[row][col]).last_st = MATRIX_NO_KEY;
            (key_st[row][col]).press_cnt = 0;
        }
    }
    printf("key_row:%d rol:%d\n", this->row_num, this->col_num);
    if (matrix_key_timer) {
        sys_s_hi_timer_del(matrix_key_timer);
    }

    matrix_key_set_io_state((MATRIX_NO_KEY) ? IO_STATUS_OUTPUT_LOW : IO_STATUS_OUTPUT_HIGH, this->col_pin_list, this->col_num);
    matrix_key_set_io_state((MATRIX_NO_KEY) ? IO_STATUS_INPUT_PULL_UP : IO_STATUS_INPUT_PULL_DOWN, this->row_pin_list, this->row_num);


    port_edge_wkup_set_callback(matrix_key_wakeup);
    matrix_key_timer = sys_s_hi_timer_add(NULL, matrix_key_scan, 10);
    return 0;
}



static u8 matrix_key_idle_query(void)
{
    return !is_key_active;
}

REGISTER_LP_TARGET(matrix_key_lp_target) = {
    .name = "matrix_key",
    .is_idle = matrix_key_idle_query,
};

#endif
