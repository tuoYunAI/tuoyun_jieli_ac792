@REM 解析指定的flash_dlog.bin文件,并将log输出到终端
dlog.exe --flash flash_dlog.bin -i ..\dlog.bin --verbose
@REM 解析指定的flash_dlog.bin文件,并将log输出到指定文件
::dlog.exe --flash flash_dlog.bin -i ..\dlog.bin -o output_log.txt --verbose

pause