#include "app_config.h"
// *INDENT-OFF*
#ifdef __SHELL__

${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg -mcpu=r3 $1.elf > ../../../$1.lst
${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt


/opt/utils/strip-ini -i isd_config.ini -o isd_config.ini

objsizedump -lite -skip-zero sdk.elf | sort -k 1 > ../../../map.lit

export PROJ_BUILD=download.bat

echo "@REM 加一下中文注释,防止服务器导出不了报错  ERROR: invalid file type to transfer   tools/download.bat 3 2 " >> ${PROJ_BUILD}
echo "@REM 加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释" >> ${PROJ_BUILD}

echo "set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe" >> ${PROJ_BUILD}
echo "set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe" >> ${PROJ_BUILD}
echo "set ELFFILE=sdk.elf" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .text %ELFFILE% text.bin" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .data %ELFFILE% data.bin" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .ram0_data  %ELFFILE% ram0_data.bin" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .cache_ram_data  %ELFFILE% cache_ram_data.bin" >> ${PROJ_BUILD}
echo "%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%" >> ${PROJ_BUILD}
echo "%OBJDUMP% -t %ELFFILE% > symbol_tbl.txt" >> ${PROJ_BUILD}
echo "copy /b text.bin+data.bin+ram0_data.bin+cache_ram_data.bin app.bin" >> ${PROJ_BUILD}
#if defined CONFIG_UI_ENABLE
echo "set UI_RES=ui_res" >> ${PROJ_BUILD}
#endif
#if defined CONFIG_VOICE_PROMPT_FILE_PATH
echo "set AUDIO_RES=audlogo" >> ${PROJ_BUILD}
#endif
#if defined CONFIG_SFC_ENABLE
echo "isd_download.exe isd_config.ini -tonorflash -dev wl82 -boot 0x1c02000 -div1 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin -res %AUDIO_RES% %UI_RES% cfg -reboot 500 -update_files normal" >> ${PROJ_BUILD}
#else
echo "set run_addr=0x2000" >> ${PROJ_BUILD}
echo "set load_addr=0x4000" >> ${PROJ_BUILD}
echo "set mask_addr=0x100000" >> ${PROJ_BUILD}
echo "uboot_lz4.exe app.bin app.lz4 %run_addr% %load_addr% rom.image %mask_addr%" >> ${PROJ_BUILD}
echo "isd_download.exe isd_config.ini -tonorflash -dev wl82 -boot 0x1c02000 -div1 -wait 300 -uboot uboot.boot -app app.lz4 cfg_tool.bin -res %AUDIO_RES% %UI_RES% cfg -reboot 500 -update_files normal" >> ${PROJ_BUILD}
#endif


#if 0
echo "if exist *.mp3 del *.mp3" >> ${PROJ_BUILD}
echo "if exist *.PIX del *.PIX" >> ${PROJ_BUILD}
echo "if exist *.TAB del *.TAB" >> ${PROJ_BUILD}
echo "if exist *.res del *.res" >> ${PROJ_BUILD}
echo "if exist *.sty del *.sty" >> ${PROJ_BUILD}
#endif

#if !(CONFIG_DOUBLE_BANK_ENABLE)
echo "fw_add.exe -noenc -fw jl_isd.fw  -add ota.bin -type 100 -out jl_isd.fw" >> ${PROJ_BUILD}
#endif
echo "fw_add.exe -noenc -fw jl_isd.fw -add script.ver -out jl_isd.fw" >> ${PROJ_BUILD}
echo "ufw_maker.exe -fw_to_ufw jl_isd.fw" >> ${PROJ_BUILD}
echo "copy jl_isd.ufw update.ufw" >> ${PROJ_BUILD}
echo "ping /n 2 127.1>null" >> ${PROJ_BUILD}
//echo "IF EXIST null del null" >> ${PROJ_BUILD}
echo "del cache_ram_data.bin" >> ${PROJ_BUILD}
echo "del data.bin" >> ${PROJ_BUILD}
echo "del ram0_data.bin" >> ${PROJ_BUILD}
echo "del text.bin" >> ${PROJ_BUILD}
echo "exit /b 0" >> ${PROJ_BUILD}

files="isd_config.ini download.bat"

host-client -project ${NICKNAME}$2 -f ${files} $1.elf

rm download.bat
rm isd_config.ini


#else





REM master分支只用于测试/研发/尝试最新特性,量产用途务必使用release分支,如果你明白自己在干什么,请删除 cpu/wl82/tools/download.c 的本行和下一行
exit /b -1
REM  再次声明 master分支只用于测试/研发/尝试最新特性,量产用途务必使用release分支,如果你明白自己在干什么,请删除 cpu/wl82/tools/download.c 的本行和下一行
exit /b -1






@echo off

@echo *********************************************************************
@echo                           AC791N SDK
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
%OBJCOPY% -O binary -j .ram0_data  %ELFFILE% ram0_data.bin
%OBJCOPY% -O binary -j .cache_ram_data  %ELFFILE% cache_ram_data.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
%OBJDUMP% -t %ELFFILE% > symbol_tbl.txt

copy /b text.bin+data.bin+ram0_data.bin+cache_ram_data.bin app.bin

REM set KEY_FILE=-key JL_791N-XXXX.key
REM set KEY_FILE=-key1 JL_791N-XXXX.key1 -mkey JL_791N-XXXX.mkey

#if defined CONFIG_UI_ENABLE
set UI_RES=ui_res
#endif

#if defined CONFIG_VOICE_PROMPT_FILE_PATH
set AUDIO_RES=audlogo
#endif

set CFG_FILE=cfg

REM  添加新参数-update_files说明
REM  1/normal就是普通的文件目录结构
REM  2/embedded就是每个文件数据前都会有一个头

REM  只生成code + res升级文件
REM  -update_files normal
REM  生成的文件名字为：db_update_files_data.bin

REM  只生成预留区资源升级文件
REM  -update_files embedded_only $(files) ,其中$(files)为需要添加的资源文件
REM  生成的文件名字为：db_update_files_data.bin

REM  生成code + res + 预留区资源升级文件
REM  -update_files embedded $(files) ,其中$(files)为需要添加的资源文件
REM  生成的文件名字为：db_update_files_data.bin

#if !defined CONFIG_SFC_ENABLE

REM @@@@@@@无效参数
set run_addr=0x2000
set load_addr=0x4000
set mask_addr=0x100000
uboot_lz4.exe app.bin app.lz4 %run_addr% %load_addr% rom.image %mask_addr%
REM @@@@@@@@@@@@@@@

isd_download.exe isd_config.ini -tonorflash -dev wl82 -boot 0x1c02000 -div1 -wait 300 -uboot uboot.boot -app app.lz4 cfg_tool.bin -res %AUDIO_RES% %UI_RES% %CFG_FILE% -reboot 500 %KEY_FILE% -update_files normal

#else

isd_download.exe isd_config.ini -tonorflash -dev wl82 -boot 0x1c02000 -div1 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin -res %AUDIO_RES% %UI_RES% %CFG_FILE% -reboot 500 %KEY_FILE% -update_files normal

#endif


@REM 常用命令说明
@rem -format vm         // 擦除VM 区域
@rem -format all        // 擦除所有
@rem -reboot 500        // reset chip, valid in JTAG debug

echo %errorlevel%

@REM 删除临时文件
if exist *.mp3 del *.mp3
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty

#if !(CONFIG_DOUBLE_BANK_ENABLE)
@REM 生成固件升级文件
fw_add.exe -noenc -fw jl_isd.fw  -add ota.bin -type 100 -out jl_isd.fw
#endif

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

#endif
// *INDENT-ON*
