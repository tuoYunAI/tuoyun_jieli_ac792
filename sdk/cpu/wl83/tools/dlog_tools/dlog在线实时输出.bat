@REM -p 端口号
@REM -b 波特率
@REM -d 串口数据位(默认8bit)
@REM -r 串口校验位(默认无校验)
@REM -s 串口停止位(默认1位)
@REM -f 串口流控(默认无流控)
@REM -i 指定dlog.bin文件
@REM --verbose 输出该工具的调试信息
@REM --local_time 显示本地时间戳
dlog.exe -p COM69 -b 1000000 -i ..\dlog.bin --verbose --local_time

pause