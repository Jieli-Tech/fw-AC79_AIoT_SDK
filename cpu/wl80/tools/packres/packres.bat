REM packres.exe -n $(dir) -o $(output) $(file1) $(file2) ...
REM 其中$(dir)为打包后的文件目录入口，$(output)打包后的输出文件，$(file1) $(file2) ...为需要打包的输入文件
REM 放置在res资源区时，其搜索路径为mnt/sdfile/res/$(output)/$(dir)/$(file)，如：mnt/sdfile/res/tone/res1.txt
REM 放置在[RESERVED_CONFIG]预留区时，其搜索路径为：mnt/sdfile/app/$(output)/$(dir)/$(file),如：mnt/sdfile/app/update/tone/res1.txt
REM 放置在[RESERVED_EXPAND_CONFIG]预留区时，其搜索路径为：mnt/sdfile/EXT_RESERVED/$(output)/$(dir)/$(file),如：mnt/sdfile/EXT_RESERVED/update/tone/res1.txt
packres.exe -n tone -o UPDATE res1.txt res2.txt res3.txt
::pause
