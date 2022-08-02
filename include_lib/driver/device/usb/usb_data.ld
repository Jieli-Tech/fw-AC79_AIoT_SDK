
_usb_data_begin = .;
PROVIDE(usb_data_begin = .);
*(.usb_h_bss)
*(.usb_g_bss)
*(.usb_h_dma)
*(.usb_config_var)
*(.usb_msd_dma)
*(.usb_fifo)
*(.mass_storage)
*(.uac_var)
*(.uac_rx)
*(.usb_h_udisk)
*(.cdc_var)
_usb_data_end = .;
PROVIDE(usb_data_end = .);


