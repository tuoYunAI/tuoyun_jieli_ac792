REM del Makefile /s 
del *.d /s
del *.o /s
del *.bmarks /s
del *.depend /s
del *.layout /s
del map.txt
del *.or32 /s
del *.format_orig /s
del *.bc /s
del *.app /s
del app.bin /s
del data.bin /s
del ram0_data.bin /s
del sdram.bin /s
del sdram_text.bin /s
del cache_ram_data.bin /s
del __zone2.zon /s
del cmd_zone /s
del sdram.apu /s
del dual_uvc_version.z /s
del ram.ld /s
del ram_sfc.ld /s
del ram2_data.bin /s
del dual_uvc_version.S /s
del jl_isd.bin /s
del jl_isd.fw /s
del jl_isd.ufw /s
del jl_isd_extend.bin /s
del *.resolution.txt /s
del main.lst /s
del sdk.elf /s
del symbol_tbl.txt /s
del app.lz4 /s
del db_update_data.bin /s
del db_update_files_data.bin /s
del cpu\wl83\tools\download.bat
del isd_config.ini /s
del extern_con.bin /ss
del ota_debug.bin /s
del p11_code.bin /s
del sdk_used_list.used /s
del uboot.boot_debug /s
del uboot_no_ota.boot /s
del uboot_no_ota.boot_debug /s
del power_up /s
del *.ai /s
del .gitignore /s
del *.in /s
del *.am /s
del *.mk /s
@for /r . %%a in (.) do @if exist "%%a/.git" rd /s /q "%%a/.git"
@for /r . %%a in (.) do @if exist "%%a/.github" rd /s /q "%%a/.github"
@for /r . %%a in (.) do @if exist "%%a/.obj" rd /s /q "%%a/.obj"
