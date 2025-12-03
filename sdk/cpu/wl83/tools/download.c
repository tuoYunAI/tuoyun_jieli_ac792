
#include "app_config.h"

// *INDENT-OFF*
#ifdef __SHELL__

${OBJDUMP} -D -print-imm-hex -print-dbg -mcpu=r3 $1.elf > ${ROOT}/$1.lst
${OBJDUMP} -section-headers $1.elf > ${ROOT}/segment_list.txt
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 > ${ROOT}/symbol_tbl.txt
cp $1.map ${ROOT}/$1.map
${OBJDUMP} -section-headers $1.elf
/opt/utils/report_segment_usage --sdk_path ${ROOT} --tbl_file ${ROOT}/symbol_tbl.txt --seg_file ${ROOT}/segment_list.txt --map_file $1.map --module_depth "{\"apps\":1,\"lib\":2,\"[lib]\":2}"

/opt/utils/strip-ini -i isd_config.ini -o isd_config.ini

export PROJ_BUILD=download.bat

echo "echo 加一下中文注释,防止服务器导出不了报错" >> ${PROJ_BUILD}
echo "echo 加一下中文注释,防止服务器导出不了报错" >> ${PROJ_BUILD}
echo "echo 加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释,防止服务器导出不了报错,加一下中文注释" >> ${PROJ_BUILD}


echo "set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe" >> ${PROJ_BUILD}
echo "set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe" >> ${PROJ_BUILD}
echo "set ELFFILE=sdk.elf" >> ${PROJ_BUILD}
echo "set LZ4_PACKET=.\lz4_packet.exe" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .text %ELFFILE% text.bin" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .data %ELFFILE% data.bin" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .ram0_data  %ELFFILE% ram0_data.bin" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .dcache_ram_data  %ELFFILE% dcache_ram_data.bin" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .video_ram_data  %ELFFILE% video_ram_data.bin" >> ${PROJ_BUILD}
echo "%OBJCOPY% -O binary -j .dlog_data  %ELFFILE% dlog.bin" >> ${PROJ_BUILD}
echo "%OBJDUMP% -section-headers %ELFFILE%" >> ${PROJ_BUILD}
echo "%OBJDUMP% -t %ELFFILE% > symbol_tbl.txt" >> ${PROJ_BUILD}
#ifdef CONFIG_LZ4_DATA_CODE_ENABLE
echo "%LZ4_PACKET% -dict text.bin -input ram0_data.bin 0 data.bin 0 -o compress.bin" >> ${PROJ_BUILD}
echo "copy /b text.bin+dcache_ram_data.bin+video_ram_data.bin+compress.bin app.bin" >> ${PROJ_BUILD}
#else
echo "copy /b text.bin+data.bin+dcache_ram_data.bin+video_ram_data.bin+ram0_data.bin app.bin" >> ${PROJ_BUILD}
#endif

