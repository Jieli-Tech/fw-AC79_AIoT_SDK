static u32 prd_1us = 0;
static u32 send_cnt = 0;
static u32 send_val = 0;
___interrupt
void timer5_isr(void)
{
    JL_TIMER5->CON |=  BIT(14);
    send_cnt ++;
    if (send_cnt % 2) {
        return;
    }
    u32 cnt = send_cnt / 2;
    if (cnt == 1) {
        JL_TIMER5->PRD = prd_1us * 5060;
        JL_TIMER5->PWM = prd_1us * 4500;
        JL_TIMER5->CNT = JL_TIMER5->PRD;
    } else if (cnt == 34) {
        JL_TIMER5->CON &= ~BIT(0);
        send_cnt = 0;
        return;
    } else if (send_val & BIT(33 - cnt)) {
        JL_TIMER5->PRD = prd_1us * 2240;
        JL_TIMER5->PWM = prd_1us * 1680;
        JL_TIMER5->CNT = JL_TIMER5->PRD;
    } else {
        JL_TIMER5->PRD = prd_1us * 1120;
        JL_TIMER5->PWM = prd_1us * 560;
        JL_TIMER5->CNT = JL_TIMER5->PRD;
    }
}

void timer5_send_ir_init(u32 port)
{
    u32 src_clk = 12000000;
    JL_IOMAP->CON0 |= BIT(21);//timer的io输入选择内部pll_12m,不是所有的timer的io输入都可选pll,修改请看文档
    JL_TIMER5->CON = 0;
    JL_TIMER5->CON |= (0b01 << 2);						//时钟源选择timer io
    JL_TIMER5->CON |= (0b0001 << 4);					//时钟源4分频
    JL_TIMER5->CON |= BIT(9);						    //波形反向
    prd_1us = (src_clk / (4 * 1000000));                //1us的计数值
    request_irq(IRQ_TIME5_IDX, 5, timer5_isr, 0);
    if ((IO_PORTB_07 == port) || (IO_PORTA_03 == port)) {//硬件io，br25的timer5有两个硬件IO口
        gpio_set_die(port, 1);
        gpio_set_pull_up(port, 0);
        gpio_set_pull_down(port, 0);
        gpio_set_direction(port, 0);
        JL_TIMER5->CON |= BIT(8);						//PWM使能
    } else {
        gpio_output_channle(port, CH2_T5_PWM_OUT);      //任意IO使用outputchannel
    }
}

void timer5_send_ir_data(u32 ir_data)
{
    send_val = ir_data;
    send_cnt = 0;
    JL_TIMER5->PRD = prd_1us * 9560;	                //设置周期 = 1us * 9605 = 9605us
    JL_TIMER5->PWM = prd_1us * 560;                     //设置高电平时间
    JL_TIMER5->CNT = JL_TIMER5->PRD;					//计数值先给满，起一次pend后，JL_TIMER->PWM寄存器才有效
    JL_TIMER5->CON |= BIT(0);                           //使能timer模块
}

void timer5_send_ir_test(void)
{
    /* timer5_send_ir_init(IO_PORTB_07); */
    timer5_send_ir_init(IO_PORTA_02);
    extern void wdt_clr();
    while (1) {
        wdt_clr();
        timer5_send_ir_data(0x6699aa55);
        os_time_dly(100);
    }

}


