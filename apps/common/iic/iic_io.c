/*--------------------------------------------------------------------------*/
/**@file     iic_io.c
   @brief    IO模拟的IIC的驱动
   @details
   @author
   @date   2011-3-7
   @note
*/
/*----------------------------------------------------------------------------*/
#include "asm/iic_io.h"

/*----------------------------------------------------------------------------*/
/**@brief   IIC的IO口初始化
   @param   无
   @return  无
   @note    void iic_init_io(void)
*/
/*----------------------------------------------------------------------------*/
void iic_init_io(void)
{
    iic_data_out();         //SDA设置成输出
    iic_clk_out();         	//SCL设置成输出
    iic_clk_h();
    iic_delay();
    iic_data_h();
    iic_delay();
}

u16 iic_data_r()
{
    iic_data_in();
    iic_delay();
    return iic_data_read();//PORTA_IN&BIT(15);
}


/*----------------------------------------------------------------------------*/
/**@brief   读取ACK
   @param   无
   @return  都会的ACK/NACK的电平
   @note    bool r_ack(void)
*/
/*----------------------------------------------------------------------------*/
void iic_start(void)
{
    //iic_init_io();
    iic_data_h();
    iic_delay();
    iic_clk_h();
    iic_delay();
    iic_data_l();
    iic_delay();
    iic_clk_l();
    iic_delay();
    //iic_data_h();
}

/*----------------------------------------------------------------------------*/
/**@brief   STOP IIC
   @param   无
   @return  无
   @note    void iic_stop(void)
*/
/*----------------------------------------------------------------------------*/
void iic_stop(void)
{
    iic_data_out();
    iic_data_l();
    iic_delay();
    iic_clk_h();
    iic_delay();
    iic_data_h();
    iic_delay();
    //iic_clk_l();  //IIC终止信号，时钟线应该置低，表示本时钟信号结束
}

/*----------------------------------------------------------------------------*/
/**@brief   读取ACK
   @param   无
   @return  都会的ACK/NACK的电平
   @note    bool r_ack(void)
*/
/*----------------------------------------------------------------------------*/
u8 r_ack(void)
{
    u8 nack;
    iic_data_in();
    iic_delay();
    iic_clk_h();
    iic_delay();
    nack = iic_data_r();
    iic_clk_l();
    iic_delay();
    return nack;
}

/*----------------------------------------------------------------------------*/
/**@brief   发送一个ACK信号的数据,
   @param   flag ：发送的ACK/nack的类型
   @return  无
   @note    void s_ack(u8 flag)
*/
/*----------------------------------------------------------------------------*/
void s_ack(u8 flag)
{
    iic_data_out();
    iic_clk_l();
    if (flag) {
        iic_data_h();
    } else {
        iic_data_l();
    }
    iic_delay();
    iic_clk_h();
    iic_delay();
    iic_clk_l();
}
/*----------------------------------------------------------------------------*/
/**@brief   从IIC总线接收一个BYTE的数据,
   @param   无
   @return  读取回的数据
   @note    u8 iic_revbyte_io( void )
*/
/*----------------------------------------------------------------------------*/
u8 iic_revbyte_io(void)
{
    u8 byteI2C = 0;
    u8 i;
    iic_data_in();
    iic_delay();
    for (i = 0; i < 8; i++) {
        iic_clk_h();
        byteI2C <<= 1;
        iic_delay();
        if (iic_data_r()) {
            byteI2C++;
        }
        iic_clk_l();
        iic_delay();
    }
    return byteI2C;
}
/*----------------------------------------------------------------------------*/
/**@brief   从IIC总线接收一个BYTE的数据,并发送一个指定的ACK
   @param   para ：发送ACK 还是 NACK
   @return  读取回的数据
   @note    u8 iic_revbyte( u8 para )
*/
/*----------------------------------------------------------------------------*/
u8 iic_revbyte(u8 para)
{
    u8 byte;
    byte = iic_revbyte_io();
    s_ack(para);
    return byte;
}
/*----------------------------------------------------------------------------*/
/**@brief   向IIC总线发送一个BYTE的数据
   @param   byte ：要写的EEROM的地址
   @return  无
   @note    void iic_sendbyte_io(u8 byte)
*/
/*----------------------------------------------------------------------------*/
void iic_sendbyte_io(u8 byte)
{
    u8 i;
    iic_data_out();
    iic_delay();
    for (i = 0; i < 8; i++) {
        if (byte & BIT(7)) {
            iic_data_h();    //最高位是否为1,为1则SDA= 1,否则 SDA=0
        } else {
            iic_data_l();
        }
        //iic_delay();
        iic_clk_h();
        iic_delay();
        byte <<= 1;                   //数据左移一位,进入下一轮送数
        iic_clk_l();
        iic_delay();
    }
}
/*----------------------------------------------------------------------------*/
/**@brief   向IIC总线发送一个BYTE的数据,并读取ACK
   @param   byte ：要写的EEROM的地址
   @return  无
   @note    void iic_sendbyte(u8 byte)
*/
/*----------------------------------------------------------------------------*/
void iic_sendbyte(u8 byte)
{
    iic_sendbyte_io(byte);
    if (r_ack())
        ;//putchar('n');
    else
        ;//putchar('r');
}