#if defined CONFIG_WIFI_IPC_PROJECT_ENABLE
echo "set UI_RES_PREFIX=ipc_" >> ${PROJ_BUILD}
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
echo "packres\packres.exe -n tone -o packres/AUPACKRES ipc_aud_res" >> ${PROJ_BUILD}
#endif
#elif defined CONFIG_WIFI_DVR_PROJECT_ENABLE
#ifdef CONFIG_UI_STYLE_LY_ENABLE
echo "set UI_RES_PREFIX=dvr_ly_" >> ${PROJ_BUILD}
#else
echo "set UI_RES_PREFIX=dvr_jl_" >> ${PROJ_BUILD}
#endif
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
echo "packres\packres.exe -n tone -o packres/AUPACKRES dvr_aud_res" >> ${PROJ_BUILD}
#endif
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
echo "packres\packres.exe -n tone -o packres/AUPACKRES dvr_aud_res" >> ${PROJ_BUILD}
#endif
echo "copy ..\\..\\..\\..\\wifi_camera\\output\\stream.bin stream.bin"     >> ${PROJ_BUILD}
echo "copy ..\\..\\..\\..\\wifi_camera\\output\\cfg_tool.bin cfg_tool.bin" >> ${PROJ_BUILD}
echo "copy ..\\..\\..\\..\\wifi_camera\\output\\tone_en.cfg tone_en.cfg"   >> ${PROJ_BUILD}
echo "copy ..\\..\\..\\..\\wifi_camera\\output\\tone_zh.cfg tone_zh.cfg"   >> ${PROJ_BUILD}
#elif defined CONFIG_DEMO_AUDIO_PROJECT_ENABLE
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE && defined CONFIG_VOICE_PROMPT_FILE_PATH
echo "packres\packres.exe -n tone -o packres/AUPACKRES story_aud_res" >> ${PROJ_BUILD}
#endif
#elif defined CONFIG_DEMO_UI_PROJECT_ENABLE
echo "set UI_RES_PREFIX=demo_" >> ${PROJ_BUILD}
#elif defined CONFIG_WIFI_SOUNDBOX_PROJECT_ENABLE
echo "set UI_RES_PREFIX=soundbox_" >> ${PROJ_BUILD}
#elif defined CONFIG_WIFI_STORY_MACHINE_PROJECT_ENABLE
echo "set UI_RES_PREFIX=story_" >> ${PROJ_BUILD}
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE && defined CONFIG_VOICE_PROMPT_FILE_PATH
echo "packres\packres.exe -n tone -o packres/AUPACKRES story_aud_res" >> ${PROJ_BUILD}
#endif
#elif defined CONFIG_WIFI_BBM_PROJECT_ENABLE
echo "set UI_RES_PREFIX=bbm_" >> ${PROJ_BUILD}
#endif

#if defined CONFIG_UI_ENABLE && defined CONFIG_UI_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
echo "packres\packres.exe -n ui -o packres/UIPACKRES %UI_RES_PREFIX%ui_res" >> ${PROJ_BUILD}
#endif
#if defined CONFIG_LOGO_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
echo "packres\packres.exe -n logo -o packres/LOGOPACKRES poweronoff_res" >> ${PROJ_BUILD}
#endif


#if defined CONFIG_MEDIA_ENABLE
echo "set MEDIA_RES=stream.bin -tone tone_zh.cfg tone_en.cfg" >> ${PROJ_BUILD}
#endif

#if defined CONFIG_DOUBLE_BANK_ENABLE
echo "set UPDATE_FILES=-update_files embedded" >> ${PROJ_BUILD}
#endif

#if defined CONFIG_EXFLASH_ENABLE
echo "packres\packres.exe -keep-suffix-case %UI_RES_PREFIX%ui_res/F_ASCII.PIX %UI_RES_PREFIX%ui_res/F_GB2312.PIX %UI_RES_PREFIX%ui_res/F_GB2312.TAB %UI_RES_PREFIX%ui_res/ascii.res -n res -o packres\font" >> ${PROJ_BUILD}
echo -n "packres\fat_comm.exe -pad-backup2 -force-align-fat -out new_res.bin -image-size ">> ${PROJ_BUILD}
echo -n __EXTERN_FLASH_SIZE_MB__ >> ${PROJ_BUILD}
echo -n " -filelist packres\font -remove-empty -remove-bpb -mark-bad-after 0xfe0000" >> ${PROJ_BUILD}
echo " -address 0" >> ${PROJ_BUILD}
echo "packres\packres.exe -n res -o exres.bin new_res.bin 0 -normal" >> ${PROJ_BUILD}
echo "set EXFLASH_FILES=-ex_flash exres.bin" >> ${PROJ_BUILD}
#endif

