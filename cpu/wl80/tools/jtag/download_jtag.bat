::copy /b sdram.bin+sdram_text.bin sdram.bin

dv10_app_make.exe -addr 0x4000000 -infile sdram.bin  -compression
::apu_make.exe -addr 0x4000000 -infile sdram.bin  -ofile sdram.apu

dv15_isd_sdr.exe isd_tools_flash-new.cfg -f uboot.boot ui.apu sdram.apu -resource res audlogo 32 -tonorflash -dev jielijtag -boot 0x3f02000 -div2 -runaddr 0x04000000  -aline 4096 -bfumode3 -reboot -erase

pause



