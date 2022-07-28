@echo off

@echo *********************************************************************
@echo 			    AC790N SDK SD/U盘升级文件生成
@echo *********************************************************************
@echo %date%

::先执行download.bat生成文件
call .\download.bat

::再复制文件
copy jl_isd.ufw update.ufw

echo.
echo.
echo 升级文件名称：update.ufw，将update.ufw拷贝到SD卡/U盘的根目录，插卡上电即可升级（2秒钟后自动关闭窗口）
echo.
choice /t 2 /d y /n >nul 

::pause
exit