#if defined CONFIG_SDFILE_EXT_ENABLE
echo -n "fat_image_tool.exe --size " >> ${PROJ_BUILD}
echo -n CONFIG_SDNAND_HFAT_LEN_TEXT >> ${PROJ_BUILD}
echo -n " --sectors-per-cluster " >> ${PROJ_BUILD}
echo -n CONFIG_SDNAND_HFAT_CLUSTER_SIZE >> ${PROJ_BUILD}
echo " --lfn true --n-root 512 --volume-name HFAT_IMG --output jl_hfat.bin --fat-dir hfat_dir" >> ${PROJ_BUILD}
echo -n "fat_image_tool.exe --size " >> ${PROJ_BUILD}
echo -n CONFIG_SDNAND_FAT1_LEN_TEXT >> ${PROJ_BUILD}
echo -n " --sectors-per-cluster " >> ${PROJ_BUILD}
echo -n CONFIG_SDNAND_FAT1_CLUSTER_SIZE >> ${PROJ_BUILD}
echo " --lfn true --n-root 512 --volume-name FAT1_IMG --output jl_fat1.bin --fat-dir fat1_dir" >> ${PROJ_BUILD}
echo "isd_download.exe isd_config.ini -gen2 -to-sdcard -dev wl83 -boot 0x103000 -div1 -app app.bin cfg_tool.bin -res %AUDIO_RES% %UI_RES% cfg %MEDIA_RES% -extend-bin -output-bin jl_hfs.bin %UPDATE_FILES% -no-app-bin-enc" >> ${PROJ_BUILD}
#elif defined CONFIG_SFC_ENABLE || !defined CONFIG_LZ4_COMPRESS_APP_CODE_ENABLE
echo "isd_download.exe isd_config.ini -gen2 -tonorflash -dev wl83 -boot 0x103000 -div1 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin -res %AUDIO_RES% %UI_RES% cfg %MEDIA_RES% -reboot 50 %UPDATE_FILES% -extend-bin %EXFLASH_FILES% -output-ufw jl_isd.ufw" >> ${PROJ_BUILD}
#else
//如果不压缩uboot, 需要删除tools目录下的rom.image
//先暂时删掉rom.image，会导致不压缩uboot时代码解压出错，后面有需要再加回去
echo "set run_addr=0x103000" >> ${PROJ_BUILD}
echo "set load_addr=0x10B000" >> ${PROJ_BUILD}
echo "set mask_addr=0xf0000" >> ${PROJ_BUILD}
echo "del rom.image" >> ${PROJ_BUILD}
echo "uboot_lz4.exe app.bin app.lz4 %run_addr% %load_addr% rom.image %mask_addr%" >> ${PROJ_BUILD}
echo "isd_download.exe isd_config.ini -gen2 -tonorflash -dev wl83 -boot 0x103000 -div1 -wait 300 -uboot uboot.boot -app app.lz4 cfg_tool.bin -res %AUDIO_RES% %UI_RES% cfg %MEDIA_RES% -reboot 50 %UPDATE_FILES% -extend-bin %EXFLASH_FILES% -output-ufw jl_isd.ufw" >> ${PROJ_BUILD}
#endif

#if defined(CONFIG_DOUBLE_BANK_ENABLE) && defined(CONFIG_UPDATE_COMPRESS)
echo "isd_download.exe -make-upgrade-bin -ufw jl_isd.ufw -output db_update.bin" >> ${PROJ_BUILD}
#endif


#if 0
echo "if exist *.mp3 del *.mp3" >> ${PROJ_BUILD}
echo "if exist *.PIX del *.PIX" >> ${PROJ_BUILD}
echo "if exist *.TAB del *.TAB" >> ${PROJ_BUILD}
echo "if exist *.res del *.res" >> ${PROJ_BUILD}
echo "if exist *.sty del *.sty" >> ${PROJ_BUILD}
#endif

#if defined CONFIG_SDFILE_EXT_ENABLE
echo "copy isd_config_loader.ini loader_tools\isd_config.ini" >> ${PROJ_BUILD}
echo "copy jl_hfs.bin loader_tools\jl_hfs.bin" >> ${PROJ_BUILD}
echo "copy jl_hfat.bin loader_tools\jl_hfat.bin" >> ${PROJ_BUILD}
echo "copy jl_fat1.bin loader_tools\jl_fat1.bin" >> ${PROJ_BUILD}
#else
#if !defined CONFIG_DOUBLE_BANK_ENABLE
echo "fw_add.exe -noenc -fw jl_isd.fw -add ota.bin -type 100 -out jl_isd.fw" >> ${PROJ_BUILD}
echo "fw_add.exe -noenc -fw jl_isd.fw -add version.ver -out jl_isd.fw" >> ${PROJ_BUILD}
#endif

echo "fw_add.exe -noenc -fw jl_isd.fw -add script.ver -out jl_isd.fw" >> ${PROJ_BUILD}

