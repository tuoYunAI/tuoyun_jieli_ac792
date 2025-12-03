@echo off

@echo ********************************************************************************
@echo 			SDK WL83			
@echo ********************************************************************************
@echo %date%

cd %~dp0


.\isd_download.exe -make-diff -old-bin .\res1.bin -new-bin .\res1-new.bin -diff-block-size 0x100000 -compress-block-size 0x100000 -output update-res1-diff.bin

pause
