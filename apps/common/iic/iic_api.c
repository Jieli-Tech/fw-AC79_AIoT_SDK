/*--------------------------------------------------------------------------*/
/**@file     iic_api.c
   @brief    IIC_API模块
   @details
   @author
   @date   2011-3-7
   @note
*/
/*----------------------------------------------------------------------------*/

#include "asm/iic_io.h"

u32 iic_busy = 0; ///<iic繁忙标记
/*----------------------------------------------------------------------------*/
/**@brief   IIC写函数
   @param   chip_id ：目标IC的ID号
   @param   iic_addr: 目标IC的目标寄存器的地址
   @param   *iic_dat: 写望目标IC的数据的指针
   @param   n:需要写的数据的数目
   @return  无
   @note    void  iic_write(u8 chip_id,u8 iic_addr,u8 *iic_dat,u8 n)
*/
/*----------------------------------------------------------------------------*/
void  iic_writen(u8 chip_id, u8 iic_addr, u8 *iic_dat, u8 n)
{
    iic_busy  = 1;
    iic_start();                //I2C启动
    iic_sendbyte(chip_id);         //写命令
    if (0xff != iic_addr) {
        iic_sendbyte(iic_addr);   //写地址
    }
    for (; n > 0; n--) {
        iic_sendbyte(*iic_dat++);      //写数据
    }
    iic_stop();                 //I2C停止时序
    iic_busy = 0;
}


void  iic_write(u8 chip_id, u8 iic_addr, u8 dat)
{
    iic_busy  = 1;
    iic_start();                //I2C启动
    iic_sendbyte(chip_id);         //写命令
    //if (0xff != iic_addr)
    {
        iic_sendbyte(iic_addr);   //写地址
    }
    //for (;n>0;n--)
    {
        iic_sendbyte(dat);      //写数据
    }
    delay(1000);//76us 至少延时64us
    iic_stop();                 //I2C停止时序
    iic_busy = 0;
}
/*----------------------------------------------------------------------------*/
/**@brief   IIC总线向一个目标ID读取几个数据
   @param   address : 目标ID
   @param   *p     :  存档读取到的数据的buffer指针
   @param   number :  需要读取的数据的个数
   @return  无
   @note    void i2c_read_nbyte(u8 address,u8 *p,u8 number)
*/
/*----------------------------------------------------------------------------*/
void iic_readn(u8 chip_id, u8 iic_addr, u8 *iic_dat, u16 n)
{
    iic_busy = 1;
    iic_start();                //I2C启动
    iic_sendbyte(chip_id);         //写命令
    //if (0xff != iic_addr)
    {
        iic_sendbyte(iic_addr);   //写地址
    }
    iic_start();
    iic_sendbyte(chip_id | BIT(0));
    for (; n > 1; n--) {
        *iic_dat++ = iic_revbyte(0);//读数据
        delay(1000);//76us 至少延时64us
    }
    *iic_dat++ = iic_revbyte(1);
    delay(1000);//76us 至少延时64us
    iic_stop();                 //I2C停止时序
    iic_busy = 0;
}

u8 iic_read(u8 chip_id, u8 iic_addr)
{
    u8  byte;

    iic_busy = 1;
    iic_start();                    //I2C启动
    iic_sendbyte(chip_id);             //写命令
    iic_sendbyte(iic_addr);       //写地址
    iic_start();                    //写转为读命令，需要再次启动I2C
    iic_sendbyte(chip_id | BIT(0));           //读命令
    byte = iic_revbyte(1);
    delay(1000);//76us 至少延时64us
    iic_stop();                     //I2C停止
    iic_busy = 0;
    return  byte;
}