echo "ufw_maker.exe -fw_to_ufw jl_isd.fw" >> ${PROJ_BUILD}

#if defined(CONFIG_DOUBLE_BANK_ENABLE) && defined(CONFIG_UPDATE_COMPRESS)
echo "fw_add.exe -noenc -fw jl_isd.fw -add db_update.bin -out update-com.fw" >> ${PROJ_BUILD}
echo "ufw_maker.exe -fw_to_ufw update-com.fw" >> ${PROJ_BUILD}
#endif

#endif

echo "ping /n 2 127.1>null" >> ${PROJ_BUILD}
//echo "IF EXIST null del null" >> ${PROJ_BUILD}
echo "del video_ram_data.bin" >> ${PROJ_BUILD}
echo "del dcache_ram_data.bin" >> ${PROJ_BUILD}
echo "del data.bin" >> ${PROJ_BUILD}
echo "del ram0_data.bin" >> ${PROJ_BUILD}
#ifdef CONFIG_LZ4_DATA_CODE_ENABLE
echo "del compress.bin" >> ${PROJ_BUILD}
#endif
echo "del text.bin" >> ${PROJ_BUILD}
#if defined CONFIG_SDFILE_EXT_ENABLE
echo "del jl_hfs.bin" >> ${PROJ_BUILD}
echo "del jl_hfat.bin" >> ${PROJ_BUILD}
echo "del jl_fat1.bin" >> ${PROJ_BUILD}
echo "cd loader_tools" >> ${PROJ_BUILD}
echo "call download.bat" >> ${PROJ_BUILD}
echo "cd ..\\" >> ${PROJ_BUILD}
#endif
echo "TIMEOUT /T 3" >> ${PROJ_BUILD}
echo "exit /b 0" >> ${PROJ_BUILD}

#if defined CONFIG_SDFILE_EXT_ENABLE
files="isd_config.ini isd_config_loader.ini download.bat"
#else
files="isd_config.ini download.bat"
#endif

host-client -project ${NICKNAME}$2 -f ${files} $1.elf

rm download.bat
rm isd_config.ini

#else

@echo off

@echo *********************************************************************
@echo                           AC792N SDK
@echo *********************************************************************
@echo %date%

cd /d %~dp0

echo %*


set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf
set LZ4_PACKET=lz4_packet.exe

REM %OBJDUMP% -D -address-mask=0x1ffffff -print-dbg %ELFFILE% > sdk.lst
%OBJCOPY% -O binary -j .text %ELFFILE% text.bin
%OBJCOPY% -O binary -j .data %ELFFILE% data.bin
%OBJCOPY% -O binary -j .ram0_data  %ELFFILE% ram0_data.bin
%OBJCOPY% -O binary -j .dcache_ram_data  %ELFFILE% dcache_ram_data.bin
%OBJCOPY% -O binary -j .video_ram_data  %ELFFILE% video_ram_data.bin
%OBJCOPY% -O binary -j .dlog_data %ELFFILE% dlog.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
%OBJDUMP% -t %ELFFILE% > symbol_tbl.txt

#ifdef CONFIG_LZ4_DATA_CODE_ENABLE
%LZ4_PACKET% -dict text.bin -input ram0_data.bin 0 data.bin 0 -o compress.bin
copy /b text.bin+dcache_ram_data.bin+video_ram_data.bin+compress.bin app.bin
#else
copy /b text.bin+data.bin+dcache_ram_data.bin+video_ram_data.bin+ram0_data.bin app.bin
#endif

