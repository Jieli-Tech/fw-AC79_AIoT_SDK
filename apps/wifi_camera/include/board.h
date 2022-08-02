#ifndef BOARD_H
#define BOARD_H


/*-------------avin det and avin parking det----- */
extern void av_parking_det_init();
extern unsigned char av_parking_det_status();
extern unsigned char PWR_CTL(unsigned char on_off);
extern unsigned char read_power_key();
extern unsigned char usb_is_charging();
extern void key_voice_enable(int enable);











#endif
