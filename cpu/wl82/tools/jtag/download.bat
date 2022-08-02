cd ..
copy isd_config.ini isd_config.ini_relase
del isd_config.ini app.bin
copy .\jtag\isd_config_debug.ini isd_config.ini
copy /B .\jtag\app.bin app.bin 
isd_download.exe -tonorflash -dev br22 -boot 0x2000 -div8 -wait 300 -uboot uboot.boot -app app.bin  -res cfg_tool.bin -tone tone.cfg -reboot 500

copy isd_config.ini_relase isd_config.ini 
del isd_config.ini_relase