#if defined CONFIG_WIFI_IPC_PROJECT_ENABLE
set UI_RES_PREFIX=ipc_
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
packres\packres.exe -n tone -o packres/AUPACKRES ipc_aud_res
#endif
#elif defined CONFIG_WIFI_DVR_PROJECT_ENABLE
#ifdef CONFIG_UI_STYLE_LY_ENABLE
set UI_RES_PREFIX=dvr_ly_
#else
set UI_RES_PREFIX=dvr_jl_
#endif
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
packres\packres.exe -n tone -o packres/AUPACKRES dvr_aud_res
#endif
copy ../../../../wifi_camera/output/stream.bin stream.bin
copy ../../../../wifi_camera/output/cfg_tool.bin cfg_tool.bin
copy ../../../../wifi_camera/output/tone_en.cfg tone_en.cfg
copy ../../../../wifi_camera/output/tone_zh.cfg tone_en.cfg
#elif defined CONFIG_DEMO_AUDIO_PROJECT_ENABLE
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE && defined CONFIG_VOICE_PROMPT_FILE_PATH
packres\packres.exe -n tone -o packres/AUPACKRES story_aud_res
#endif
#elif defined CONFIG_DEMO_UI_PROJECT_ENABLE
set UI_RES_PREFIX=demo_
#elif defined CONFIG_WIFI_SOUNDBOX_PROJECT_ENABLE
set UI_RES_PREFIX=soundbox_
#elif defined CONFIG_WIFI_STORY_MACHINE_PROJECT_ENABLE
set UI_RES_PREFIX=story_
#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE && defined CONFIG_VOICE_PROMPT_FILE_PATH
packres\packres.exe -n tone -o packres/AUPACKRES story_aud_res
#endif
#elif defined CONFIG_WIFI_BBM_PROJECT_ENABLE
set UI_RES_PREFIX=bbm_
#endif

#if defined CONFIG_UI_ENABLE && defined CONFIG_UI_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
packres\packres.exe -n ui -o packres/UIPACKRES %UI_RES_PREFIX%ui_res
#endif

#if defined CONFIG_LOGO_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
packres\packres.exe -n logo -o packres/LOGOPACKRES poweronoff_res
#endif

#if defined CONFIG_MEDIA_ENABLE
#if TCFG_TONE_EN_ENABLE
set TONE_EN_ENABLE=1
#else
set TONE_EN_ENABLE=0
#endif
#if TCFG_TONE_ZH_ENABLE
set TONE_ZH_ENABLE=1
#else
set TONE_ZH_ENABLE=0
#endif
if not %KEY_FILE_PATH%A==A set KEY_FILE=-key %KEY_FILE_PATH%
if %PROJ_DOWNLOAD_PATH%A==A set PROJ_DOWNLOAD_PATH=..\..\..\..\..\output
copy %PROJ_DOWNLOAD_PATH%\*.bin .
if exist %PROJ_DOWNLOAD_PATH%\tone_en.cfg copy %PROJ_DOWNLOAD_PATH%\tone_en.cfg .
if exist %PROJ_DOWNLOAD_PATH%\tone_zh.cfg copy %PROJ_DOWNLOAD_PATH%\tone_zh.cfg .

if %TONE_EN_ENABLE%A==1A (
    set TONE_FILES=tone_en.cfg
)
if %TONE_ZH_ENABLE%A==1A (
    set TONE_FILES=%TONE_FILES% tone_zh.cfg
)

if %FORMAT_VM_ENABLE%A==1A set FORMAT=-format vm
if %FORMAT_ALL_ENABLE%A==1A set FORMAT=-format all

set MEDIA_RES=stream.bin -tone %TONE_FILES%
#endif

REM set KEY_FILE=-key JL_792N-XXXX.key
REM set KEY_FILE=-key1 JL_792N-XXXX.key1 -mkey JL_792N-XXXX.mkey

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

#if defined CONFIG_DOUBLE_BANK_ENABLE
set UPDATE_FILES=-update_files embedded
#endif

#if defined CONFIG_EXFLASH_ENABLE
packres\packres.exe -keep-suffix-case %UI_RES_PREFIX%ui_res/F_ASCII.PIX %UI_RES_PREFIX%ui_res/F_GB2312.PIX %UI_RES_PREFIX%ui_res/F_GB2312.TAB %UI_RES_PREFIX%ui_res/ascii.res -n res -o packres\font
packres\fat_comm.exe -pad-backup2 -force-align-fat -out new_res.bin -image-size __EXTERN_FLASH_SIZE_MB__ -filelist font -remove-empty -remove-bpb -mark-bad-after 0xfe0000 -address 0 %KEY_FILE%
packres\packres.exe -n res -o exres.bin new_res.bin 0 -normal
set EXFLASH_FILES=-ex_flash exres.bin
#endif

