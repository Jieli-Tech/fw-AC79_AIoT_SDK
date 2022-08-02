REM master分支只用于测试/研发/尝试最新特性,量产用途务必使用release分支,如果你明白自己在干什么,请删除 cpu/wl82/tools/download.c 的本行和下一行
exit /b -1
REM 再次声明 master分支只用于测试/研发/尝试最新特性,量产用途务必使用release分支,如果你明白自己在干什么,请删除 cpu/wl82/tools/download.c 的本行和下一行
exit /b -1






@echo off

@echo *********************************************************************
@echo AC791N SDK
@echo *********************************************************************
@echo %date%

cd /d %~dp0

echo %*

set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf

REM %OBJDUMP% -D -address-mask=0x1ffffff -print-dbg %ELFFILE% > sdk.lst
%OBJCOPY% -O binary -j .text %ELFFILE% text.bin
%OBJCOPY% -O binary -j .data %ELFFILE% data.bin
%OBJCOPY% -O binary -j .ram0_data %ELFFILE% ram0_data.bin
%OBJCOPY% -O binary -j .cache_ram_data %ELFFILE% cache_ram_data.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
%OBJDUMP% -t %ELFFILE% > symbol_tbl.txt

copy /b text.bin+data.bin+ram0_data.bin+cache_ram_data.bin app.bin

REM set KEY_FILE=-key JL_791N-XXXX.key
REM set KEY_FILE=-key1 JL_791N-XXXX.key1 -mkey JL_791N-XXXX.mkey






set AUDIO_RES=audlogo


set CFG_FILE=cfg

REM 添加新参数-update_files说明
REM 1/normal就是普通的文件目录结构
REM 2/embedded就是每个文件数据前都会有一个头

REM 只生成code + res升级文件
REM -update_files normal
REM 生成的文件名字为：db_update_files_data.bin

REM 只生成预留区资源升级文件
REM -update_files embedded_only $(files) ,其中$(files)为需要添加的资源文件
REM 生成的文件名字为：db_update_files_data.bin

REM 生成code + res + 预留区资源升级文件
REM -update_files embedded $(files) ,其中$(files)为需要添加的资源文件
REM 生成的文件名字为：db_update_files_data.bin
isd_download.exe isd_config.ini -tonorflash -dev wl82 -boot 0x1c02000 -div1 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin -res %AUDIO_RES% %UI_RES% %CFG_FILE% -reboot 500 %KEY_FILE% -update_files normal




@REM 常用命令说明
@rem -format vm
@rem -format all
@rem -reboot 500

echo %errorlevel%

@REM 删除临时文件
if exist *.mp3 del *.mp3
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty






@REM 添加配置脚本的版本信息到 FW 文件中
fw_add.exe -noenc -fw jl_isd.fw -add script.ver -out jl_isd.fw

ufw_maker.exe -fw_to_ufw jl_isd.fw
copy jl_isd.ufw update.ufw


@REM 生成配置文件升级文件
rem ufw_maker.exe -chip AC630N %ADD_KEY% -output config.ufw -res bt_cfg.cfg

ping /n 2 127.1>null
IF EXIST null del null


::del app.bin
del cache_ram_data.bin
del data.bin
del ram0_data.bin
del text.bin

::退出当前批处理加返回值(0),才能多个批处理嵌套调用批处理
exit /b 0
