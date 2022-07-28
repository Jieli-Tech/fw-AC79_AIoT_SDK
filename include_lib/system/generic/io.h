#ifndef __GENERIC_IO_H__
#define __GENERIC_IO_H__


#define readb(a) 		(*((volatile unsigned char *)(a)))
#define readw(a) 		(*((volatile unsigned short *)(a)))
#define readl(a) 		(*((volatile unsigned int *)(a)))


#define writeb(a, v) 	(*((volatile unsigned char *)(a))) = (unsigned char)(v)
#define writew(a, v) 	(*((volatile unsigned short *)(a))) =(unsigned short)(v)
#define writel(a, v) 	(*((volatile unsigned int *)(a))) = (unsigned int)(v)


#endif