#if defined CONFIG_SDFILE_EXT_ENABLE
fat_image_tool.exe --size CONFIG_SDNAND_HFAT_LEN_TEXT --sectors-per-cluster CONFIG_SDNAND_HFAT_CLUSTER_SIZE --lfn true --n-root 512 --volume-name HFAT_IMG --output jl_hfat.bin --fat-dir hfat_dir
fat_image_tool.exe --size CONFIG_SDNAND_FAT1_LEN_TEXT --sectors-per-cluster CONFIG_SDNAND_FAT1_CLUSTER_SIZE --lfn true --n-root 512 --volume-name FAT1_IMG --output jl_fat1.bin --fat-dir fat1_dir
isd_download.exe isd_config.ini -gen2 -to-sdcard -dev wl83 -boot 0x103000 -div1 -app app.bin cfg_tool.bin -res %AUDIO_RES% %UI_RES% cfg %MEDIA_RES% -extend-bin -output-bin jl_hfs.bin %UPDATE_FILES% -no-app-bin-enc
#elif defined CONFIG_SFC_ENABLE || !defined CONFIG_LZ4_COMPRESS_APP_CODE_ENABLE
isd_download.exe isd_config.ini -gen2 -tonorflash -dev wl83 -boot 0x103000 -div1 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin -res %AUDIO_RES% %UI_RES% %CFG_FILE% %MEDIA_RES% -reboot 50 %KEY_FILE% %UPDATE_FILES% -extend-bin %EXFLASH_FILES% -output-ufw jl_isd.ufw %FORMAT%
#else
REM 如果不压缩uboot, 需要删除tools目录下的rom.image
REM 先暂时删掉rom.image，会导致不压缩uboot时代码解压出错，后面有需要再加回去
set run_addr=0x103000
set load_addr=0x10B000
set mask_addr=0xf0000
del rom.image
uboot_lz4.exe app.bin app.lz4 %run_addr% %load_addr% rom.image %mask_addr%
REM @@@@@@@@@@@@@@@

isd_download.exe isd_config.ini -gen2 -tonorflash -dev wl83 -boot 0x103000 -div1 -wait 300 -uboot uboot.boot -app app.lz4 cfg_tool.bin -res %AUDIO_RES% %UI_RES% %CFG_FILE% %MEDIA_RES% -reboot 50 %KEY_FILE% %UPDATE_FILES% -extend-bin %EXFLASH_FILES% -output-ufw jl_isd.ufw %FORMAT%
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

#if defined CONFIG_SDFILE_EXT_ENABLE
copy isd_config_loader.ini loader_tools\isd_config.ini
copy jl_hfs.bin loader_tools\jl_hfs.bin
copy jl_hfat.bin loader_tools\jl_hfat.bin
copy jl_fat1.bin loader_tools\jl_fat1.bin
#else
#if !defined CONFIG_DOUBLE_BANK_ENABLE
@REM 生成固件升级文件
fw_add.exe -noenc -fw jl_isd.fw -add ota.bin -type 100 -out jl_isd.fw
#endif

@REM 添加配置脚本的版本信息到 FW 文件中
fw_add.exe -noenc -fw jl_isd.fw -add script.ver -out jl_isd.fw

@REM 添加SDK softwate信道到 FW 文件中
fw_add.exe -noenc -fw jl_isd.fw -add version.ver -out jl_isd.fw

ufw_maker.exe -fw_to_ufw jl_isd.fw
#endif

@REM 生成配置文件升级文件
rem ufw_maker.exe -chip AC792N %ADD_KEY% -output config.ufw -res bt_cfg.cfg

ping /n 2 127.1>null
IF EXIST null del null

::del app.bin
del video_ram_data.bin
del dcache_ram_data.bin
del data.bin
del ram0_data.bin
del text.bin
#ifdef CONFIG_LZ4_DATA_CODE_ENABLE
del compress.bin
#endif

#if defined CONFIG_SDFILE_EXT_ENABLE
del jl_hfs.bin
del jl_hfat.bin
del jl_fat1.bin
cd loader_tools
call download.bat
cd ..\

#endif

::退出当前批处理加返回值(0),才能多个批处理嵌套调用批处理
exit /b 0

#endif
// *INDENT-ON*

