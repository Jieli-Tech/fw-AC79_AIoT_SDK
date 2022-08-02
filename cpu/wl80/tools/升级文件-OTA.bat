@echo off

@echo *********************************************************************
@echo 			    AC790N SDK 网络升级文件生成
@echo *********************************************************************
@echo %date%

::先执行download.bat生成文件
call .\download.bat

::再复制文件
copy db_update_files_data.bin update-ota.ufw

echo.
echo.
echo 升级文件名称：update-ota.ufw，用在OTA网络升级即可（2秒钟后自动关闭窗口）
echo.
choice /t 2 /d y /n >nul 

::pause
exit